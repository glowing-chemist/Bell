#include "VulkanRenderDevice.hpp"
#include "Core/BellLogging.hpp"
#include "Core/ConversionUtils.hpp"
#include "VulkanBarrierManager.hpp"
#include "VulkanShader.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanExecutor.hpp"
#include "VulkanCommandContext.hpp"

#include "RenderGraph/RenderGraph.hpp"

#include <glslang/Public/ShaderLang.h>
#include <vulkan/vulkan.hpp>

#include <limits>
#include <memory>
#include <vector>


VulkanRenderDevice::VulkanRenderDevice(vk::Instance instance,
						   vk::PhysicalDevice physDev,
						   vk::Device dev,
						   vk::SurfaceKHR surface,
						   GLFWwindow* window) :
	RenderDevice(),
    mDevice{dev},
    mPhysicalDevice{physDev},
	mInstance(instance),
	mSwapChainInitializer(this, surface, window, &mSwapChain),
    mMemoryManager{this},
    mPermanentDescriptorManager{this},
    mSubmissionCount{0},
    mFinishedTimeStamps{}
{
    // This is a bit of a hack to work around not being able to tell for the first few frames that
    // the fences we waited on hadn't been submitted (as no work has been done).
    mCurrentSubmission = mSwapChain->getNumberOfSwapChainImages();

    mQueueFamilyIndicies = getAvailableQueues(surface, mPhysicalDevice);
    mGraphicsQueue = mDevice.getQueue(static_cast<uint32_t>(mQueueFamilyIndicies.GraphicsQueueIndex), 0);
    mComputeQueue  = mDevice.getQueue(static_cast<uint32_t>(mQueueFamilyIndicies.ComputeQueueIndex), 0);
    mTransferQueue = mDevice.getQueue(static_cast<uint32_t>(mQueueFamilyIndicies.TransferQueueIndex), 0);

    mLimits = mPhysicalDevice.getProperties().limits;

    mFrameFinished.reserve(mSwapChain->getNumberOfSwapChainImages());
    mCommandContexts.resize(mSwapChain->getNumberOfSwapChainImages());
    for (uint32_t i = 0; i < mSwapChain->getNumberOfSwapChainImages(); ++i)
    {
        mFrameFinished.push_back(createFence(true));
        mCommandContexts[i].push_back(new VulkanCommandContext(this));
    }

    // Initialise the glsl compiler here rather than in the first compiled shader to avoid
    // having to synchronise there.
    const bool glslInitialised = glslang::InitializeProcess();
    BELL_ASSERT(glslInitialised, "FAILED TO INITIALISE GLSLANG, we will not be able to compile any shaders from source!")

#ifndef NDEBUG
    vk::EventCreateInfo eventInfo{};
    mDebugEvent = mDevice.createEvent(eventInfo);
#endif
}


VulkanRenderDevice::~VulkanRenderDevice()
{
	flushWait();

    // destroy the swapchain first so that is can add it's image views to the deferred destruction queue.
    mSwapChain->destroy();
	delete mSwapChain;

    for(auto& frameContexts : mCommandContexts)
    {
        for(auto* context : frameContexts)
            delete context;
    }

    // We can ignore lastUsed as we have just waited till all work has finished.
    for(auto& [lastUsed, handle, memory] : mImagesPendingDestruction)
    {
        mDevice.destroyImage(handle);
		mMemoryManager.Free(memory);
    }
    mImagesPendingDestruction.clear();

    for (const auto& [lastyUSed, view, cubeView] : mImageViewsPendingDestruction)
	{
		mDevice.destroyImageView(view);
        if(cubeView != vk::ImageView{nullptr})
            mDevice.destroyImageView(cubeView);
	}
	mImageViewsPendingDestruction.clear();

    for(auto& [lastUsed, handle, memory] : mBuffersPendingDestruction)
    {
        mDevice.destroyBuffer(handle);
        mMemoryManager.Free(memory);
    }
    mBuffersPendingDestruction.clear();

    for(auto& [imageViews, frameBuffer] : mFrameBufferCache)
    {
            mDevice.destroyFramebuffer(frameBuffer);
    }

	for(const auto& [lastUsed, frameBuffer] : mFramebuffersPendingDestruction)
    {
        mDevice.destroyFramebuffer(frameBuffer);
    }

    mMemoryManager.Destroy();

    for(auto& [_, graphicsHandles] : mGraphicsPipelineCache)
    {
        mDevice.destroyPipeline(graphicsHandles.mGraphicsPipeline->getHandle());
        mDevice.destroyPipelineLayout(graphicsHandles.mGraphicsPipeline->getLayoutHandle());
        mDevice.destroyRenderPass(graphicsHandles.mRenderPass);
        mDevice.destroyDescriptorSetLayout(graphicsHandles.mDescriptorSetLayout[0]); // if there are any more they will get destroyed elsewhere.
    }

    for(auto& [_, computeHandles]: mComputePipelineCache)
    {
        mDevice.destroyPipeline(computeHandles.mComputePipeline->getHandle());
        mDevice.destroyPipelineLayout(computeHandles.mComputePipeline->getLayoutHandle());
        mDevice.destroyDescriptorSetLayout(computeHandles.mDescriptorSetLayout[0]);
    }

	for (auto& [submission, pool, layout, set] : mSRSPendingDestruction)
	{
		mDevice.destroyDescriptorSetLayout(layout);
	}

    for(auto& fence : mFrameFinished)
    {
        mDevice.destroyFence(fence);
    }

    for(auto& [samplerType, sampler] : mImmutableSamplerCache)
    {
        mDevice.destroySampler(sampler);
    }

#ifndef NDEBUG
    mDevice.destroyEvent(mDebugEvent);
#endif
}


