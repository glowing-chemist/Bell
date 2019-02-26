#include "RenderDevice.hpp"
#include "RenderInstance.hpp"
#include <vulkan/vulkan.hpp>


RenderDevice::RenderDevice(vk::PhysicalDevice physDev, vk::Device dev, vk::SurfaceKHR surface, GLFWwindow* window) :
    mCurrentSubmission{0},
    mFinishedSubmission{0},
    mDevice{dev},
    mPhysicalDevice{physDev},
    mSwapChain{mDevice, mPhysicalDevice, surface, window},
    mMemoryManager{this},
    mDescriptorManager{this}
{

    mQueueFamilyIndicies = getAvailableQueues(surface, mPhysicalDevice);
    mGraphicsQueue = mDevice.getQueue(mQueueFamilyIndicies.GraphicsQueueIndex, 0);
    mComputeQueue  = mDevice.getQueue(mQueueFamilyIndicies.ComputeQueueIndex, 0);
    mTransferQueue = mDevice.getQueue(mQueueFamilyIndicies.TransferQueueIndex, 0);

	// Create a command pool for each frame.
	for (uint32_t i = 0; i < mSwapChain.getNumberOfSwapChainImages(); ++i)
	{
		mCommandPools.push_back(CommandPool{ this });
	}
}


vk::Image   RenderDevice::createImage(const vk::Format format,
                                      const vk::ImageUsageFlags usage,
                                      const vk::ImageType type,
                                      const uint32_t x,
                                      const uint32_t y,
                                      const uint32_t z)
{
    // default to only allowing 2D images for now, with no MS
    vk::ImageCreateInfo createInfo(vk::ImageCreateFlags{},
                                  type,
                                  format,
                                  vk::Extent3D{x, y, z},
                                  0,
                                  1,
                                  vk::SampleCountFlagBits::e1,
                                  vk::ImageTiling::eOptimal,
                                  usage,
                                  vk::SharingMode::eExclusive,
                                  0,
                                  nullptr,
                                  vk::ImageLayout::eUndefined);

    return mDevice.createImage(createInfo);
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
    }
    return mGraphicsQueue; // This should be unreachable unless I add more queue types.
}


uint32_t RenderDevice::getQueueFamilyIndex(const QueueType type) const
{
	switch(type)
	{
	case QueueType::Graphics:
		return mQueueFamilyIndicies.GraphicsQueueIndex;
	case QueueType::Compute:
		return mQueueFamilyIndicies.ComputeQueueIndex;
	case QueueType::Transfer:
		return mQueueFamilyIndicies.TransferQueueIndex;
	}
	return mQueueFamilyIndicies.GraphicsQueueIndex; // This should be unreachable unless I add more queue types.
}


GraphicsPipelineHandles RenderDevice::createPipelineHandles(const GraphicsTask& task)
{
    const auto [vertexBindings, vertexAttributes] = generateVertexInput(task);
    const vk::DescriptorSetLayout descSetLayout = generateDescriptorSetLayout(task);
    const vk::PipelineLayout pipelineLayout = generatePipelineLayout(descSetLayout);
    const vk::RenderPass renderPass = generateRenderPass(task);
    const vk::Pipeline pipeline = generatePipeline( task,
                                                    pipelineLayout,
                                                    vertexBindings,
                                                    vertexAttributes,
                                                    renderPass);

    return {pipeline, pipelineLayout, renderPass, vertexBindings, vertexAttributes, descSetLayout};
}


ComputePipelineHandles RenderDevice::createPipelineHandles(const ComputeTask& task)
{
    const vk::DescriptorSetLayout descSetLayout = generateDescriptorSetLayout(task);
    const vk::PipelineLayout pipelineLayout = generatePipelineLayout(descSetLayout);
    const vk::Pipeline pipeline = generatePipeline(task, pipelineLayout);

    return {pipeline, pipelineLayout, descSetLayout};
}


