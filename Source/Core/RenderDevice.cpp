#include "RenderDevice.hpp"
#include "RenderInstance.hpp"
#include "Core/BellLogging.hpp"
#include "Core/ConversionUtils.hpp"
#include "Core/Pipeline.hpp"

#include <glslang/Public/ShaderLang.h>
#include <vulkan/vulkan.hpp>

#include <limits>
#include <vector>


RenderDevice::RenderDevice(vk::Instance instance,
						   vk::PhysicalDevice physDev,
						   vk::Device dev,
						   vk::SurfaceKHR surface,
						   GLFWwindow* window) :
    mFinishedSubmission{0},
    mCurrentFrameIndex{0},
    mDevice{dev},
    mPhysicalDevice{physDev},
	mInstance(instance),
	mHasDebugLableSupport{false},
    mSwapChain{this, surface, window},
    mMemoryManager{this},
	mDescriptorManager{this}
{
    // This is a bit of a hack to work around not being able to tell for the first few frames that
    // the fences we waited on hadn't been submitted (as no work has been done).
    mCurrentSubmission = mSwapChain.getNumberOfSwapChainImages();

    mQueueFamilyIndicies = getAvailableQueues(surface, mPhysicalDevice);
    mGraphicsQueue = mDevice.getQueue(static_cast<uint32_t>(mQueueFamilyIndicies.GraphicsQueueIndex), 0);
    mComputeQueue  = mDevice.getQueue(static_cast<uint32_t>(mQueueFamilyIndicies.ComputeQueueIndex), 0);
    mTransferQueue = mDevice.getQueue(static_cast<uint32_t>(mQueueFamilyIndicies.TransferQueueIndex), 0);

    mLimits = mPhysicalDevice.getProperties().limits;

	// Create a command pool for each frame.
    mCommandPools.reserve(mSwapChain.getNumberOfSwapChainImages());
	for (uint32_t i = 0; i < mSwapChain.getNumberOfSwapChainImages(); ++i)
	{
        mCommandPools.emplace_back(this);
	}

    mFrameFinished.reserve(mSwapChain.getNumberOfSwapChainImages());
    for (uint32_t i = 0; i < mSwapChain.getNumberOfSwapChainImages(); ++i)
    {
        mFrameFinished.push_back(createFence(true));
        vk::SemaphoreCreateInfo semInfo{};
        mImageAquired.push_back(mDevice.createSemaphore(semInfo));
        mImageRendered.push_back(mDevice.createSemaphore(semInfo));
    }

    // Initialise the glsl compiler here rather than in the first compiled shader to avoid
    // having to synchronise there.
    const bool glslInitialised = glslang::InitializeProcess();
    BELL_ASSERT(glslInitialised, "FAILED TO INITIALISE GLSLANG, we will not be able to compile any shaders from source!")

#ifndef NDEBUG
    vk::EventCreateInfo eventInfo{};
    mDebugEvent = mDevice.createEvent(eventInfo);
#endif

	// Check for the debug names extension
	for(const auto extensionProperty : mPhysicalDevice.enumerateDeviceExtensionProperties())
	{
		if(strcmp(VK_EXT_DEBUG_MARKER_EXTENSION_NAME, extensionProperty.extensionName) == 0)
			mHasDebugLableSupport = true;
	}
}