CommandContextBase* VulkanRenderDevice::getCommandContext(const uint32_t index)
{
    std::vector<CommandContextBase*>& frameContexts = mCommandContexts[mCurrentFrameIndex];
    if(frameContexts.size() <= index)
    {
        const uint32_t neededContexts = (frameContexts.size() - index) + 1;
        for(uint32_t i = 0; i < neededContexts; ++i)
        {
            frameContexts.push_back(new VulkanCommandContext(this));
        }
    }

    return frameContexts[index];
}


vk::Buffer  VulkanRenderDevice::createBuffer(const uint32_t size, const vk::BufferUsageFlags usage)
{
    vk::BufferCreateInfo createInfo{};
    createInfo.setSize(size);
    createInfo.setUsage(usage);

    return mDevice.createBuffer(createInfo);
}


vk::Queue&  VulkanRenderDevice::getQueue(const QueueType type)
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


uint32_t VulkanRenderDevice::getQueueFamilyIndex(const QueueType type) const
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


GraphicsPipelineHandles VulkanRenderDevice::createPipelineHandles(const GraphicsTask& task, const RenderGraph& graph, const uint64_t prefixHash)
{
    std::hash<GraphicsPipelineDescription> hasher{};
    uint64_t hash = hasher(task.getPipelineDescription());
    hash += prefixHash;

    if(mGraphicsPipelineCache[hash].mGraphicsPipeline)
        return mGraphicsPipelineCache[hash];

    const vk::RenderPass renderPass = generateRenderPass(task);

	const std::vector<vk::DescriptorSetLayout> SRSLayouts = generateShaderResourceSetLayouts(task, graph);

    const auto pipeline = generatePipeline( task,
											SRSLayouts,
											renderPass);

    GraphicsPipelineHandles handles{pipeline, renderPass, SRSLayouts };
    mGraphicsPipelineCache[hash] = handles;

    return handles;
}


ComputePipelineHandles VulkanRenderDevice::createPipelineHandles(const ComputeTask& task, const RenderGraph& graph, const uint64_t prefixHash)
{
    std::hash<ComputePipelineDescription> hasher{};
    uint64_t hash = hasher(task.getPipelineDescription());
    hash += prefixHash;

	if(mComputePipelineCache[hash].mComputePipeline)
        return mComputePipelineCache[hash];

	const std::vector<vk::DescriptorSetLayout> SRSLayouts = generateShaderResourceSetLayouts(task, graph);

    const auto pipeline = generatePipeline(task, SRSLayouts);

    ComputePipelineHandles handles{pipeline, SRSLayouts };
    mComputePipelineCache[hash] = handles;

    return handles;
}