vk::Pipeline RenderDevice::generatePipeline(const GraphicsTask& task,
                                            const vk::PipelineLayout &pipelineLayout,
                                            const vk::VertexInputBindingDescription &vertexBinding,
                                            const std::vector<vk::VertexInputAttributeDescription> &vertexAttributes,
                                            const vk::RenderPass &renderPass)
{
    const GraphicsPipelineDescription& pipelineDesc = task.getPipelineDescription();
    vk::PipelineRasterizationStateCreateInfo rasterInfo = generateRasterizationInfo(task);
    std::vector<vk::PipelineShaderStageCreateInfo> shaderInfo = generateShaderStagesInfo(task);

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.setTopology(vk::PrimitiveTopology::eTriangleList);
    inputAssemblyInfo.setPrimitiveRestartEnable(false);

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.setVertexAttributeDescriptionCount(vertexAttributes.size());
    vertexInputInfo.setPVertexAttributeDescriptions(vertexAttributes.data());
    vertexInputInfo.setVertexBindingDescriptionCount(1);
    vertexInputInfo.setPVertexBindingDescriptions(&vertexBinding);

    // Viewport and scissor Rects
    vk::Extent2D viewportExtent{pipelineDesc.mViewport.x, pipelineDesc.mViewport.y};
    vk::Extent2D scissorExtent{pipelineDesc.mScissorRect.x, pipelineDesc.mScissorRect.y};

    vk::Viewport viewPort{0.0f, 0.0f, static_cast<float>(viewportExtent.width), static_cast<float>(viewportExtent.height), 0.0f, 1.0f};

    vk::Offset2D offset{0, 0};

    vk::Rect2D scissor{offset, scissorExtent};

    vk::PipelineViewportStateCreateInfo viewPortInfo{};
    viewPortInfo.setPScissors(&scissor);
    viewPortInfo.setPViewports(&viewPort);
    viewPortInfo.setScissorCount(1);
    viewPortInfo.setViewportCount(1);

    vk::PipelineMultisampleStateCreateInfo multiSampInfo{};
    multiSampInfo.setSampleShadingEnable(false);
    multiSampInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1);

    vk::PipelineColorBlendAttachmentState colorAttachState{};
    colorAttachState.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |  vk::ColorComponentFlagBits::eB |  vk::ColorComponentFlagBits::eA); // write to all color components
    colorAttachState.setBlendEnable(false);

    vk::PipelineColorBlendStateCreateInfo blendStateInfo{};
    blendStateInfo.setLogicOpEnable(pipelineDesc.mBlendMode != BlendMode::None);
    blendStateInfo.setAttachmentCount(1);
    blendStateInfo.setPAttachments(&colorAttachState);
    blendStateInfo.setLogicOp([op = pipelineDesc.mBlendMode] {
       switch(op)
       {
            case BlendMode::None:
            case BlendMode::Or:
                return vk::LogicOp::eOr;
            case BlendMode::Nor:
                return vk::LogicOp::eNor;
            case BlendMode::Xor:
                return vk::LogicOp::eXor;
            case BlendMode::And:
                return vk::LogicOp::eAnd;
            case BlendMode::Nand:
                return vk::LogicOp::eNand;
       }
    }());

    vk::PipelineDepthStencilStateCreateInfo depthStencilInfo{};
    depthStencilInfo.setDepthTestEnable(pipelineDesc.mDepthTest != DepthTest::None);
    depthStencilInfo.setDepthWriteEnable(true);
    depthStencilInfo.setDepthCompareOp([testOp = pipelineDesc.mDepthTest] {
       switch(testOp)
       {
            case DepthTest::Less:
                return vk::CompareOp::eLess;
            case DepthTest::None:
                return vk::CompareOp::eNever;
            case DepthTest::LessEqual:
                return vk::CompareOp::eLessOrEqual;
            case DepthTest::Equal:
                return vk::CompareOp::eEqual;
            case DepthTest::GreaterEqual:
                return vk::CompareOp::eGreaterOrEqual;
            case DepthTest::Greater:
                return vk::CompareOp::eGreater;
       }
    }());
    depthStencilInfo.setDepthBoundsTestEnable(false);

    vk::GraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.setStageCount(shaderInfo.size());
    pipelineCreateInfo.setPStages(shaderInfo.data());
    pipelineCreateInfo.setPVertexInputState(&vertexInputInfo);
    pipelineCreateInfo.setPInputAssemblyState(&inputAssemblyInfo);
    pipelineCreateInfo.setPTessellationState(nullptr); // We don't handle tesselation shaders yet
    pipelineCreateInfo.setPViewportState(&viewPortInfo);
    pipelineCreateInfo.setPRasterizationState(&rasterInfo);
    pipelineCreateInfo.setPMultisampleState(&multiSampInfo);
    pipelineCreateInfo.setPDepthStencilState(&depthStencilInfo);
    pipelineCreateInfo.setPDynamicState(nullptr);
    pipelineCreateInfo.setLayout(pipelineLayout);
    pipelineCreateInfo.setRenderPass(renderPass);

    return mDevice.createGraphicsPipeline(nullptr, pipelineCreateInfo);
}