RenderDevice::~RenderDevice()
{
	flushWait();

    // destroy the swapchain first so that is can add it's image views to the deferred destruction queue.
    mSwapChain.destroy();

    // We can ignore lastUsed as we have just waited till all work has finished.
    for(auto& [lastUsed, handle, memory, imageView] : mImagesPendingDestruction)
    {
        if(imageView != vk::ImageView{nullptr})
            mDevice.destroyImageView(imageView);

        if(handle != vk::Image{nullptr})
        {
            mDevice.destroyImage(handle);
            mMemoryManager.Free(memory);
        }
    }
    mImagesPendingDestruction.clear();

    for(auto& [lastUsed, handle, memory] : mBuffersPendingDestruction)
    {
        mDevice.destroyBuffer(handle);
        mMemoryManager.Free(memory);
    }
    mBuffersPendingDestruction.clear();

    for(auto& resources : mVulkanResources)
    {
        if(resources.mFrameBuffer)
            mFramebuffersPendingDestruction.push_back({0, *resources.mFrameBuffer});
    }

	for(const auto& [lastUsed, frameBuffer] : mFramebuffersPendingDestruction)
    {
        mDevice.destroyFramebuffer(frameBuffer);
    }

    mMemoryManager.Destroy();

    for(auto& [desc, graphicsHandles] : mGraphicsPipelineCache)
    {
        mDevice.destroyPipeline(graphicsHandles.mGraphicsPipeline->getHandle());
        mDevice.destroyPipelineLayout(graphicsHandles.mGraphicsPipeline->getLayoutHandle());
        mDevice.destroyRenderPass(graphicsHandles.mRenderPass);
        mDevice.destroyDescriptorSetLayout(graphicsHandles.mDescriptorSetLayout);
    }

    for(auto& [desc, computeHandles]: mComputePipelineCache)
    {
        mDevice.destroyPipeline(computeHandles.mComputePipeline->getHandle());
        mDevice.destroyPipelineLayout(computeHandles.mComputePipeline->getLayoutHandle());
        mDevice.destroyDescriptorSetLayout(computeHandles.mDescriptorSetLayout);
    }

    for(auto& fence : mFrameFinished)
    {
        mDevice.destroyFence(fence);
    }

    for(auto& semaphore : mImageAquired)
    {
        mDevice.destroySemaphore(semaphore);
    }

    for(auto& semaphore : mImageRendered)
    {
        mDevice.destroySemaphore(semaphore);
    }

    for(auto& [samplerType, sampler] : mImmutableSamplerCache)
    {
        mDevice.destroySampler(sampler);
    }

#ifndef NDEBUG
    mDevice.destroyEvent(mDebugEvent);
#endif
}


vk::Buffer  RenderDevice::createBuffer(const uint32_t size, const vk::BufferUsageFlags usage)
{
    vk::BufferCreateInfo createInfo{};
    createInfo.setSize(size);
    createInfo.setUsage(usage);

    return mDevice.createBuffer(createInfo);
}


vk::Queue&  RenderDevice::getQueue(const QueueType type)
{
    switch(type)
    {
        case QueueType::Graphics:
            return mGraphicsQueue;
        case QueueType::Compute:
            return mComputeQueue;
        case QueueType::Transfer:
            return mTransferQueue;
        default:
            return mGraphicsQueue;
    }
}


uint32_t RenderDevice::getQueueFamilyIndex(const QueueType type) const
{
	switch(type)
	{
	case QueueType::Graphics:
        return static_cast<uint32_t>(mQueueFamilyIndicies.GraphicsQueueIndex);
	case QueueType::Compute:
        return static_cast<uint32_t>(mQueueFamilyIndicies.ComputeQueueIndex);
	case QueueType::Transfer:
        return static_cast<uint32_t>(mQueueFamilyIndicies.TransferQueueIndex);
    default:
        return static_cast<uint32_t>(mQueueFamilyIndicies.GraphicsQueueIndex);
	}
}


GraphicsPipelineHandles RenderDevice::createPipelineHandles(const GraphicsTask& task)
{
    if(mGraphicsPipelineCache[task.getPipelineDescription()].mGraphicsPipeline)
        return mGraphicsPipelineCache[task.getPipelineDescription()];

    const vk::DescriptorSetLayout descSetLayout = generateDescriptorSetLayout(task);
    const vk::RenderPass renderPass = generateRenderPass(task);
    const auto pipeline = generatePipeline( task,
													descSetLayout,
													renderPass);

    GraphicsPipelineHandles handles{pipeline, renderPass, descSetLayout};
    mGraphicsPipelineCache[task.getPipelineDescription()] = handles;

    return handles;
}


ComputePipelineHandles RenderDevice::createPipelineHandles(const ComputeTask& task)
{
    if(mComputePipelineCache[task.getPipelineDescription()].mComputePipeline->getHandle() != vk::Pipeline{nullptr})
        return mComputePipelineCache[task.getPipelineDescription()];

    const vk::DescriptorSetLayout descSetLayout = generateDescriptorSetLayout(task);
    const auto pipeline = generatePipeline(task, descSetLayout);

    ComputePipelineHandles handles{pipeline, descSetLayout};
    mComputePipelineCache[task.getPipelineDescription()] = handles;

    return handles;
}


std::shared_ptr<GraphicsPipeline> RenderDevice::generatePipeline(const GraphicsTask& task,
											const vk::DescriptorSetLayout descSetLayout,
											const vk::RenderPass &renderPass)
{
    const GraphicsPipelineDescription& pipelineDesc = task.getPipelineDescription();
	const vk::PipelineLayout pipelineLayout = generatePipelineLayout(descSetLayout, task);

	std::shared_ptr<GraphicsPipeline> pipeline = std::make_shared<GraphicsPipeline>(this, pipelineDesc);
	pipeline->setLayout(pipelineLayout);
	pipeline->setDescriptorLayout(descSetLayout);
	pipeline->setFrameBufferBlendStates(task);
	pipeline->setRenderPass(renderPass);
	pipeline->setVertexAttributes(task.getVertexAttributes());

	pipeline->compile(task);

	return pipeline;
}