std::shared_ptr<GraphicsPipeline> VulkanRenderDevice::generatePipeline(const GraphicsTask& task,
																 const std::vector< vk::DescriptorSetLayout>& descSetLayout,
																 const vk::RenderPass &renderPass)
{
    const GraphicsPipelineDescription& pipelineDesc = task.getPipelineDescription();
	const vk::PipelineLayout pipelineLayout = generatePipelineLayout(descSetLayout, task);

	std::shared_ptr<GraphicsPipeline> pipeline = std::make_shared<GraphicsPipeline>(this, pipelineDesc);
	pipeline->setLayout(pipelineLayout);
	pipeline->setDescriptorLayouts(descSetLayout);
	pipeline->setFrameBufferBlendStates(task);
	pipeline->setRenderPass(renderPass);
	pipeline->setVertexAttributes(task.getVertexAttributes());

	pipeline->compile(task);

	return pipeline;
}


std::shared_ptr<ComputePipeline> VulkanRenderDevice::generatePipeline(const ComputeTask& task,
																const std::vector< vk::DescriptorSetLayout>& descriptorSetLayout)
{
    const ComputePipelineDescription pipelineDesc = task.getPipelineDescription();
	const vk::PipelineLayout pipelineLayout = generatePipelineLayout(descriptorSetLayout, task);

	std::shared_ptr<ComputePipeline> pipeline = std::make_unique<ComputePipeline>(this, pipelineDesc);
	pipeline->setLayout(pipelineLayout);
	pipeline->compile(task);

	return pipeline;
}