vk::Pipeline RenderDevice::generatePipeline(const ComputeTask& task,
                                            const vk::PipelineLayout& pipelineLayout)
{
    const ComputePipelineDescription PipelineDesc = task.getPipelineDescription();

    vk::PipelineShaderStageCreateInfo computeShaderInfo{};
    computeShaderInfo.setModule(PipelineDesc.mComputeShader.getShaderModule());
    computeShaderInfo.setPName("main");
    computeShaderInfo.setStage(vk::ShaderStageFlagBits::eCompute);

    vk::ComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.setStage(computeShaderInfo);
    pipelineInfo.setLayout(pipelineLayout);

    return mDevice.createComputePipeline(nullptr, pipelineInfo);
}


vk::RenderPass	RenderDevice::generateRenderPass(const GraphicsTask& task)
{
    // TODO implement a renderpass cache.

    const auto& inputAttachments = task.getInputAttachments();
    const auto& outputAttachments = task.getOuputAttachments();

    std::vector<vk::AttachmentDescription> attachmentDescriptions;

    for(const auto& [name, type] : inputAttachments)
    {
        // We only care about images here.
        if(type == AttachmentType::DataBuffer ||
           type == AttachmentType::PushConstants ||
           type == AttachmentType::UniformBuffer)
                continue;

        vk::AttachmentDescription attachmentDesc{};

        // get the needed format
        const std::pair<vk::Format, vk::ImageLayout> formatandLayout = [type, this]()
        {
            switch(type)
            {
                case AttachmentType::Texture1D:
                case AttachmentType::Texture2D:
                case AttachmentType::Texture3D:
                    return std::make_pair(vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eShaderReadOnlyOptimal);
                case AttachmentType::Depth:
                    return std::make_pair(vk::Format::eD32Sfloat, vk::ImageLayout::eDepthStencilReadOnlyOptimal);
                case AttachmentType::SwapChain:
                    return std::make_pair(this->getSwapChain()->getSwapChainImageFormat(),
                                          vk::ImageLayout::eShaderReadOnlyOptimal);
                default:
                    return std::make_pair(vk::Format::eR8Sint, vk::ImageLayout::eUndefined); // should be obvious that something has gone wrong.
            }
        }();

        attachmentDesc.setFormat(formatandLayout.first);

        // eventually I will implment a render pass system that is aware of what comes beofer and after it
        // in order to avoid having to do manula barriers for all transitions.
        attachmentDesc.setInitialLayout((formatandLayout.second));
        attachmentDesc.setFinalLayout(formatandLayout.second);

        attachmentDesc.setLoadOp(vk::AttachmentLoadOp::eLoad); // we are going to overwrite all pixles
        attachmentDesc.setStoreOp(vk::AttachmentStoreOp::eStore);
        attachmentDesc.setStencilLoadOp(vk::AttachmentLoadOp::eLoad);
        attachmentDesc.setStencilStoreOp(vk::AttachmentStoreOp::eStore);

        attachmentDescriptions.push_back((attachmentDesc));
    }

    for(const auto& [name, type] : outputAttachments)
    {
        // We only care about images here.
        if(type == AttachmentType::DataBuffer ||
           type == AttachmentType::PushConstants ||
           type == AttachmentType::UniformBuffer)
                continue;

        vk::AttachmentDescription attachmentDesc{};

        // get the needed format
        const std::pair<vk::Format, vk::ImageLayout> formatandLayout = [type, this]()
        {
            switch(type)
            {
                case AttachmentType::RenderTarget1D:
                case AttachmentType::RenderTarget2D:
                case AttachmentType::RenderTarget3D:
                    return std::make_pair(vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eColorAttachmentOptimal);
                case AttachmentType::Depth:
                    return std::make_pair(vk::Format::eD32Sfloat, vk::ImageLayout::eDepthStencilAttachmentOptimal);
                case AttachmentType::SwapChain:
                    return std::make_pair(this->getSwapChain()->getSwapChainImageFormat(),
                                          vk::ImageLayout::eColorAttachmentOptimal);
                default:
                    return std::make_pair(vk::Format::eR8Sint, vk::ImageLayout::eUndefined); // should be obvious that something has gone wrong.
            }
        }();

        attachmentDesc.setFormat(formatandLayout.first);

        // eventually I will implment a render pass system that is aware of what comes beofer and after it
        // in order to avoid having to do manula barriers for all transitions.
        attachmentDesc.setInitialLayout((formatandLayout.second));
        attachmentDesc.setFinalLayout(formatandLayout.second);

        attachmentDesc.setLoadOp(vk::AttachmentLoadOp::eLoad); // we are going to overwrite all pixles
        attachmentDesc.setStoreOp(vk::AttachmentStoreOp::eStore);
        attachmentDesc.setStencilLoadOp(vk::AttachmentLoadOp::eLoad);
        attachmentDesc.setStencilStoreOp(vk::AttachmentStoreOp::eStore);

        attachmentDescriptions.push_back((attachmentDesc));
    }

    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.setAttachmentCount(attachmentDescriptions.size());
    renderPassInfo.setPAttachments(attachmentDescriptions.data());

    return mDevice.createRenderPass(renderPassInfo);
}


