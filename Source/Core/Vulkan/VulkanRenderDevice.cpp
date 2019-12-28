#include "VulkanRenderDevice.hpp"
#include "Core/BellLogging.hpp"
#include "Core/ConversionUtils.hpp"
#include "VulkanBarrierManager.hpp"
#include "VulkanShader.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanExecutor.hpp"

#include "RenderGraph/RenderGraph.hpp"

#include <glslang/Public/ShaderLang.h>
#include <vulkan/vulkan.hpp>

#include <limits>
#include <memory>
#include <vector>

// Constants
constexpr uint32_t BINDLESS_ARRAY_SIZE = 16; // set to a super low limit for now.


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
	mDescriptorManager{this}
{
    // This is a bit of a hack to work around not being able to tell for the first few frames that
    // the fences we waited on hadn't been submitted (as no work has been done).
    mCurrentSubmission = mSwapChain->getNumberOfSwapChainImages();

    mQueueFamilyIndicies = getAvailableQueues(surface, mPhysicalDevice);
    mGraphicsQueue = mDevice.getQueue(static_cast<uint32_t>(mQueueFamilyIndicies.GraphicsQueueIndex), 0);
    mComputeQueue  = mDevice.getQueue(static_cast<uint32_t>(mQueueFamilyIndicies.ComputeQueueIndex), 0);
    mTransferQueue = mDevice.getQueue(static_cast<uint32_t>(mQueueFamilyIndicies.TransferQueueIndex), 0);

    mLimits = mPhysicalDevice.getProperties().limits;

	// Create a command pool for each frame.
    mCommandPools.reserve(mSwapChain->getNumberOfSwapChainImages());
	for (uint32_t i = 0; i < mSwapChain->getNumberOfSwapChainImages(); ++i)
	{
        mCommandPools.emplace_back(this);
	}

    mFrameFinished.reserve(mSwapChain->getNumberOfSwapChainImages());
    for (uint32_t i = 0; i < mSwapChain->getNumberOfSwapChainImages(); ++i)
    {
        mFrameFinished.push_back(createFence(true));
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

    // We can ignore lastUsed as we have just waited till all work has finished.
    for(auto& [lastUsed, handle, memory] : mImagesPendingDestruction)
    {
        mDevice.destroyImage(handle);
		mMemoryManager.Free(memory);
    }
    mImagesPendingDestruction.clear();

	for (const auto& [lastyUSed, view] : mImageViewsPendingDestruction)
	{
		mDevice.destroyImageView(view);
	}
	mImageViewsPendingDestruction.clear();

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
        mDevice.destroyDescriptorSetLayout(graphicsHandles.mDescriptorSetLayout[0]); // if there are any more they will get destroyed elsewhere.
    }

    for(auto& [desc, computeHandles]: mComputePipelineCache)
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


GraphicsPipelineHandles VulkanRenderDevice::createPipelineHandles(const GraphicsTask& task, const RenderGraph& graph)
{
    if(mGraphicsPipelineCache[task.getPipelineDescription()].mGraphicsPipeline)
        return mGraphicsPipelineCache[task.getPipelineDescription()];

    const vk::RenderPass renderPass = generateRenderPass(task);

	const std::vector<vk::DescriptorSetLayout> SRSLayouts = generateShaderResourceSetLayouts(task, graph);

    const auto pipeline = generatePipeline( task,
											SRSLayouts,
											renderPass);

    GraphicsPipelineHandles handles{pipeline, renderPass, SRSLayouts };
    mGraphicsPipelineCache[task.getPipelineDescription()] = handles;

    return handles;
}


ComputePipelineHandles VulkanRenderDevice::createPipelineHandles(const ComputeTask& task, const RenderGraph& graph)
{
	if(mComputePipelineCache[task.getPipelineDescription()].mComputePipeline)
        return mComputePipelineCache[task.getPipelineDescription()];

	const std::vector<vk::DescriptorSetLayout> SRSLayouts = generateShaderResourceSetLayouts(task, graph);

    const auto pipeline = generatePipeline(task, SRSLayouts);

    ComputePipelineHandles handles{pipeline, SRSLayouts };
    mComputePipelineCache[task.getPipelineDescription()] = handles;

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

		vk::AttachmentLoadOp op = [loadop](){
			switch(loadop)
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
vk::DescriptorSetLayout VulkanRenderDevice::generateDescriptorSetLayoutBindings(const std::vector<B>& bindings)
{
	std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{};
	layoutBindings.reserve(bindings.size());

	uint32_t currentBinding = 0;

	for (const auto& [unused, type] : bindings)
	{
		// Translate between Bell enums to the vulkan equivelent.
		vk::DescriptorType descriptorType = [type]()
		{
			switch (type)
			{
			case AttachmentType::Texture1D:
			case AttachmentType::Texture2D:
			case AttachmentType::Texture3D:
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

				// Ignore push constants for now.
			}

			return vk::DescriptorType::eCombinedImageSampler; // For now use this to indicate push_constants (terrible I know)
		}();

		// This indicates it's a push constant (or indirect buffer) which we don't need to handle when allocating descriptor
		// sets.
		if (descriptorType == vk::DescriptorType::eCombinedImageSampler)
			continue;

		vk::DescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.setBinding(currentBinding++);
		layoutBinding.setDescriptorType(descriptorType);
		layoutBinding.setDescriptorCount(type == AttachmentType::TextureArray ? BINDLESS_ARRAY_SIZE : 1);
		layoutBinding.setStageFlags(vk::ShaderStageFlagBits::eAll);

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

	return  generateDescriptorSetLayoutBindings(inputAttachments);
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


void VulkanRenderDevice::generateVulkanResources(RenderGraph& graph)
{
	auto task = graph.taskBegin();
	clearVulkanResources();
    mVulkanResources.resize(graph.mTaskOrder.size());
    auto resource = mVulkanResources.begin();

	while( task != graph.taskEnd())
    {
        if((*task).taskType() == TaskType::Graphics)
        {
            const GraphicsTask& graphicsTask = static_cast<const GraphicsTask&>(*task);
            GraphicsPipelineHandles pipelineHandles = createPipelineHandles(graphicsTask, graph);

            vulkanResources& taskResources = *resource;
            taskResources.mPipeline = pipelineHandles.mGraphicsPipeline;
            taskResources.mDescSetLayout = pipelineHandles.mDescriptorSetLayout;
            taskResources.mRenderPass = pipelineHandles.mRenderPass;
            // Frame buffers and descset get created/written in a diferent place.
        }
        else
        {
            const ComputeTask& computeTask = static_cast<const ComputeTask&>(*task);
            ComputePipelineHandles pipelineHandles = createPipelineHandles(computeTask, graph);

            vulkanResources& taskResources = *resource;
            taskResources.mPipeline = pipelineHandles.mComputePipeline;
            taskResources.mDescSetLayout = pipelineHandles.mDescriptorSetLayout;
			taskResources.mRenderPass.reset();
			taskResources.mFrameBuffer.reset();
        }
		++task;
		++resource;
    }
    generateFrameBuffers(graph);
    generateDescriptorSets(graph);
}


void VulkanRenderDevice::generateFrameBuffers(RenderGraph& graph)
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
        ImageExtent imageExtent;

		// Sort the bindings by location within the frameBuffer.
		// Resources aren't always bound in order so make sure that they are in order when we iterate over them.
		std::sort((*outputBindings).begin(), (*outputBindings).end(), [](const auto& lhs, const auto& rhs)
		{
			return lhs.mResourceBinding < rhs.mResourceBinding;
		});

        for(const auto& bindingInfo : *outputBindings)
        {
                const auto& imageView = graph.getImageView(bindingInfo.mResourceIndex);
                imageExtent = imageView->getImageExtent();
                imageViews.push_back(static_cast<const VulkanImageView&>(*imageView.getBase()).getImageView());
        }

        vk::FramebufferCreateInfo info{};
        info.setRenderPass(*(*resource).mRenderPass);
        info.setAttachmentCount(static_cast<uint32_t>(imageViews.size()));
        info.setPAttachments(imageViews.data());
        info.setWidth(imageExtent.width);
        info.setHeight(imageExtent.height);
        info.setLayers(imageExtent.depth);

		// Make sure we don't leak frameBuffers, add them to the pending destruction list.
		// Conservartively set the frameBuffer as used in the last frame.
        if((*resource).mFrameBuffer)
            destroyFrameBuffer(*(*resource).mFrameBuffer, getCurrentSubmissionIndex() - 1);

		(*resource).mFrameBuffer = mDevice.createFramebuffer(info);

		++resource;
		++outputBindings;
        graph.mFrameBuffersNeedUpdating[taskIndex] = false;
		++taskIndex;
	}
}


std::vector<vk::DescriptorSetLayout> VulkanRenderDevice::generateShaderResourceSetLayouts(const RenderTask& task, const RenderGraph& graph)
{
	std::vector<vk::DescriptorSetLayout> layouts{};
	layouts.push_back(generateDescriptorSetLayout(task));

	const auto& inputs = task.getInputAttachments();
	for (const auto& [slot, type] : inputs)
	{
		if (type == AttachmentType::ShaderResourceSet)
		{
			layouts.push_back(static_cast<const VulkanShaderResourceSet&>(*graph.getBoundShaderResourceSet(slot).getBase()).getLayout());
		}
	}

	return layouts;
}


void VulkanRenderDevice::generateDescriptorSets(RenderGraph & graph)
{
	mDescriptorManager.getDescriptors(graph, mVulkanResources);
	mDescriptorManager.writeDescriptors(graph, mVulkanResources);
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
    info.setMaxLod(0.0f);

    sampler =  createSampler(info);
    mImmutableSamplerCache[samplerDesc] = sampler;

    return sampler;
}



void VulkanRenderDevice::execute(RenderGraph& graph)
{
	graph.generateNonPersistentImages(this);
	graph.reorderTasks();
	graph.mergeTasks();

    generateVulkanResources(graph);

	std::vector<BarrierRecorder> neededBarriers = graph.generateBarriers(this);

	CommandPool* currentCommandPool = getCurrentCommandPool();
    currentCommandPool->reserve(static_cast<uint32_t>(graph.taskCount()) + 1, QueueType::Graphics); // +1 for the primary cmd buffer all the secondaries will be recorded in to.

    vk::CommandBuffer primaryCmdBuffer = currentCommandPool->getBufferForQueue(QueueType::Graphics);

#if SUBMISSION_PER_TASK
    bool firstTask = true;
#endif

	uint32_t cmdBufferIndex = 1;
	auto vulkanResource = mVulkanResources.begin();
	for (auto task = graph.taskBegin(); task != graph.taskEnd(); ++task, ++vulkanResource)
	{

#if SUBMISSION_PER_TASK
      if(!firstTask)
      {
          primaryCmdBuffer.reset(vk::CommandBufferResetFlags{});

          vk::CommandBufferBeginInfo primaryBegin{};
          primaryCmdBuffer.begin(primaryBegin);
      }
#endif

        const auto& resources = *vulkanResource;
		
		vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eCompute;

        execute(neededBarriers[cmdBufferIndex - 1]);

		vk::CommandBufferUsageFlags commadBufferUsage = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

		vk::CommandBufferInheritanceInfo secondaryInherit{};

        if (resources.mRenderPass)
        {
            const auto& graphicsTask = static_cast<const GraphicsTask&>(*task);
			const auto& pipelineDesc = graphicsTask.getPipelineDescription();
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

			secondaryInherit.setRenderPass(*resources.mRenderPass);
			secondaryInherit.setFramebuffer(*resources.mFrameBuffer);
        }

        vk::CommandBufferBeginInfo secondaryBegin{};
		secondaryBegin.setFlags(commadBufferUsage);
        secondaryBegin.setPInheritanceInfo(&secondaryInherit);

        vk::CommandBuffer& secondaryCmdBuffer = currentCommandPool->getBufferForQueue(QueueType::Graphics, cmdBufferIndex);
        secondaryCmdBuffer.begin(secondaryBegin);
        setDebugName((*task).getName(), reinterpret_cast<uint64_t>(VkCommandBuffer(secondaryCmdBuffer)), VK_OBJECT_TYPE_COMMAND_BUFFER);

        secondaryCmdBuffer.bindPipeline(bindPoint, resources.mPipeline->getHandle());

        if(graph.getIndexBuffer())
            secondaryCmdBuffer.bindIndexBuffer(static_cast<const VulkanBuffer&>(*graph.getIndexBuffer().value().getBase()).getBuffer(), 0, vk::IndexType::eUint32);

		secondaryCmdBuffer.bindDescriptorSets(bindPoint, resources.mPipeline->getLayoutHandle(), 0, resources.mDescSet.size(), resources.mDescSet.data(), 0, nullptr);

		{
			VulkanExecutor executor(secondaryCmdBuffer, resources);
			(*task).recordCommands(executor, graph);
		}

        secondaryCmdBuffer.end();
        primaryCmdBuffer.executeCommands(secondaryCmdBuffer);

		// end the render pass if we are executing a graphics task.
		if (resources.mRenderPass)
			primaryCmdBuffer.endRenderPass();

#if SUBMISSION_PER_TASK
        primaryCmdBuffer.end();

        submitFrame();

        flushWait();

        firstTask = false;
#endif

        ++cmdBufferIndex;
    }

	// Transition the frameBuffer to a presentable format (hehe).
	BarrierRecorder frameBufferTransition{this};
	auto& frameBufferView = getSwapChainImageView();
	frameBufferTransition->transitionLayout(frameBufferView, ImageLayout::Present, Hazard::ReadAfterWrite, SyncPoint::FragmentShaderOutput, SyncPoint::BottomOfPipe);

	execute(frameBufferTransition);

	primaryCmdBuffer.end();

    submitFrame();
}


void VulkanRenderDevice::startFrame()
{
    frameSyncSetup();
    getCurrentCommandPool()->reset();
    clearDeferredResources();
	mDescriptorManager.reset();
    ++mCurrentSubmission;
    ++mFinishedSubmission;
}


void VulkanRenderDevice::endFrame()
{
    mCurrentFrameIndex = (mCurrentFrameIndex + 1) %  getSwapChain()->getNumberOfSwapChainImages();
}


void VulkanRenderDevice::submitFrame()
{
	const VulkanSwapChain* swapChain = static_cast<VulkanSwapChain*>(mSwapChain);

    vk::SubmitInfo submitInfo{};
    submitInfo.setCommandBufferCount(1);
    submitInfo.setPCommandBuffers(&getCurrentCommandPool()->getBufferForQueue(QueueType::Graphics));
    submitInfo.setWaitSemaphoreCount(1);
    submitInfo.setPWaitSemaphores(&swapChain->getImageAquired());
    submitInfo.setPSignalSemaphores(&swapChain->getImageRendered());
    submitInfo.setSignalSemaphoreCount(1);
    auto const waitStage = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    submitInfo.setPWaitDstStageMask(&waitStage);

    mGraphicsQueue.submit(submitInfo, mFrameFinished[getCurrentFrameIndex()]);
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


void VulkanRenderDevice::execute(BarrierRecorder& recorder)
{
	for (uint8_t i = 0; i < static_cast<uint8_t>(QueueType::MaxQueues); ++i)
	{
		QueueType currentQueue = static_cast<QueueType>(i);

		VulkanBarrierRecorder& VKRecorder = static_cast<VulkanBarrierRecorder&>(*recorder.getBase());

		if (!VKRecorder.empty())
		{
			const auto imageBarriers = VKRecorder.getImageBarriers(currentQueue);
			const auto bufferBarriers = VKRecorder.getBufferBarriers(currentQueue);
            const auto memoryBarriers = VKRecorder.getMemoryBarriers(currentQueue);

            if(bufferBarriers.empty() && imageBarriers.empty())
                continue;

			getCurrentCommandPool()->getBufferForQueue(currentQueue).pipelineBarrier(getVulkanPipelineStage(VKRecorder.getSource()), getVulkanPipelineStage(VKRecorder.getDestination()),
				vk::DependencyFlagBits::eByRegion,
                static_cast<uint32_t>(memoryBarriers.size()), memoryBarriers.data(),
                static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
                static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data());
		}
	}
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
		const auto& [submission, view] = mImageViewsPendingDestruction.front();

		if (submission <= mFinishedSubmission)
		{
			mDevice.destroyImageView(view);
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
			mSRSPendingDestruction.pop_front();
		}
		else
			break;
	}
}


void VulkanRenderDevice::clearVulkanResources()
{
	for(auto& resources : mVulkanResources)
	{
		if(resources.mFrameBuffer)
			mFramebuffersPendingDestruction.emplace_back(getCurrentSubmissionIndex(), resources.mFrameBuffer.value());
		resources.mFrameBuffer.reset();

		resources.mDescSet.clear();
	}

	mVulkanResources.clear();
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
vk::DescriptorSetLayout VulkanRenderDevice::generateDescriptorSetLayoutBindings(const std::vector<ShaderResourceSetBase::ResourceInfo>&);