vk::RenderPass	VulkanRenderDevice::generateRenderPass(const GraphicsTask& task)
{
    const auto& outputAttachments = task.getOuputAttachments();

    std::vector<vk::AttachmentDescription> attachmentDescriptions;

    // needed for subpass createInfo;
    bool hasDepthAttachment = false;
    std::vector<vk::AttachmentReference> outputAttachmentRefs{};
    std::vector<vk::AttachmentReference> depthAttachmentRef{};
    uint32_t outputAttatchmentCounter = 0;

	for(const auto& [name, type, format, size, loadop] : outputAttachments)
    {
        // We only care about images here.
        if(type == AttachmentType::DataBufferRO ||
           type == AttachmentType::DataBufferRW ||
           type == AttachmentType::DataBufferWO ||
           type == AttachmentType::PushConstants ||
           type == AttachmentType::UniformBuffer ||
           type == AttachmentType::ShaderResourceSet)
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
		attachmentDesc.setFinalLayout(layout);

		vk::AttachmentLoadOp op = [loadType = loadop](){
			switch(loadType)
			{
				case LoadOp::Preserve:
					return vk::AttachmentLoadOp::eLoad;
				case LoadOp::Nothing:
					return vk::AttachmentLoadOp::eDontCare;
				default:
					return vk::AttachmentLoadOp::eClear;
			}
		}();

		attachmentDesc.setLoadOp(op);

        attachmentDesc.setStoreOp(vk::AttachmentStoreOp::eStore);
		attachmentDesc.setStencilLoadOp(op);
        attachmentDesc.setStencilStoreOp(vk::AttachmentStoreOp::eStore);
        attachmentDesc.setSamples(vk::SampleCountFlagBits::e1); // TODO respect multisample images.

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


std::vector<vk::PipelineShaderStageCreateInfo> VulkanRenderDevice::generateShaderStagesInfo(GraphicsPipelineDescription& pipelineDesc)
{
	// Make sure that all shaders have been compiled by this point.
	if (!pipelineDesc.mVertexShader->hasBeenCompiled())
	{
		pipelineDesc.mVertexShader->compile();
		pipelineDesc.mFragmentShader->compile();
	}

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

    vk::PipelineShaderStageCreateInfo vertexStage{};
    vertexStage.setStage(vk::ShaderStageFlagBits::eVertex);
    vertexStage.setPName("main"); //entry point of the shader
    vertexStage.setModule(static_cast<const VulkanShader&>(*pipelineDesc.mVertexShader.getBase()).getShaderModule());
    shaderStages.push_back(vertexStage);

    if(pipelineDesc.mGeometryShader)
    {
		if (!pipelineDesc.mGeometryShader.value()->hasBeenCompiled())
			pipelineDesc.mGeometryShader.value()->compile();

        vk::PipelineShaderStageCreateInfo geometryStage{};
        geometryStage.setStage(vk::ShaderStageFlagBits::eGeometry);
        geometryStage.setPName("main"); //entry point of the shader
        geometryStage.setModule(static_cast<const VulkanShader&>(*pipelineDesc.mGeometryShader.value().getBase()).getShaderModule());
        shaderStages.push_back(geometryStage);
    }

    if(pipelineDesc.mHullShader)
    {
		if (!pipelineDesc.mHullShader.value()->hasBeenCompiled())
			pipelineDesc.mHullShader.value()->compile();

        vk::PipelineShaderStageCreateInfo hullStage{};
        hullStage.setStage(vk::ShaderStageFlagBits::eTessellationControl);
        hullStage.setPName("main"); //entry point of the shader
        hullStage.setModule(static_cast<const VulkanShader&>(*pipelineDesc.mHullShader.value().getBase()).getShaderModule());
        shaderStages.push_back(hullStage);
    }

    if(pipelineDesc.mTesselationControlShader)
    {
		if (!pipelineDesc.mTesselationControlShader.value()->hasBeenCompiled())
			pipelineDesc.mTesselationControlShader.value()->compile();

        vk::PipelineShaderStageCreateInfo tesseStage{};
        tesseStage.setStage(vk::ShaderStageFlagBits::eTessellationEvaluation);
        tesseStage.setPName("main"); //entry point of the shader
        tesseStage.setModule(static_cast<const VulkanShader&>(*pipelineDesc.mTesselationControlShader.value().getBase()).getShaderModule());
        shaderStages.push_back(tesseStage);
    }

    vk::PipelineShaderStageCreateInfo fragmentStage{};
    fragmentStage.setStage(vk::ShaderStageFlagBits::eFragment);
    fragmentStage.setPName("main"); //entry point of the shader
    fragmentStage.setModule(static_cast<const VulkanShader&>(*pipelineDesc.mFragmentShader.getBase()).getShaderModule());
    shaderStages.push_back(fragmentStage);

    return shaderStages;
}


std::vector<vk::PipelineShaderStageCreateInfo> VulkanRenderDevice::generateIndexedShaderStagesInfo(GraphicsPipelineDescription& pipelineDesc)
{
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

	vk::PipelineShaderStageCreateInfo vertexStage{};
	vertexStage.setStage(vk::ShaderStageFlagBits::eVertex);
	vertexStage.setPName("main"); //entry point of the shader
	vertexStage.setModule(static_cast<const VulkanShader&>(*pipelineDesc.mInstancedVertexShader.value().getBase()).getShaderModule());
	shaderStages.push_back(vertexStage);

	if (pipelineDesc.mGeometryShader)
	{
		if (!pipelineDesc.mGeometryShader.value()->hasBeenCompiled())
			pipelineDesc.mGeometryShader.value()->compile();

		vk::PipelineShaderStageCreateInfo geometryStage{};
		geometryStage.setStage(vk::ShaderStageFlagBits::eGeometry);
		geometryStage.setPName("main"); //entry point of the shader
		geometryStage.setModule(static_cast<const VulkanShader&>(*pipelineDesc.mGeometryShader.value().getBase()).getShaderModule());
		shaderStages.push_back(geometryStage);
	}

	if (pipelineDesc.mHullShader)
	{
		if (!pipelineDesc.mHullShader.value()->hasBeenCompiled())
			pipelineDesc.mHullShader.value()->compile();

		vk::PipelineShaderStageCreateInfo hullStage{};
		hullStage.setStage(vk::ShaderStageFlagBits::eTessellationControl);
		hullStage.setPName("main"); //entry point of the shader
		hullStage.setModule(static_cast<const VulkanShader&>(*pipelineDesc.mHullShader.value().getBase()).getShaderModule());
		shaderStages.push_back(hullStage);
	}

	if (pipelineDesc.mTesselationControlShader)
	{
		if (!pipelineDesc.mTesselationControlShader.value()->hasBeenCompiled())
			pipelineDesc.mTesselationControlShader.value()->compile();

		vk::PipelineShaderStageCreateInfo tesseStage{};
		tesseStage.setStage(vk::ShaderStageFlagBits::eTessellationEvaluation);
		tesseStage.setPName("main"); //entry point of the shader
		tesseStage.setModule(static_cast<const VulkanShader&>(*pipelineDesc.mTesselationControlShader.value().getBase()).getShaderModule());
		shaderStages.push_back(tesseStage);
	}

	vk::PipelineShaderStageCreateInfo fragmentStage{};
	fragmentStage.setStage(vk::ShaderStageFlagBits::eFragment);
	fragmentStage.setPName("main"); //entry point of the shader
	fragmentStage.setModule(static_cast<const VulkanShader&>(*pipelineDesc.mFragmentShader.getBase()).getShaderModule());
	shaderStages.push_back(fragmentStage);

	return shaderStages;
}


template<typename B>
vk::DescriptorSetLayout VulkanRenderDevice::generateDescriptorSetLayoutBindings(const std::vector<B>& bindings, const TaskType taskType)
{
	std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{};
	layoutBindings.reserve(bindings.size());

	uint32_t currentBinding = 0;

    for (const auto& [_, type, arraySize] : bindings)
	{
		// Translate between Bell enums to the vulkan equivelent.
		vk::DescriptorType descriptorType = [attachmentType = type]()
		{
			switch (attachmentType)
			{
			case AttachmentType::Texture1D:
			case AttachmentType::Texture2D:
			case AttachmentType::Texture3D:
            case AttachmentType::CubeMap:
			case AttachmentType::TextureArray:
				return vk::DescriptorType::eSampledImage;

			case AttachmentType::Image1D:
			case AttachmentType::Image2D:
			case AttachmentType::Image3D:
				return vk::DescriptorType::eStorageImage;

            case AttachmentType::DataBufferRO:
            case AttachmentType::DataBufferRW:
            case AttachmentType::DataBufferWO:
				return vk::DescriptorType::eStorageBuffer;

			case AttachmentType::UniformBuffer:
				return vk::DescriptorType::eUniformBuffer;

			case AttachmentType::Sampler:
				return vk::DescriptorType::eSampler;

            default:
                return vk::DescriptorType::eCombinedImageSampler;// For now use this to indicate push_constants (terrible I know)

			}
		}();

		// This indicates it's a push constant (or indirect buffer) which we don't need to handle when allocating descriptor
		// sets.
		if (descriptorType == vk::DescriptorType::eCombinedImageSampler)
			continue;

		vk::DescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.setBinding(currentBinding++);
		layoutBinding.setDescriptorType(descriptorType);
        layoutBinding.setDescriptorCount(type == AttachmentType::TextureArray ? arraySize : 1);
		
		const vk::ShaderStageFlags stages = [&]()
		{
			switch (taskType)
			{
			case TaskType::Graphics:
				return vk::ShaderStageFlagBits::eAllGraphics;
			
			case TaskType::Compute:
				return vk::ShaderStageFlagBits::eCompute;

			case TaskType::All:
				return vk::ShaderStageFlagBits::eAll;
			}
		}();
		layoutBinding.setStageFlags(stages);

		layoutBindings.push_back(layoutBinding);
	}

	vk::DescriptorSetLayoutCreateInfo descSetLayoutInfo{};
	descSetLayoutInfo.setPBindings(layoutBindings.data());
	descSetLayoutInfo.setBindingCount(static_cast<uint32_t>(layoutBindings.size()));

	return  mDevice.createDescriptorSetLayout(descSetLayoutInfo);
}


vk::DescriptorSetLayout VulkanRenderDevice::generateDescriptorSetLayout(const RenderTask& task)
{
    const auto& inputAttachments = task.getInputAttachments();

	return  generateDescriptorSetLayoutBindings(inputAttachments, task.taskType());
}


vk::PipelineLayout VulkanRenderDevice::generatePipelineLayout(const std::vector< vk::DescriptorSetLayout>& descLayout, const RenderTask& task)
{
    vk::PipelineLayoutCreateInfo info{};
    info.setSetLayoutCount(descLayout.size());
    info.setPSetLayouts(descLayout.data());

	const auto& inputAttachments = task.getInputAttachments();
	const bool hasPushConstants =  std::find_if(inputAttachments.begin(), inputAttachments.end(), [](const auto& attachment)
	{
		return attachment.mType == AttachmentType::PushConstants;
	}) != inputAttachments.end();

	// at least for now just use a mat4 for push constants.
	vk::PushConstantRange range{};
    range.setSize(128);
	range.setOffset(0);
	range.setStageFlags(vk::ShaderStageFlagBits::eAll);

	if(hasPushConstants)
	{
		info.setPPushConstantRanges(&range);
		info.setPushConstantRangeCount(1);
	}

    return mDevice.createPipelineLayout(info);
}


vulkanResources VulkanRenderDevice::getTaskResources(const RenderGraph& graph, const uint32_t taskIndex, const uint64_t prefixHash)
{
    const RenderTask& task = graph.getTask(taskIndex);

    uint64_t resourceHash = prefixHash;
    if(task.taskType() == TaskType::Graphics)
    {
        const GraphicsTask& gTask = static_cast<const GraphicsTask&>(task);
        std::hash<GraphicsPipelineDescription> hasher;
        resourceHash += hasher(gTask.getPipelineDescription());
    }
    else
    {
        const ComputeTask& cTask = static_cast<const ComputeTask&>(task);
        std::hash<ComputePipelineDescription> hasher;
        resourceHash += hasher(cTask.getPipelineDescription());
    }

    mResourcesLock.lock_shared();

    if(mVulkanResources.find(resourceHash) != mVulkanResources.end())
    {
        vulkanResources& resources = mVulkanResources[resourceHash];
        mResourcesLock.unlock_shared();

        return resources;
    }
    else
    {
        mResourcesLock.unlock_shared();
        mResourcesLock.lock(); // need to write to the resource map so need exclusive access.

        vulkanResources resources = generateVulkanResources(graph, taskIndex, prefixHash);

        mResourcesLock.unlock();

        return resources;
    }
}


vulkanResources VulkanRenderDevice::generateVulkanResources(const RenderGraph& graph, const uint32_t taskIndex, const uint64_t prefixHash)
{
    const RenderTask& task = graph.getTask(taskIndex);
    vulkanResources resources{};

    if(task.taskType() == TaskType::Graphics)
    {
        const GraphicsTask& graphicsTask = static_cast<const GraphicsTask&>(task);
        GraphicsPipelineHandles pipelineHandles = createPipelineHandles(graphicsTask, graph, prefixHash);

        resources.mPipeline = pipelineHandles.mGraphicsPipeline;
        resources.mDescSetLayout = pipelineHandles.mDescriptorSetLayout;
        resources.mRenderPass = pipelineHandles.mRenderPass;
        // Frame buffers and descset get created/written in a diferent place.
    }
    else
    {
        const ComputeTask& computeTask = static_cast<const ComputeTask&>(task);
        ComputePipelineHandles pipelineHandles = createPipelineHandles(computeTask, graph, prefixHash);

        resources.mPipeline = pipelineHandles.mComputePipeline;
        resources.mDescSetLayout = pipelineHandles.mDescriptorSetLayout;
    }

    return resources;
}


vk::Framebuffer VulkanRenderDevice::createFrameBuffer(const RenderGraph& graph, const uint32_t taskIndex, const vk::RenderPass renderPass)
{
    const auto& outputBindings = graph.getTask(taskIndex).getOuputAttachments();

    std::vector<vk::ImageView> imageViews{};
    ImageExtent imageExtent;

    for(const auto& bindingInfo : outputBindings)
    {
            const auto& imageView = graph.getImageView(bindingInfo.mName);
            imageExtent = imageView->getImageExtent();
            imageViews.push_back(static_cast<const VulkanImageView&>(*imageView.getBase()).getImageView());
    }

    if(mFrameBufferCache.find(imageViews) != mFrameBufferCache.end())
    {
        return mFrameBufferCache[imageViews];
    }

    vk::FramebufferCreateInfo info{};
    info.setRenderPass(renderPass);
    info.setAttachmentCount(static_cast<uint32_t>(imageViews.size()));
    info.setPAttachments(imageViews.data());
    info.setWidth(imageExtent.width);
    info.setHeight(imageExtent.height);
    info.setLayers(imageExtent.depth);

    vk::Framebuffer newFrameBuffer =  mDevice.createFramebuffer(info);

    mFrameBufferCache.insert({imageViews, newFrameBuffer});

    return newFrameBuffer;
}


std::vector<vk::DescriptorSetLayout> VulkanRenderDevice::generateShaderResourceSetLayouts(const RenderTask& task, const RenderGraph& graph)
{
	std::vector<vk::DescriptorSetLayout> layouts{};
	layouts.push_back(generateDescriptorSetLayout(task));

	const auto& inputs = task.getInputAttachments();
    for (const auto& [slot, type, _] : inputs)
	{
		if (type == AttachmentType::ShaderResourceSet)
		{
			layouts.push_back(static_cast<const VulkanShaderResourceSet&>(*graph.getBoundShaderResourceSet(slot).getBase()).getLayout());
		}
	}

	return layouts;
}


vk::Fence VulkanRenderDevice::createFence(const bool signaled)
{
    vk::FenceCreateInfo info{};
    info.setFlags(signaled ? vk::FenceCreateFlagBits::eSignaled : static_cast<vk::FenceCreateFlags>(0));

    return mDevice.createFence(info);
}


vk::Sampler VulkanRenderDevice::getImmutableSampler(const Sampler& samplerDesc)
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
    info.setMaxLod(16.0f);

    sampler =  createSampler(info);
    mImmutableSamplerCache[samplerDesc] = sampler;

    return sampler;
}


void VulkanRenderDevice::startFrame()
{
    frameSyncSetup();
    // update timestampts.
    mFinishedTimeStamps.clear();
    for(CommandContextBase* context : mCommandContexts[mCurrentFrameIndex])
    {
        const std::vector<uint64_t>& timeStamps = context->getTimestamps();
        uint32_t i = 0;
        while(i < timeStamps.size())
        {
            const uint64_t start = timeStamps[i++];
            const uint64_t end = timeStamps[i++];

            const uint64_t duration = end - start;
            mFinishedTimeStamps.push_back(duration);
        }
        context->reset();
    }
    clearDeferredResources();
    mPermanentDescriptorManager.reset();
    ++mCurrentSubmission;
    ++mFinishedSubmission;
    mSubmissionCount = 0;
}


void VulkanRenderDevice::endFrame()
{
    mCurrentFrameIndex = (mCurrentFrameIndex + 1) %  getSwapChain()->getNumberOfSwapChainImages();
}


void VulkanRenderDevice::submitContext(CommandContextBase *context, const bool finalSubmission)
{
    vk::CommandBuffer primaryCmdBuffer = static_cast<VulkanCommandContext*>(context)->getPrefixCommandBuffer();
    primaryCmdBuffer.end();

    const VulkanSwapChain* swapChain = static_cast<VulkanSwapChain*>(mSwapChain);

    vk::SubmitInfo submitInfo{};
    submitInfo.setCommandBufferCount(1);
    submitInfo.setPCommandBuffers(&primaryCmdBuffer);
    submitInfo.setPWaitSemaphores(mSubmissionCount == 0 ? swapChain->getImageAquired() : nullptr);
    submitInfo.setWaitSemaphoreCount(mSubmissionCount == 0 ? 1 : 0);
    submitInfo.setPSignalSemaphores(finalSubmission ? swapChain->getImageRendered() : nullptr);
    submitInfo.setSignalSemaphoreCount(finalSubmission ? 1 : 0);
    auto const waitStage = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    submitInfo.setPWaitDstStageMask(&waitStage);

    mGraphicsQueue.submit(submitInfo, finalSubmission ? mFrameFinished[getCurrentFrameIndex()] : vk::Fence{nullptr});

    ++mSubmissionCount;
}


void VulkanRenderDevice::swap()
{
    mSwapChain->present(QueueType::Graphics);
}


void VulkanRenderDevice::frameSyncSetup()
{
	mSwapChain->getNextImageIndex();

    // wait for the previous frame using this swapchain image to be finished.
    auto& fence = mFrameFinished[getCurrentFrameIndex()];
    mDevice.waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
    mDevice.resetFences(1, &fence);
}


void VulkanRenderDevice::clearDeferredResources()
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
        const auto& [submission, image, memory] = mImagesPendingDestruction.front();

        if(submission <= mFinishedSubmission)
        {
            destroyImage(image);
            getMemoryManager()->Free(memory);
            mImagesPendingDestruction.pop_front();
        }
        else
            break;
    }

	for (uint32_t i = 0; i < mImageViewsPendingDestruction.size(); ++i)
	{
        const auto& [submission, view, cubeView] = mImageViewsPendingDestruction.front();

		if (submission <= mFinishedSubmission)
		{
			mDevice.destroyImageView(view);
            if(cubeView != vk::ImageView{nullptr})
                mDevice.destroyImageView(cubeView);
			mImageViewsPendingDestruction.pop_front();
		}
		else
			break;
	}

    for(uint32_t i = 0; i < mBuffersPendingDestruction.size(); ++i)
    {
        const auto& [submission, buffer, memory] = mBuffersPendingDestruction.front();

        if(submission <= mFinishedSubmission)
        {
            destroyBuffer(buffer);
            getMemoryManager()->Free(memory);
            mBuffersPendingDestruction.pop_front();
        }
        else
            break;
    }

	for (uint32_t i = 0; i < mSRSPendingDestruction.size(); ++i)
	{
		auto& [submission, pool, layout, set] = mSRSPendingDestruction.front();

		if (submission <= mFinishedSubmission)
		{
			mDevice.destroyDescriptorSetLayout(layout);
            mDevice.freeDescriptorSets(pool, 1, &set);
			mSRSPendingDestruction.pop_front();
		}
		else
			break;
	}
}