vk::PipelineRasterizationStateCreateInfo RenderDevice::generateRasterizationInfo(const GraphicsTask& task)
{
    const GraphicsPipelineDescription pipelineDesc = task.getPipelineDescription();

    vk::PipelineRasterizationStateCreateInfo rastInfo{};
    rastInfo.setRasterizerDiscardEnable(false);
    rastInfo.setDepthBiasClamp(false);
    rastInfo.setPolygonMode(vk::PolygonMode::eFill); // output filled in fragments
    rastInfo.setLineWidth(1.0f);
    if(pipelineDesc.mUseBackFaceCulling){
        rastInfo.setCullMode(vk::CullModeFlagBits::eBack); // cull fragments from the back
        rastInfo.setFrontFace(vk::FrontFace::eClockwise);
    } else {
        rastInfo.setCullMode(vk::CullModeFlagBits::eNone);
    }
    rastInfo.setDepthBiasEnable(false);

    return rastInfo;
}


std::vector<vk::PipelineShaderStageCreateInfo> generateShaderStagesInfo(const GraphicsTask& task)
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
        vk::DescriptorType descriptorType = [type]()
        {
            switch(type)
            {
            case AttachmentType::RenderTarget1D:
            case AttachmentType::RenderTarget2D:
            case AttachmentType::RenderTarget3D:
                return vk::DescriptorType::eCombinedImageSampler;

           case AttachmentType::DataBuffer:
                return vk::DescriptorType::eStorageBufferDynamic;

           case AttachmentType::UniformBuffer:
                return vk::DescriptorType::eUniformBuffer;

           // Ignore push constants for now.
            }

            return vk::DescriptorType::eSampler; // For now use this to indicate push_constants (terrible I know)
        }();

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
    descSetLayoutInfo.setBindingCount(layoutBindings.size());

    return mDevice.createDescriptorSetLayout(descSetLayoutInfo);
}