std::shared_ptr<ComputePipeline> RenderDevice::generatePipeline(const ComputeTask& task,
                                            const vk::DescriptorSetLayout& descriptorSetLayout)
{
    const ComputePipelineDescription pipelineDesc = task.getPipelineDescription();
	const vk::PipelineLayout pipelineLayout = generatePipelineLayout(descriptorSetLayout, task);

	std::shared_ptr<ComputePipeline> pipeline = std::make_unique<ComputePipeline>(this, pipelineDesc);
	pipeline->setLayout(pipelineLayout);
	pipeline->compile(task);

	return pipeline;
}


vk::RenderPass	RenderDevice::generateRenderPass(const GraphicsTask& task)
{
    const auto& outputAttachments = task.getOuputAttachments();

    std::vector<vk::AttachmentDescription> attachmentDescriptions;

    // needed for subpass createInfo;
    bool hasDepthAttachment = false;
    std::vector<vk::AttachmentReference> outputAttachmentRefs{};
    std::vector<vk::AttachmentReference> depthAttachmentRef{};
    uint32_t outputAttatchmentCounter = 0;

	for(const auto& [name, type, format, loadop] : outputAttachments)
    {
        // We only care about images here.
        if(type == AttachmentType::DataBuffer ||
           type == AttachmentType::PushConstants ||
           type == AttachmentType::UniformBuffer)
        {
                ++outputAttatchmentCounter;
                continue;
        }

		vk::ImageLayout layout = getVulkanImageLayout(type);
		vk::Format adjustedFormat = getVulkanImageFormat(format);

		if(layout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
		{
			hasDepthAttachment = true;
			depthAttachmentRef.push_back({outputAttatchmentCounter, layout});
		}
		else
			outputAttachmentRefs.push_back({outputAttatchmentCounter, layout});

        vk::AttachmentDescription attachmentDesc{};
		attachmentDesc.setFormat(adjustedFormat);

        // eventually I will implment a render pass system that is aware of what comes beofer and after it
        // in order to avoid having to do manula barriers for all transitions.
        attachmentDesc.setInitialLayout((layout));
        if(type == AttachmentType::SwapChain)
        {
            attachmentDesc.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
        }
        else
        {
            attachmentDesc.setFinalLayout(layout);
        }
		vk::AttachmentLoadOp op = [loadop](){
			switch(loadop)
			{
				case LoadOp::Preserve:
					return vk::AttachmentLoadOp::eLoad;
				default:
					return vk::AttachmentLoadOp::eClear;
			}
		}();

		attachmentDesc.setLoadOp(op);

        attachmentDesc.setStoreOp(vk::AttachmentStoreOp::eStore);
		attachmentDesc.setStencilLoadOp(op);
        attachmentDesc.setStencilStoreOp(vk::AttachmentStoreOp::eStore);

        attachmentDescriptions.push_back((attachmentDesc));

        ++outputAttatchmentCounter;
    }

    vk::SubpassDescription subpassDesc{};
    subpassDesc.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpassDesc.setColorAttachmentCount(static_cast<uint32_t>(outputAttachmentRefs.size()));
	subpassDesc.setPColorAttachments(outputAttachmentRefs.data());
    if(hasDepthAttachment)
    {
        subpassDesc.setPDepthStencilAttachment(depthAttachmentRef.data());
    }

    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.setAttachmentCount(static_cast<uint32_t>(attachmentDescriptions.size()));
    renderPassInfo.setPAttachments(attachmentDescriptions.data());
    renderPassInfo.setSubpassCount(1);
    renderPassInfo.setPSubpasses(&subpassDesc);

    return mDevice.createRenderPass(renderPassInfo);
}


std::vector<vk::PipelineShaderStageCreateInfo> RenderDevice::generateShaderStagesInfo(const GraphicsTask& task)
{
    const GraphicsPipelineDescription pipelineDesc = task.getPipelineDescription();

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

    vk::PipelineShaderStageCreateInfo vertexStage{};
    vertexStage.setStage(vk::ShaderStageFlagBits::eVertex);
    vertexStage.setPName("main"); //entry point of the shader
    vertexStage.setModule(pipelineDesc.mVertexShader.getShaderModule());
    shaderStages.push_back(vertexStage);

    if(pipelineDesc.mGeometryShader)
    {
        vk::PipelineShaderStageCreateInfo geometryStage{};
        geometryStage.setStage(vk::ShaderStageFlagBits::eGeometry);
        geometryStage.setPName("main"); //entry point of the shader
        geometryStage.setModule(pipelineDesc.mGeometryShader.value().getShaderModule());
        shaderStages.push_back(geometryStage);
    }

    if(pipelineDesc.mHullShader)
    {
        vk::PipelineShaderStageCreateInfo hullStage{};
        hullStage.setStage(vk::ShaderStageFlagBits::eTessellationControl);
        hullStage.setPName("main"); //entry point of the shader
        hullStage.setModule(pipelineDesc.mHullShader.value().getShaderModule());
        shaderStages.push_back(hullStage);
    }

    if(pipelineDesc.mHullShader)
    {
        vk::PipelineShaderStageCreateInfo tesseStage{};
        tesseStage.setStage(vk::ShaderStageFlagBits::eTessellationEvaluation);
        tesseStage.setPName("main"); //entry point of the shader
        tesseStage.setModule(pipelineDesc.mTesselationControlShader.value().getShaderModule());
        shaderStages.push_back(tesseStage);
    }

    vk::PipelineShaderStageCreateInfo fragmentStage{};
    fragmentStage.setStage(vk::ShaderStageFlagBits::eFragment);
    fragmentStage.setPName("main"); //entry point of the shader
    fragmentStage.setModule(pipelineDesc.mFragmentShader.getShaderModule());
    shaderStages.push_back(fragmentStage);

    return shaderStages;
}


vk::DescriptorSetLayout RenderDevice::generateDescriptorSetLayout(const RenderTask& task)
{
    const auto& inputAttachments = task.getInputAttachments();

    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
    uint32_t curretnBinding = 0;

    for(const auto& [name, type] : inputAttachments)
    {
        // TRanslate between Bell enums to the vulkan equivelent.
        vk::DescriptorType descriptorType = [type]()
        {
            switch(type)
            {
            case AttachmentType::Texture1D:
            case AttachmentType::Texture2D:
            case AttachmentType::Texture3D:
				return vk::DescriptorType::eSampledImage;

            case AttachmentType::Image1D:
            case AttachmentType::Image2D:
            case AttachmentType::Image3D:
                return vk::DescriptorType::eStorageImage;

           case AttachmentType::DataBuffer:
                return vk::DescriptorType::eStorageBufferDynamic;

           case AttachmentType::UniformBuffer:
                return vk::DescriptorType::eUniformBuffer;

           case AttachmentType::Sampler:
                return vk::DescriptorType::eSampler;

           // Ignore push constants for now.
            }

            return vk::DescriptorType::eCombinedImageSampler; // For now use this to indicate push_constants (terrible I know)
        }();

		// This indicates it's a push constant which we don't need to handle when allocating descriptor
		// sets.
		if(descriptorType == vk::DescriptorType::eCombinedImageSampler)
			continue;

        vk::DescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.setBinding(curretnBinding++);
        layoutBinding.setDescriptorType(descriptorType);
        layoutBinding.setDescriptorCount(1);
        // TODO make this less general in the future/effiecent.
        layoutBinding.setStageFlags(vk::ShaderStageFlagBits::eAll);

        layoutBindings.push_back(layoutBinding);
    }

    vk::DescriptorSetLayoutCreateInfo descSetLayoutInfo{};
    descSetLayoutInfo.setPBindings(layoutBindings.data());
    descSetLayoutInfo.setBindingCount(static_cast<uint32_t>(layoutBindings.size()));

    return mDevice.createDescriptorSetLayout(descSetLayoutInfo);
}


vk::PipelineLayout RenderDevice::generatePipelineLayout(vk::DescriptorSetLayout descLayout, const RenderTask& task)
{
    vk::PipelineLayoutCreateInfo info{};
    info.setSetLayoutCount(1);
    info.setPSetLayouts(&descLayout);

	const auto& inputAttachments = task.getInputAttachments();
	const bool hasPushConstants =  std::find_if(inputAttachments.begin(), inputAttachments.end(), [](const auto& attachment)
	{
		return attachment.second == AttachmentType::PushConstants;
	}) != inputAttachments.end();

	// at least for now just use a mat4 for push constants.
	vk::PushConstantRange range{};
	range.setSize(sizeof(glm::mat4));
	range.setOffset(0);
	range.setStageFlags(vk::ShaderStageFlagBits::eAll);

	if(hasPushConstants)
	{
		info.setPPushConstantRanges(&range);
		info.setPushConstantRangeCount(1);
	}

    return mDevice.createPipelineLayout(info);
}


void RenderDevice::generateVulkanResources(RenderGraph& graph)
{
	auto task = graph.taskBegin();
    mVulkanResources.resize(graph.mTaskOrder.size());
    auto resource = mVulkanResources.begin();

	while( task != graph.taskEnd())
    {
		if ((*resource).mPipeline && (*resource).mPipeline->getHandle() != vk::Pipeline{ nullptr })
		{
			++task;
			++resource;
			continue;
		}

        if((*task).taskType() == TaskType::Graphics)
        {
            const GraphicsTask& graphicsTask = static_cast<const GraphicsTask&>(*task);
            GraphicsPipelineHandles pipelineHandles = createPipelineHandles(graphicsTask);

            vulkanResources& taskResources = *resource;
            taskResources.mPipeline = pipelineHandles.mGraphicsPipeline;
            taskResources.mDescSetLayout = pipelineHandles.mDescriptorSetLayout;
            taskResources.mRenderPass = pipelineHandles.mRenderPass;
            // Frame buffers and descset get created/written in a diferent place.
        }
        else
        {
            const ComputeTask& computeTask = static_cast<const ComputeTask&>(*task);
            ComputePipelineHandles pipelineHandles = createPipelineHandles(computeTask);

            vulkanResources& taskResources = *resource;
            taskResources.mPipeline = pipelineHandles.mComputePipeline;
            taskResources.mDescSetLayout = pipelineHandles.mDescriptorSetLayout;
        }
		++task;
		++resource;
    }
    generateFrameBuffers(graph);
    generateDescriptorSets(graph);
}


void RenderDevice::generateFrameBuffers(RenderGraph& graph)
{
	auto outputBindings = graph.outputBindingBegin();
	auto resource = mVulkanResources.begin();
	uint32_t taskIndex = 0;

	while(outputBindings != graph.outputBindingEnd())
    {
        // Just reuse the old frameBuffer if no knew resources have been bound.
        if(!graph.mFrameBuffersNeedUpdating[taskIndex])
        {
            ++resource;
            ++outputBindings;
			++taskIndex;
            continue;
        }

        std::vector<vk::ImageView> imageViews{};
        vk::Extent3D imageExtent;

		// Sort the bindings by location within the frameBuffer.
		// Resources aren't always bound in order so make sure that they are in order when we iterate over them.
		std::sort((*outputBindings).begin(), (*outputBindings).end(), [](const auto& lhs, const auto& rhs)
		{
			return lhs.mResourceBinding < rhs.mResourceBinding;
		});

        for(const auto& bindingInfo : *outputBindings)
        {
                const auto& imageView = graph.getImageView(bindingInfo.mResourceIndex);
                imageExtent = imageView.getImageExtent();
                imageViews.push_back(imageView.getImageView());
        }

        vk::FramebufferCreateInfo info{};
        info.setRenderPass(*(*resource).mRenderPass);
        info.setAttachmentCount(static_cast<uint32_t>(imageViews.size()));
        info.setPAttachments(imageViews.data());
        info.setWidth(imageExtent.width);
        info.setHeight(imageExtent.height);
        info.setLayers(imageExtent.depth);

		// Make sure we don't leak frameBuffers, add them to the pending destruction list.
        // Conservartively set the fraemBuffer as used in the last frame.
        if((*resource).mFrameBuffer)
            destroyFrameBuffer(*(*resource).mFrameBuffer, getCurrentSubmissionIndex() - 1);

        (*resource).mFrameBuffer = mDevice.createFramebuffer(info);

		++resource;
		++outputBindings;
        graph.mFrameBuffersNeedUpdating[taskIndex] = false;
		++taskIndex;
	}
}


void RenderDevice::generateDescriptorSets(RenderGraph & graph)
{
    auto descriptorSets = mDescriptorManager.getDescriptors(graph, mVulkanResources);
    mDescriptorManager.writeDescriptors(descriptorSets, graph, mVulkanResources);
}


std::vector<BarrierRecorder> RenderDevice::recordBarriers(RenderGraph& graph)
{
    std::vector<BarrierRecorder> barriers{};

	auto taskIt = graph.taskBegin();

    for(uint32_t i = 0; i < graph.mInputResources.size(); ++i)
    {
        const auto& inputResources = graph.mInputResources[i];
        const auto& outputResources = graph.mOutputResources[i];

        BarrierRecorder recorder{this};

        for(const auto& resource : inputResources)
        {
            // For now only handle image layout transitions
            // Will handle visibility later (only needed when swapping between compute and graphics)
            if(resource.mResourcetype == RenderGraph::ResourceType::Image)
            {
                ImageView& imageView = graph.getImageView(resource.mResourceIndex);

				if(imageView.getImageUsage() & ImageUsage::ColourAttachment ||
				   imageView.getImageUsage() & ImageUsage::Storage)
				{
					// fetch the attachment type for the current image view and get what layout the image needs to be in.
					const vk::ImageLayout layout = getVulkanImageLayout((*taskIt).getInputAttachments()[resource.mResourceBinding].second);

					recorder.transitionImageLayout(imageView, layout);
				}
				else if(imageView.getImageUsage() & ImageUsage::DepthStencil)
					recorder.transitionImageLayout(imageView, vk::ImageLayout::eDepthStencilReadOnlyOptimal);
            }
        }

        for(const auto& resource : outputResources)
        {
            // For now only handle image layout transitions
            // Will handle visibility later (only needed when swapping between compute and graphics)
            if(resource.mResourcetype == RenderGraph::ResourceType::Image)
            {
                ImageView& imageView = graph.getImageView(resource.mResourceIndex);

				if(imageView.getImageUsage() & ImageUsage::ColourAttachment)
                    recorder.transitionImageLayout(imageView, vk::ImageLayout::eColorAttachmentOptimal);
				else if(imageView.getImageUsage() & ImageUsage::DepthStencil)
                    recorder.transitionImageLayout(imageView, vk::ImageLayout::eDepthStencilAttachmentOptimal);
            }
        }

        barriers.push_back(recorder);
		++taskIt;
    }

    return barriers;
}


vk::Fence RenderDevice::createFence(const bool signaled)
{
    vk::FenceCreateInfo info{};
    info.setFlags(signaled ? vk::FenceCreateFlagBits::eSignaled : static_cast<vk::FenceCreateFlags>(0));

    return mDevice.createFence(info);
}


vk::Sampler RenderDevice::getImmutableSampler(const Sampler& samplerDesc)
{
    vk::Sampler sampler = mImmutableSamplerCache[samplerDesc];

    if(sampler != vk::Sampler(nullptr))
        return sampler;

    const vk::Filter filterMode = [&samplerDesc]()
    {
        switch(samplerDesc.getSamplerType())
        {
            case SamplerType::Linear:
                return vk::Filter::eLinear;

            case SamplerType::Point:
                return vk::Filter::eNearest;

        }
    }();

    const vk::SamplerMipmapMode mipmapMode = filterMode == vk::Filter::eLinear ? vk::SamplerMipmapMode::eLinear :
                                                                                 vk::SamplerMipmapMode::eNearest;

    std::array<AddressMode, 3> addressModes = samplerDesc.getAddressModes();

    auto addressModeConversion = [](const AddressMode mode)
    {
        switch(mode)
        {
            case AddressMode::Clamp:
                return vk::SamplerAddressMode::eClampToEdge;

            case AddressMode::Mirror:
                return vk::SamplerAddressMode::eMirroredRepeat;

            case AddressMode::Repeat:
                return  vk::SamplerAddressMode::eRepeat;
        }
    };

    vk::SamplerCreateInfo info{};
    info.setMagFilter(filterMode);
    info.setMinFilter(filterMode);
    info.setAddressModeU(addressModeConversion(addressModes[0]));
    info.setAddressModeV(addressModeConversion(addressModes[1]));
    info.setAddressModeW (addressModeConversion(addressModes[2]));
    info.setAnisotropyEnable(samplerDesc.getNumberOfSamples() > 1);
    info.setMaxAnisotropy(samplerDesc.getNumberOfSamples());
    info.setBorderColor(vk::BorderColor::eIntOpaqueBlack);
    info.setCompareEnable(false);
    info.setCompareOp(vk::CompareOp::eNever);
    info.setMipmapMode(mipmapMode);
    info.setMipLodBias(0.0f);
    info.setMinLod(0.0f);
    info.setMaxLod(0.0f);

    sampler =  createSampler(info);
    mImmutableSamplerCache[samplerDesc] = sampler;

    return sampler;
}



void RenderDevice::execute(RenderGraph& graph)
{
	graph.reorderTasks();
	graph.mergeTasks();

    generateVulkanResources(graph);

    std::vector<BarrierRecorder> neededBarriers = recordBarriers(graph);

	CommandPool* currentCommandPool = getCurrentCommandPool();
    currentCommandPool->reserve(static_cast<uint32_t>(graph.taskCount()) + 1, QueueType::Graphics); // +1 for the primary cmd buffer all the secondaries will be recorded in to.

    vk::CommandBuffer primaryCmdBuffer = currentCommandPool->getBufferForQueue(QueueType::Graphics);

	uint32_t cmdBufferIndex = 1;
	auto vulkanResource = mVulkanResources.begin();
	for (auto task = graph.taskBegin(); task != graph.taskEnd(); ++task, ++vulkanResource)
	{
        const auto& resources = *vulkanResource;
		
		vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eCompute;

        execute(neededBarriers[cmdBufferIndex - 1]);

		vk::CommandBufferUsageFlags commadBufferUsage = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        if (resources.mRenderPass)
        {
            const auto& graphicsTask = static_cast<const GraphicsTask&>(*task);
            const auto pipelineDesc = graphicsTask.getPipelineDescription();
            vk::Rect2D renderArea{ {0, 0}, {pipelineDesc.mViewport.x, pipelineDesc.mViewport.y} };

            const std::vector<vk::ClearValue> clearValues = graphicsTask.getClearValues();

			commadBufferUsage |= vk::CommandBufferUsageFlagBits::eRenderPassContinue;

            vk::RenderPassBeginInfo passBegin{};
            passBegin.setRenderPass(*resources.mRenderPass);
            passBegin.setFramebuffer(*resources.mFrameBuffer);
            passBegin.setRenderArea(renderArea);
            if(!clearValues.empty())
            {
                passBegin.setClearValueCount(static_cast<uint32_t>(clearValues.size()));
                passBegin.setPClearValues(clearValues.data());
            }

            primaryCmdBuffer.beginRenderPass(passBegin, vk::SubpassContents::eSecondaryCommandBuffers);

            bindPoint = vk::PipelineBindPoint::eGraphics;
        }

        vk::CommandBufferInheritanceInfo secondaryInherit{};
        secondaryInherit.setRenderPass(*resources.mRenderPass);
        secondaryInherit.setFramebuffer(*resources.mFrameBuffer);

        vk::CommandBufferBeginInfo secondaryBegin{};
		secondaryBegin.setFlags(commadBufferUsage);
        secondaryBegin.setPInheritanceInfo(&secondaryInherit);

        vk::CommandBuffer& secondaryCmdBuffer = currentCommandPool->getBufferForQueue(QueueType::Graphics, cmdBufferIndex);
        secondaryCmdBuffer.begin(secondaryBegin);

        secondaryCmdBuffer.bindPipeline(bindPoint, resources.mPipeline->getHandle());

        if(graph.getVertexBuffer())
            secondaryCmdBuffer.bindVertexBuffers(0, { graph.getVertexBuffer()->getBuffer() }, {0});

        if(graph.getIndexBuffer())
            secondaryCmdBuffer.bindIndexBuffer(graph.getIndexBuffer()->getBuffer(), 0, vk::IndexType::eUint32);

        // Don't bind descriptor sets if we have no input attachments.
		if (resources.mDescSet != vk::DescriptorSet{ nullptr })
            secondaryCmdBuffer.bindDescriptorSets(bindPoint, resources.mPipeline->getLayoutHandle(), 0, 1,  &resources.mDescSet, 0, nullptr);

		(*task).recordCommands(secondaryCmdBuffer, graph, resources);

        secondaryCmdBuffer.end();
        primaryCmdBuffer.executeCommands(secondaryCmdBuffer);

		// end the render pass if we are executing a graphics task.
		if (resources.mRenderPass)
			primaryCmdBuffer.endRenderPass();

		++cmdBufferIndex;
    }

	primaryCmdBuffer.end();

    submitFrame();
}


void RenderDevice::startFrame()
{
    frameSyncSetup();
    getCurrentCommandPool()->reset();
    clearDeferredResources();
    ++mCurrentSubmission;
    ++mFinishedSubmission;
}


void RenderDevice::endFrame()
{
    mCurrentFrameIndex = (mCurrentFrameIndex + 1) %  getSwapChain()->getNumberOfSwapChainImages();
}


void RenderDevice::submitFrame()
{
    vk::SubmitInfo submitInfo{};
    submitInfo.setCommandBufferCount(1);
    submitInfo.setPCommandBuffers(&getCurrentCommandPool()->getBufferForQueue(QueueType::Graphics));
    submitInfo.setWaitSemaphoreCount(1);
    submitInfo.setPWaitSemaphores(&mImageAquired[getCurrentFrameIndex()]);
    submitInfo.setPSignalSemaphores(&mImageRendered[getCurrentFrameIndex()]);
    submitInfo.setSignalSemaphoreCount(1);
    auto const waitStage = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    submitInfo.setPWaitDstStageMask(&waitStage);

    mGraphicsQueue.submit(submitInfo, mFrameFinished[getCurrentFrameIndex()]);
}


void RenderDevice::swap()
{
    mSwapChain.present(mGraphicsQueue, mImageRendered[getCurrentFrameIndex()]);
}


void RenderDevice::frameSyncSetup()
{
    mSwapChain.getNextImageIndex(mImageAquired[getCurrentFrameIndex()]);

    // wait for the previous frame using this swapchain image to be finished.
    auto& fence = mFrameFinished[getCurrentFrameIndex()];
    mDevice.waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
    mDevice.resetFences(1, &fence);
}


void RenderDevice::execute(BarrierRecorder& recorder)
{
	for (uint8_t i = 0; i < static_cast<uint8_t>(QueueType::MaxQueues); ++i)
	{
		QueueType currentQueue = static_cast<QueueType>(i);

		if (!recorder.empty())
		{
			const auto imageBarriers = recorder.getImageBarriers(currentQueue);
			const auto bufferBarriers = recorder.getBufferBarriers(currentQueue);

            if(bufferBarriers.empty() && imageBarriers.empty())
                continue;

            getCurrentCommandPool()->getBufferForQueue(currentQueue).pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlagBits::eByRegion,
				0, nullptr,
                static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
                static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data());
		}
	}
}


