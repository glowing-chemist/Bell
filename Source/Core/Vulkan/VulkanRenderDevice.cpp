#include "VulkanRenderDevice.hpp"
#include "Core/BellLogging.hpp"
#include "Core/ConversionUtils.hpp"
#include "VulkanBarrierManager.hpp"
#include "VulkanShader.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanExecutor.hpp"
#include "VulkanCommandContext.hpp"
#include "Core/ContainerUtils.hpp"
#include "Core/Profiling.hpp"

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
						   GLFWwindow* window,
						   const uint32_t deviceFeatures) :
	RenderDevice(deviceFeatures),
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

#if PROFILE
    mProfileDeviceHandle = mDevice;
    mProfilePhysicalDeviceHandle = mPhysicalDevice;
    mProfileGraphicsQueue = mGraphicsQueue;
#endif

    uint32_t graphicsQueueIndex = mQueueFamilyIndicies.GraphicsQueueIndex;
    PROFILER_INIT_VULKAN(&mProfileDeviceHandle, &mProfilePhysicalDeviceHandle, &mProfileGraphicsQueue, &graphicsQueueIndex, 1);

    mLimits = mPhysicalDevice.getProperties().limits;

    mFrameFinished.reserve(mSwapChain->getNumberOfSwapChainImages());
    mGraphicsCommandContexts.resize(mSwapChain->getNumberOfSwapChainImages());
    mAsyncComputeCommandContexts.resize(mSwapChain->getNumberOfSwapChainImages());
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

    // Check for conditional rendering support.
    vk::PhysicalDeviceConditionalRenderingFeaturesEXT conditionalRenderingInfo{};
    vk::PhysicalDeviceFeatures2 features2{};
    features2.pNext = &conditionalRenderingInfo;
    mPhysicalDevice.getFeatures2(&features2);
    if(conditionalRenderingInfo.conditionalRendering == true)
        mHasConditionalRenderingSupport = true;

    // Create semaphores for synchronising with ansyn compute queue.
    if(getHasAsyncComputeSupport())
    {
        for(uint32_t i = 0; i < mSwapChain->getNumberOfSwapChainImages(); ++i)
        {
            vk::SemaphoreTypeCreateInfo timelineCreateInfo{};
            timelineCreateInfo.semaphoreType = vk::SemaphoreType::eTimeline;
            timelineCreateInfo.initialValue = 0;

            vk::SemaphoreCreateInfo createInfo{};
            createInfo.pNext = &timelineCreateInfo;

            mAsyncQueueSemaphores.push_back(mDevice.createSemaphore(createInfo));
        }
    }
}