std::pair<vk::VertexInputBindingDescription, std::vector<vk::VertexInputAttributeDescription>> generateVertexInput(const GraphicsTask& task)
{
    const int vertexInputs = task.getVertexAttributes();

    const bool hasPosition = vertexInputs & VertexAttributes::Position;
    const bool hasTextureCoords = vertexInputs & VertexAttributes::TextureCoordinates;
    const bool hasNormals = vertexInputs & VertexAttributes::Normals;
    const bool hasAlbedo = vertexInputs & VertexAttributes::Aledo;

    const uint32_t vertexStride = (hasPosition ? 4 : 0) + (hasTextureCoords ? 2 : 0) + (hasNormals ? 4 : 0) + (hasAlbedo ? 1 : 0);

    vk::VertexInputBindingDescription bindingDesc{};
    bindingDesc.setStride(vertexStride);
    bindingDesc.setBinding(0);
    bindingDesc.setInputRate(vk::VertexInputRate::eVertex); // only support the same model for instances draws currently.

    std::vector<vk::VertexInputAttributeDescription> attribs;
    uint8_t currentOffset = 0;
    uint8_t currentLocation  = 0;

    if(hasPosition)
    {
        vk::VertexInputAttributeDescription attribDescPos{};
        attribDescPos.setBinding(0);
        attribDescPos.setLocation(currentLocation);
        attribDescPos.setFormat(vk::Format::eR32G32B32A32Sfloat);
        attribDescPos.setOffset(currentOffset);

        attribs.push_back(attribDescPos);
        currentOffset += 16;
        ++currentLocation;
    }

    if(hasTextureCoords)
    {
        vk::VertexInputAttributeDescription attribDescTex{};
        attribDescTex.setBinding(0);
        attribDescTex.setLocation(currentLocation);
        attribDescTex.setFormat(vk::Format::eR32G32Sfloat);
        attribDescTex.setOffset(currentOffset);

        attribs.push_back(attribDescTex);
        currentOffset += 8;
        ++currentLocation;
    }

    if(hasNormals)
    {
        vk::VertexInputAttributeDescription attribDescNormal{};
        attribDescNormal.setBinding(0);
        attribDescNormal.setLocation(currentLocation);
        attribDescNormal.setFormat(vk::Format::eR32G32B32A32Sfloat);
        attribDescNormal.setOffset(currentOffset);

        attribs.push_back(attribDescNormal);
        currentOffset += 16;
        ++currentLocation;
    }

    if(hasAlbedo)
    {
        vk::VertexInputAttributeDescription attribDescAlbedo{};
        attribDescAlbedo.setBinding(0);
        attribDescAlbedo.setLocation(currentLocation);
        attribDescAlbedo.setFormat(vk::Format::eR32Sfloat);
        attribDescAlbedo.setOffset(currentOffset);

        attribs.push_back(attribDescAlbedo);
    }

    return {bindingDesc, attribs};
}


vk::PipelineLayout RenderDevice::generatePipelineLayout(vk::DescriptorSetLayout descLayout)
{
    vk::PipelineLayoutCreateInfo info{};
    info.setSetLayoutCount(1);
    info.setPSetLayouts(&descLayout);

    return mDevice.createPipelineLayout(info);
}


void RenderDevice::generateVulkanResources(RenderGraph& graph)
{
	auto task = graph.taskBegin();
	auto resource = graph.resourceBegin();
    
	while( task != graph.taskEnd())
    {
		if ((*resource).mPipeline != vk::Pipeline{ nullptr })
		{
			++task;
			++resource;
			continue;
		}

        if((*task).taskType() == TaskType::Graphics)
        {
            const GraphicsTask& graphicsTask = static_cast<const GraphicsTask&>(*task);
            GraphicsPipelineHandles pipelineHandles = createPipelineHandles(graphicsTask);

            RenderGraph::vulkanResources& taskResources = *resource;
            taskResources.mPipeline = pipelineHandles.mPipeline;
            taskResources.mPipelineLayout = pipelineHandles.mPipelineLayout;
            taskResources.mDescSetLayout = pipelineHandles.mDescriptorSetLayout;
            taskResources.mRenderPass = pipelineHandles.mRenderPass;
            taskResources.mVertexBindingDescription = pipelineHandles.mVertexBindingDescription;
            taskResources.mVertexAttributeDescription = pipelineHandles.mVertexAttributeDescription;
            // Frame buffers and descset get created/written in a diferent place.
        }
        else
        {
            const ComputeTask& computeTask = static_cast<const ComputeTask&>(*task);
            ComputePipelineHandles pipelineHandles = createPipelineHandles(computeTask);

            RenderGraph::vulkanResources& taskResources = *resource;
            taskResources.mPipeline = pipelineHandles.mPipeline;
            taskResources.mPipelineLayout = pipelineHandles.mPipelineLayout;
            taskResources.mDescSetLayout = pipelineHandles.mDescriptorSetLayout;
        }
		++task;
		++resource;
    }
}