void RenderDevice::clearDeferredResources()
{
    for(uint32_t i = 0; i < mFramebuffersPendingDestruction.size(); ++i)
    {
        auto& [submission, frameBuffer] = mFramebuffersPendingDestruction.front();

        if(submission <= mFinishedSubmission)
        {
            mDevice.destroyFramebuffer(frameBuffer);
            mFramebuffersPendingDestruction.pop_front();
        }
        else
            break;
    }

    for(uint32_t i = 0; i < mImagesPendingDestruction.size(); ++i)
    {
        auto& [submission, image, memory, imageView] = mImagesPendingDestruction.front();

        if(submission <= mFinishedSubmission)
        {
            if(imageView != vk::ImageView{nullptr})
                mDevice.destroyImageView(imageView);
            mDevice.destroyImage(image);
            getMemoryManager()->Free(memory);
            mImagesPendingDestruction.pop_front();
        }
        else
            break;
    }

    for(uint32_t i = 0; i < mBuffersPendingDestruction.size(); ++i)
    {
        auto& [submission, buffer, memory] = mBuffersPendingDestruction.front();

        if(submission <= mFinishedSubmission)
        {
            mDevice.destroyBuffer(buffer);
            getMemoryManager()->Free(memory);
            mBuffersPendingDestruction.pop_front();
        }
        else
            break;
    }
}