VulkanRenderDevice::~VulkanRenderDevice()
{
	flushWait();

    // destroy the swapchain first so that is can add it's image views to the deferred destruction queue.
    mSwapChain->destroy();
	delete mSwapChain;

    for(auto& frameContexts : mGraphicsCommandContexts)
    {
        for(auto* context : frameContexts)
            delete context;
    }

    for (auto& frameContexts : mAsyncComputeCommandContexts)
    {
        for (auto* context : frameContexts)
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
    mFrameBufferCache.clear();

	for(const auto& [lastUsed, frameBuffer] : mFramebuffersPendingDestruction)
    {
        mDevice.destroyFramebuffer(frameBuffer);
    }

    mMemoryManager.Destroy();

    for(auto& [hash, handles] : mVulkanResources)
    {
        mDevice.destroyDescriptorSetLayout(handles.mDescSetLayout[0]); // if there are any more they will get destroyed elsewhere.
        mDevice.destroyPipelineLayout(handles.mPipelineTemplate->getLayoutHandle());
        handles.mPipelineTemplate->invalidatePipelineCache();
        if(handles.mRenderPass)
            mDevice.destroyRenderPass(*handles.mRenderPass);

    }
    mVulkanResources.clear();

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


CommandContextBase* VulkanRenderDevice::getCommandContext(const uint32_t index, const QueueType queue)
{
    std::vector<CommandContextBase*>& frameContexts = queue == QueueType::Compute ? mAsyncComputeCommandContexts[mCurrentFrameIndex] : mGraphicsCommandContexts[mCurrentFrameIndex];
    if(frameContexts.size() <= index)
    {
        const uint32_t neededContexts = (index - frameContexts.size()) + 1;
        for(uint32_t i = 0; i < neededContexts; ++i)
        {
            frameContexts.push_back(new VulkanCommandContext(this, queue));
        }
    }

    return frameContexts[index];
}


PipelineHandle VulkanRenderDevice::compileGraphicsPipeline(const GraphicsTask& task,
                                                           const RenderGraph& graph,
                                                           const Shader& vertexShader,
                                                           const Shader* geometryShader,
                                                           const Shader* tessControl,
                                                           const Shader* tessEval,
                                                           const Shader& fragmentShader)
{
    BELL_ASSERT(vertexShader->getPrefixHash() == fragmentShader->getPrefixHash(), "Shaders compiled with different prefix hashes")

    vulkanResources handles = getTaskResources(graph, task, vertexShader->getPrefixHash());

    std::shared_ptr<Pipeline> pipeline = handles.mPipelineTemplate->instanciateGraphicsPipeline(task, vertexShader->getPrefixHash() ^ vertexShader->getCompiledDefinesHash() ^ fragmentShader->getCompiledDefinesHash(), *handles.mRenderPass, vertexShader, geometryShader, tessControl, tessEval, fragmentShader);

    return reinterpret_cast<uint64_t>(VkPipeline(pipeline->getHandle()));
}

PipelineHandle VulkanRenderDevice::compileComputePipeline(const ComputeTask& task,
                                                          const RenderGraph& graph,
                                                          const Shader& computeShader)
{
    vulkanResources handles = getTaskResources(graph, task, computeShader->getPrefixHash());

    std::shared_ptr<Pipeline> pipeline = handles.mPipelineTemplate->instanciateComputePipeline(task, computeShader->getPrefixHash() ^ computeShader->getCompiledDefinesHash(), computeShader);

    return reinterpret_cast<uint64_t>(VkPipeline(pipeline->getHandle()));
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
    PROFILER_EVENT();

    const vk::RenderPass renderPass = generateRenderPass(task);

	const std::vector<vk::DescriptorSetLayout> SRSLayouts = generateShaderResourceSetLayouts(task, graph);
    const vk::PipelineLayout layout = generatePipelineLayout(SRSLayouts, task);

    GraphicsPipelineDescription desc = task.getPipelineDescription();
    GraphicsPipelineHandles handles{ std::make_shared<PipelineTemplate>(this, &desc, layout), renderPass, SRSLayouts };

    return handles;
}


ComputePipelineHandles VulkanRenderDevice::createPipelineHandles(const ComputeTask& task, const RenderGraph& graph)
{
    PROFILER_EVENT();

	const std::vector<vk::DescriptorSetLayout> SRSLayouts = generateShaderResourceSetLayouts(task, graph);
    const vk::PipelineLayout layout = generatePipelineLayout(SRSLayouts, task);

    ComputePipelineHandles handles{ std::make_shared<PipelineTemplate>(this, nullptr, layout), SRSLayouts };

    return handles;
}


vk::RenderPass	VulkanRenderDevice::generateRenderPass(const GraphicsTask& task)
{
    PROFILER_EVENT();

    const auto& outputAttachments = task.getOuputAttachments();

    std::vector<vk::AttachmentDescription> attachmentDescriptions;

    // needed for subpass createInfo;
    bool hasDepthAttachment = false;
    std::vector<vk::AttachmentReference> outputAttachmentRefs{};
    std::vector<vk::AttachmentReference> depthAttachmentRef{};
    uint32_t outputAttatchmentCounter = 0;

    for(const auto& [name, type, format, size, loadop, storeOp, usage] : outputAttachments)
    {
        // We only care about images here.
        if(type == AttachmentType::DataBufferRO ||
           type == AttachmentType::DataBufferRW ||
           type == AttachmentType::DataBufferWO ||
           type == AttachmentType::PushConstants ||
           type == AttachmentType::UniformBuffer ||
           type == AttachmentType::ShaderResourceSet ||
           type == AttachmentType::CommandPredicationBuffer)
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

        vk::AttachmentLoadOp lop = [loadType = loadop]()
        {
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

        vk::AttachmentStoreOp sop = [storeType = storeOp]()
        {
            switch(storeType)
            {
                case StoreOp::Store:
                    return vk::AttachmentStoreOp::eStore;

                case StoreOp::Discard:
                    return vk::AttachmentStoreOp::eDontCare;
            }
        }();

        attachmentDesc.setLoadOp(lop);
        attachmentDesc.setStoreOp(sop);
        attachmentDesc.setStencilLoadOp(lop);
        attachmentDesc.setStencilStoreOp(sop);
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


template<typename B>
vk::DescriptorSetLayout VulkanRenderDevice::generateDescriptorSetLayoutBindings(const std::vector<B>& bindings, const TaskType taskType)
{
    PROFILER_EVENT();

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
            case TaskType::AsyncCompute:
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
    PROFILER_EVENT();

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


vulkanResources VulkanRenderDevice::getTaskResources(const RenderGraph& graph, const RenderTask& task, const uint64_t prefixHash)
{
    PROFILER_EVENT();

    std::hash<std::string> stringHasher{};

    uint64_t hash = prefixHash;
    hash += stringHasher(task.getName());

    mResourcesLock.lock_shared();

    auto element = mVulkanResources.find(hash);
    if(element != mVulkanResources.end())
    {
        vulkanResources resources = element->second;
        BELL_ASSERT(resources.mDebugName == task.getName(), "Hash collision")
        mResourcesLock.unlock_shared();

        return resources;
    }
    else
    {
        mResourcesLock.unlock_shared();
        mResourcesLock.lock(); // need to write to the resource map so need exclusive access.

        vulkanResources resources = generateVulkanResources(graph, task);
#if ENABLE_LOGGING
        resources.mDebugName = task.getName();
#endif
        mVulkanResources.insert({hash, resources});

        mResourcesLock.unlock();

        return resources;
    }
}


vulkanResources VulkanRenderDevice::getTaskResources(const RenderGraph& graph, const uint32_t taskIndex, const uint64_t prefixHash)
{
    const RenderTask& task = graph.getTask(taskIndex);

    return getTaskResources(graph, task, prefixHash);
}


vulkanResources VulkanRenderDevice::generateVulkanResources(const RenderGraph& graph, const RenderTask& task)
{
    PROFILER_EVENT();

    if(task.taskType() == TaskType::Graphics)
    {
        const GraphicsTask& graphicsTask = static_cast<const GraphicsTask&>(task);
        GraphicsPipelineHandles pipelineHandles = createPipelineHandles(graphicsTask, graph);

#if ENABLE_LOGGING
        vulkanResources resources{task.getName(), pipelineHandles.mGraphicsPipelineTemplate, pipelineHandles.mDescriptorSetLayout, pipelineHandles.mRenderPass, {}, {}};
#else
        vulkanResources resources{pipelineHandles.mGraphicsPipelineTemplate, pipelineHandles.mDescriptorSetLayout, pipelineHandles.mRenderPass, {}, {}};
#endif
        // Frame buffers and descset get created/written in a diferent place.

        return resources;
    }
    else
    {
        const ComputeTask& computeTask = static_cast<const ComputeTask&>(task);
        ComputePipelineHandles pipelineHandles = createPipelineHandles(computeTask, graph);

#if ENABLE_LOGGING
        vulkanResources resources{task.getName(), pipelineHandles.mComputePipelineTemplate, pipelineHandles.mDescriptorSetLayout, {}, {}, {}};
#else
        vulkanResources resources{pipelineHandles.mComputePipelineTemplate, pipelineHandles.mDescriptorSetLayout, {}, {}, {}};
#endif
        return resources;
    }
}


vulkanResources VulkanRenderDevice::generateVulkanResources(const RenderGraph& graph, const uint32_t taskIndex)
{
    const RenderTask& task = graph.getTask(taskIndex);

    return generateVulkanResources(graph, task);
}


vk::Framebuffer VulkanRenderDevice::createFrameBuffer(const RenderGraph& graph, const uint32_t taskIndex, const vk::RenderPass renderPass)
{
    PROFILER_EVENT();

    const GraphicsTask& task = static_cast<const GraphicsTask&>(graph.getTask(taskIndex));
    const GraphicsPipelineDescription& pipelineDesc = task.getPipelineDescription();
    const auto& outputBindings = task.getOuputAttachments();

    std::vector<vk::ImageView> imageViews{};
    ImageExtent imageExtent = {pipelineDesc.mViewport.x, pipelineDesc.mViewport.y, 1u};

    for(const auto& bindingInfo : outputBindings)
    {
            const auto& imageView = graph.getImageView(bindingInfo.mName);
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
    PROFILER_EVENT();

	std::vector<vk::DescriptorSetLayout> layouts{};
	layouts.push_back(generateDescriptorSetLayout(task));

	const auto& inputs = task.getInputAttachments();
    for (const auto& [slot, type, _] : inputs)
	{
		if (type == AttachmentType::ShaderResourceSet)
		{
			layouts.push_back(static_cast<const VulkanShaderResourceSet&>(*graph.getShaderResourceSet(slot).getBase()).getLayout());
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
    PROFILER_EVENT();

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
    PROFILER_EVENT();

    frameSyncSetup();
    // update timestampts.
    mFinishedTimeStamps.clear();
    for(CommandContextBase* context : mGraphicsCommandContexts[mCurrentFrameIndex])
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
    for (CommandContextBase* context : mAsyncComputeCommandContexts[mCurrentFrameIndex])
    {
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
}


void VulkanRenderDevice::submitContext(CommandContextBase *context, const bool finalSubmission)
{
    PROFILER_EVENT();

    vk::CommandBuffer primaryCmdBuffer = static_cast<VulkanCommandContext*>(context)->getPrefixCommandBuffer();
    primaryCmdBuffer.end();

    const VulkanSwapChain* swapChain = static_cast<VulkanSwapChain*>(mSwapChain);
    const uint64_t semaphoreRead = static_cast<VulkanCommandContext*>(context)->getSemaphoreSignalRead();
    const uint64_t semaphoreWrite = static_cast<VulkanCommandContext*>(context)->getSemaphoreSignalWrite();

    StaticGrowableBuffer<vk::Semaphore, 4> waitSemaphores;
    StaticGrowableBuffer<uint64_t, 4> waitSemaphoresValues;
    if(mSubmissionCount == 0)
    {
        waitSemaphores.push_back(*swapChain->getImageAquired());
        waitSemaphoresValues.push_back(0);
    }
    if(semaphoreRead != ~0ULL)
    {
        waitSemaphores.push_back(getAsyncQueueSemaphore());
        waitSemaphoresValues.push_back(semaphoreRead);
    }

    StaticGrowableBuffer<vk::Semaphore, 4> signalSemaphores;
    StaticGrowableBuffer<uint64_t, 4> signalSemaphoresValues;
    if(finalSubmission)
    {
        signalSemaphores.push_back(*swapChain->getImageRendered());
        signalSemaphoresValues.push_back(0);
    }
    if(semaphoreWrite != ~0ULL)
    {
        signalSemaphores.push_back(getAsyncQueueSemaphore());
        signalSemaphoresValues.push_back(semaphoreWrite);
    }

    vk::SubmitInfo submitInfo{};
    submitInfo.setCommandBufferCount(1);
    submitInfo.setPCommandBuffers(&primaryCmdBuffer);
    submitInfo.setPWaitSemaphores(waitSemaphores.data());
    submitInfo.setWaitSemaphoreCount(waitSemaphores.size());
    submitInfo.setPSignalSemaphores(signalSemaphores.data());
    submitInfo.setSignalSemaphoreCount(signalSemaphores.size());
    const vk::PipelineStageFlags waitStage = waitSemaphores.size() > 1 ? vk::PipelineStageFlagBits::eTopOfPipe : vk::PipelineStageFlagBits::eColorAttachmentOutput;
    submitInfo.setPWaitDstStageMask(&waitStage);

    vk::TimelineSemaphoreSubmitInfo timelineInfo{};
    timelineInfo.setPWaitSemaphoreValues(waitSemaphoresValues.data());
    timelineInfo.setWaitSemaphoreValueCount(waitSemaphoresValues.size());
    timelineInfo.setPSignalSemaphoreValues(signalSemaphoresValues.data());
    timelineInfo.setSignalSemaphoreValueCount(signalSemaphoresValues.size());
    submitInfo.setPNext(&timelineInfo);

    if (context->getQueueType() == QueueType::Compute)
        mComputeQueue.submit(submitInfo, vk::Fence{ nullptr });
    else
        mGraphicsQueue.submit(submitInfo, finalSubmission ? mFrameFinished[getCurrentFrameIndex()] : vk::Fence{nullptr});

    ++mSubmissionCount;
}


void VulkanRenderDevice::swap()
{
    mSwapChain->present(QueueType::Graphics);
}


void VulkanRenderDevice::frameSyncSetup()
{
	mCurrentFrameIndex = mSwapChain->getNextImageIndex();

    // wait for the previous frame using this swapchain image to be finished.
    vk::Fence fence = mFrameFinished[getCurrentFrameIndex()];
    mDevice.waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
    mDevice.resetFences(1, &fence);
}


void VulkanRenderDevice::clearDeferredResources()
{
    PROFILER_EVENT();

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
    PROFILER_EVENT();

    mDevice.waitIdle();

    for(auto& [hash, handles] : mVulkanResources)
    {
        mDevice.destroyPipelineLayout(handles.mPipelineTemplate->getLayoutHandle());
        handles.mPipelineTemplate->invalidatePipelineCache();
        mDevice.destroyDescriptorSetLayout(handles.mDescSetLayout[0]); // if there are any more they will get destroyed elsewhere.
        if(handles.mRenderPass)
            mDevice.destroyRenderPass(*handles.mRenderPass);

    }
    mVulkanResources.clear();

    for(auto& [imageViews, frameBuffer] : mFrameBufferCache)
    {
        destroyFrameBuffer(frameBuffer, mCurrentSubmission);
        mFrameBufferCache.erase(imageViews);
    }
}


vk::CommandBuffer VulkanRenderDevice::getPrefixCommandBuffer()
{
    VulkanCommandContext* VKCmdContext = static_cast<VulkanCommandContext*>(getCommandContext(0, QueueType::Graphics));
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