void RenderDevice::generateFrameBuffers(RenderGraph& graph)
{
	auto outputBindings = graph.outputBindingBegin();
	auto resource = graph.resourceBegin();

	while(outputBindings != graph.outputBindingEnd())
    {
        if(!(*resource).mFrameBufferNeedsUpdating)
            continue;

        std::vector<vk::ImageView> imageViews{};
        vk::Extent3D imageExtent;

        for(const auto& bindingInfo : *outputBindings)
        {
            const auto& image = static_cast<Image&>(graph.getResource(bindingInfo.mResourcetype, bindingInfo.mResourceIndex));
            imageExtent = image.getExtent();
            imageViews.push_back(image.getCurrentImageView());
        }

        vk::FramebufferCreateInfo info{};
        info.setRenderPass(*(*resource).mRenderPass);
        info.setPAttachments(imageViews.data());
        info.setWidth(imageExtent.width);
        info.setHeight(imageExtent.height);
        info.setLayers(imageExtent.depth);

        (*resource).mFrameBuffer = mDevice.createFramebuffer(info);

		++resource;
		++outputBindings;
    }
}


void RenderDevice::generateDescriptorSets(RenderGraph & graph)
{
    auto descriptorSets = mDescriptorManager.getDescriptors(graph);
    mDescriptorManager.writeDescriptors(descriptorSets, graph);
}


void RenderDevice::execute(RenderGraph& graph)
{
	CommandPool* currentCommandPool = getCurrentCommandPool();
	currentCommandPool->reserve(graph.taskCount() + 1, QueueType::Graphics); // +1 for the primary cmd buffer all the secondaries will be recorded in to.

	vk::CommandBufferBeginInfo primaryBegin{};
	primaryBegin.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	
	vk::CommandBuffer primaryCmdBuffer = currentCommandPool->getBufferForQueue(QueueType::Graphics);
	primaryCmdBuffer.begin(primaryBegin);

	uint32_t cmdBufferIndex = 1;
	auto vulkanResource = graph.resourceBegin();
	for (auto task = graph.taskBegin(); task != graph.taskEnd(); ++task, ++vulkanResource)
	{
		vk::CommandBufferBeginInfo secondaryBegin{};
		secondaryBegin.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		vk::CommandBuffer secondaryCmdBuffer = currentCommandPool->getBufferForQueue(QueueType::Graphics, cmdBufferIndex);
		secondaryCmdBuffer.begin(secondaryBegin);
		
		const auto& resources = *vulkanResource;
		
		vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eCompute;


		if (resources.mRenderPass)
		{
			const auto& graphicsTask = static_cast<const GraphicsTask&>(*task);
			const auto pipelineDesc = graphicsTask.getPipelineDescription();
			vk::Rect2D renderArea{ {0, 0}, {pipelineDesc.mViewport.x, pipelineDesc.mViewport.y} };

			vk::RenderPassBeginInfo passBegin{};
			passBegin.setRenderPass(*resources.mRenderPass);
			passBegin.setFramebuffer(*resources.mFrameBuffer);
			passBegin.setRenderArea(renderArea);

			secondaryCmdBuffer.beginRenderPass(passBegin, vk::SubpassContents::eInline);

			bindPoint = vk::PipelineBindPoint::eGraphics;
		}

		secondaryCmdBuffer.bindVertexBuffers(0, { graph.getVertexBuffer().getBuffer() }, {});
		secondaryCmdBuffer.bindIndexBuffer(graph.getIndexBuffer().getBuffer(), 0, vk::IndexType::eUint32);
		secondaryCmdBuffer.bindDescriptorSets(bindPoint, resources.mPipelineLayout, 0, 1,  &resources.mDescSet, 0, nullptr);
		secondaryCmdBuffer.bindPipeline(bindPoint, resources.mPipeline);

		(*task).recordCommands(secondaryCmdBuffer);

		secondaryCmdBuffer.endRenderPass();
		secondaryCmdBuffer.end();

		primaryCmdBuffer.executeCommands(secondaryCmdBuffer);

		++cmdBufferIndex;
	}

	primaryCmdBuffer.end();
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