void RenderDevice::setDebugName(const std::string& name, const uint64_t handle, const vk::DebugReportObjectTypeEXT objectType)
{
	vk::DebugMarkerObjectNameInfoEXT markerInfo{};
	markerInfo.setObject(handle);
	markerInfo.setObjectType(objectType);
	markerInfo.setPObjectName(name.c_str());

	static auto* debugMarkerFunction = reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(mInstance.getProcAddr("vkDebugMarkerSetObjectNameEXT"));

	if(debugMarkerFunction != nullptr && mHasDebugLableSupport)
	{
		VkDebugMarkerObjectNameInfoEXT Info = markerInfo;
		Info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		debugMarkerFunction(mDevice, &Info);
	}
}



// Memory management functions
vk::PhysicalDeviceMemoryProperties RenderDevice::getMemoryProperties() const
{
    return mPhysicalDevice.getMemoryProperties();
}


vk::DeviceMemory    RenderDevice::allocateMemory(vk::MemoryAllocateInfo allocInfo)
{
    return mDevice.allocateMemory(allocInfo);
}


void    RenderDevice::freeMemory(const vk::DeviceMemory memory)
{
    mDevice.freeMemory(memory);
}


void*   RenderDevice::mapMemory(vk::DeviceMemory memory, vk::DeviceSize size, vk::DeviceSize offset)
{
    return mDevice.mapMemory(memory, offset, size);
}


void    RenderDevice::unmapMemory(vk::DeviceMemory memory)
{
    mDevice.unmapMemory(memory);
}


void    RenderDevice::bindBufferMemory(vk::Buffer& buffer, vk::DeviceMemory memory, uint64_t offset)
{
    mDevice.bindBufferMemory(buffer, memory, offset);
}


void    RenderDevice::bindImageMemory(vk::Image& image, vk::DeviceMemory memory, const uint64_t offset)
{
    mDevice.bindImageMemory(image, memory, offset);
}