void VulkanRenderDevice::setDebugName(const std::string& name, const uint64_t handle, const uint64_t objectType)
{
    VkDebugUtilsObjectNameInfoEXT lableInfo{};
    lableInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    lableInfo.objectType = static_cast<VkObjectType>(objectType);
    lableInfo.objectHandle = handle;
    lableInfo.pObjectName = name.c_str();

    static auto* debugMarkerFunction = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(mInstance.getProcAddr("vkSetDebugUtilsObjectNameEXT"));

    if(debugMarkerFunction != nullptr)
    {

        debugMarkerFunction(mDevice, &lableInfo);
    }
}


void VulkanRenderDevice::invalidatePipelines()
{
    mDevice.waitIdle();

    for(auto& [_, graphicsHandles] : mGraphicsPipelineCache)
    {
        mDevice.destroyPipeline(graphicsHandles.mGraphicsPipeline->getHandle());
        mDevice.destroyPipelineLayout(graphicsHandles.mGraphicsPipeline->getLayoutHandle());
        mDevice.destroyRenderPass(graphicsHandles.mRenderPass);
        mDevice.destroyDescriptorSetLayout(graphicsHandles.mDescriptorSetLayout[0]); // if there are any more they will get destroyed elsewhere.
    }
    mGraphicsPipelineCache.clear();

    for(auto& [_, computeHandles]: mComputePipelineCache)
    {
        mDevice.destroyPipeline(computeHandles.mComputePipeline->getHandle());
        mDevice.destroyPipelineLayout(computeHandles.mComputePipeline->getLayoutHandle());
        mDevice.destroyDescriptorSetLayout(computeHandles.mDescriptorSetLayout[0]);
    }
    mComputePipelineCache.clear();

    for(auto& [imageViews, frameBuffer] : mFrameBufferCache)
    {
        destroyFrameBuffer(frameBuffer, mCurrentSubmission);
    }

    mVulkanResources.clear();
}


vk::CommandBuffer VulkanRenderDevice::getPrefixCommandBuffer()
{
    VulkanCommandContext* VKCmdContext = static_cast<VulkanCommandContext*>(getCommandContext(0));
    return VKCmdContext->getPrefixCommandBuffer();
}


// Memory management functions
vk::PhysicalDeviceMemoryProperties VulkanRenderDevice::getMemoryProperties() const
{
    return mPhysicalDevice.getMemoryProperties();
}


vk::DeviceMemory    VulkanRenderDevice::allocateMemory(vk::MemoryAllocateInfo allocInfo)
{
    return mDevice.allocateMemory(allocInfo);
}


void    VulkanRenderDevice::freeMemory(const vk::DeviceMemory memory)
{
    mDevice.freeMemory(memory);
}


void*   VulkanRenderDevice::mapMemory(vk::DeviceMemory memory, vk::DeviceSize size, vk::DeviceSize offset)
{
    return mDevice.mapMemory(memory, offset, size);
}


void    VulkanRenderDevice::unmapMemory(vk::DeviceMemory memory)
{
    mDevice.unmapMemory(memory);
}


void    VulkanRenderDevice::bindBufferMemory(vk::Buffer& buffer, vk::DeviceMemory memory, uint64_t offset)
{
    mDevice.bindBufferMemory(buffer, memory, offset);
}


void    VulkanRenderDevice::bindImageMemory(vk::Image& image, vk::DeviceMemory memory, const uint64_t offset)
{
    mDevice.bindImageMemory(image, memory, offset);
}


template
vk::DescriptorSetLayout VulkanRenderDevice::generateDescriptorSetLayoutBindings(const std::vector<ShaderResourceSetBase::ResourceInfo>&, const TaskType);
