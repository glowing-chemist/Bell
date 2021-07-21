#include "VulkanPipeline.hpp"
#include "VulkanShader.hpp"
#include "VulkanRenderDevice.hpp"
#include "Core/BellLogging.hpp"

Pipeline::Pipeline(RenderDevice* dev) :
	DeviceChild{ dev } {}


PipelineTemplate::PipelineTemplate(RenderDevice* dev, const GraphicsPipelineDescription* desc, const vk::PipelineLayout layout) :
    DeviceChild(dev),
    mGraphicsPipeline(desc != nullptr),
    mDesc(mGraphicsPipeline ? *desc : GraphicsPipelineDescription(Rect{0, 0}, Rect{0, 0})),
    mPipelineLayout(layout) {}


std::shared_ptr<Pipeline> PipelineTemplate::instanciateGraphicsPipeline(const GraphicsTask& task,
                                                      const uint64_t hashPrefix,
                                                      const vk::RenderPass rp,
                                                      const int vertexAttributes,
                                                      const Shader& vertexShader,
                                                      const Shader* geometryShader,
                                                      const Shader* tessControl,
                                                      const Shader* tessEval,
                                                      const Shader& fragmentShader)
{
    std::hash<std::string> stringHasher{};
    uint64_t hash = hashPrefix ^ vertexAttributes;
    hash  += stringHasher(vertexShader->getFilePath());
    if(geometryShader)
        hash  += stringHasher((*geometryShader)->getFilePath());
    if(tessControl)
        hash  += stringHasher((*tessControl)->getFilePath());
    if(tessEval)
        hash  += stringHasher((*tessEval)->getFilePath());
    hash  += stringHasher(fragmentShader->getFilePath());

    if(mPipelineCache.find(hash) != mPipelineCache.end())
    {
        BELL_ASSERT(mPipelineCache[hash]->getDebugName() == task.getName(), "Hash collision")
        return mPipelineCache[hash];
    }
    else
    {
        std::shared_ptr<GraphicsPipeline> graphicsPipeline = std::make_shared<GraphicsPipeline>(getDevice(), mDesc,
                                                                                       vertexShader,
                                                                                       geometryShader,
                                                                                       tessControl,
                                                                                       tessEval,
                                                                                       fragmentShader);
        graphicsPipeline->setLayout(mPipelineLayout);
        graphicsPipeline->setRenderPass(rp);
        graphicsPipeline->setVertexAttributes(vertexAttributes);
        graphicsPipeline->setFrameBufferBlendStates(task);
        graphicsPipeline->setDebugName(task.getName());
        graphicsPipeline->compile(task);

        mPipelineCache.insert({hash, graphicsPipeline});

        return graphicsPipeline;
    }
}


std::shared_ptr<Pipeline> PipelineTemplate::instanciateComputePipeline(const ComputeTask& task, const uint64_t hashPrefix, const Shader& computeShader)
{
    std::hash<std::string> stringHasher{};
    uint64_t hash = hashPrefix;
    hash  += stringHasher(computeShader->getFilePath());

    if(mPipelineCache.find(hash) != mPipelineCache.end())
    {
        BELL_ASSERT(mPipelineCache[hash]->getDebugName() == task.getName(), "Hash collision")
        return mPipelineCache[hash];
    }
    else
    {
        std::shared_ptr<Pipeline> computePipeline = std::make_shared<ComputePipeline>(getDevice(), computeShader);
        computePipeline->setLayout(mPipelineLayout);
        computePipeline->setDebugName(task.getName());
        computePipeline->compile(task);

        mPipelineCache.insert({hash, computePipeline});

        return computePipeline;
    }
}

void PipelineTemplate::invalidatePipelineCache()
{
    VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());
    for(auto& [hash, pipeline] : mPipelineCache)
    {
        device->destroyPipeline(pipeline->getHandle());
    }
    mPipelineCache.clear();
}

bool ComputePipeline::compile(const RenderTask&)
{
    const VulkanShader& VkShader = static_cast<const VulkanShader&>(*mComputeShader.getBase());
	vk::PipelineShaderStageCreateInfo computeShaderInfo{};
	computeShaderInfo.setModule(VkShader.getShaderModule());
	computeShaderInfo.setPName("main");
	computeShaderInfo.setStage(vk::ShaderStageFlagBits::eCompute);

	vk::ComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.setStage(computeShaderInfo);
	pipelineInfo.setLayout(mPipelineLayout);

	mPipeline = static_cast<VulkanRenderDevice*>(getDevice())->createPipeline(pipelineInfo);

	return true;
}


std::vector<vk::PipelineShaderStageCreateInfo> GraphicsPipeline::generateShaderStagesInfo()
{
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

    vk::PipelineShaderStageCreateInfo vertexStage{};
    vertexStage.setStage(vk::ShaderStageFlagBits::eVertex);
    vertexStage.setPName("main"); //entry point of the shader
    vertexStage.setModule(static_cast<const VulkanShader*>(mVertexShader.getBase())->getShaderModule());
    shaderStages.push_back(vertexStage);

    if (mGeometryShader)
    {
        vk::PipelineShaderStageCreateInfo geometryStage{};
        geometryStage.setStage(vk::ShaderStageFlagBits::eGeometry);
        geometryStage.setPName("main"); //entry point of the shader
        geometryStage.setModule(static_cast<const VulkanShader*>(mGeometryShader->getBase())->getShaderModule());
        shaderStages.push_back(geometryStage);
    }

    if (mTessEvalShader)
    {
        vk::PipelineShaderStageCreateInfo hullStage{};
        hullStage.setStage(vk::ShaderStageFlagBits::eTessellationControl);
        hullStage.setPName("main"); //entry point of the shader
        hullStage.setModule(static_cast<const VulkanShader*>(mTessEvalShader->getBase())->getShaderModule());
        shaderStages.push_back(hullStage);
    }

    if (mTessControlShader)
    {
        vk::PipelineShaderStageCreateInfo tesseStage{};
        tesseStage.setStage(vk::ShaderStageFlagBits::eTessellationEvaluation);
        tesseStage.setPName("main"); //entry point of the shader
        tesseStage.setModule(static_cast<const VulkanShader*>(mTessControlShader->getBase())->getShaderModule());
        shaderStages.push_back(tesseStage);
    }

    vk::PipelineShaderStageCreateInfo fragmentStage{};
    fragmentStage.setStage(vk::ShaderStageFlagBits::eFragment);
    fragmentStage.setPName("main"); //entry point of the shader
    fragmentStage.setModule(static_cast<const VulkanShader*>(mFragmentShader.getBase())->getShaderModule());
    shaderStages.push_back(fragmentStage);

    return shaderStages;
}


bool GraphicsPipeline::compile(const RenderTask&)
{
	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    std::vector<vk::PipelineShaderStageCreateInfo> shaderInfo = generateShaderStagesInfo();

	const vk::PrimitiveTopology topology = [primitiveType = mPipelineDescription.mPrimitiveType]()
	{
		switch (primitiveType)
		{
		case Primitive::LineList:
			return vk::PrimitiveTopology::eLineList;

		case Primitive::LineStrip:
			return vk::PrimitiveTopology::eLineStrip;

		case Primitive::TriangleList:
			return vk::PrimitiveTopology::eTriangleList;

		case Primitive::TriangleStrip:
			return vk::PrimitiveTopology::eTriangleStrip;

		case Primitive::Point:
			return vk::PrimitiveTopology::ePointList;
		}
	}();

	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
	inputAssemblyInfo.setTopology(topology);
	inputAssemblyInfo.setPrimitiveRestartEnable(false);

    vk::PolygonMode polygonMode = [fillMode = mPipelineDescription.mFillMode]()
    {
        switch(fillMode)
        {
            case FillMode::Fill:
                return vk::PolygonMode::eFill;

            case FillMode::Line:
                return vk::PolygonMode::eLine;

            case FillMode::Point:
                return vk::PolygonMode::ePoint;
        }
    }();

	vk::PipelineRasterizationStateCreateInfo rasterInfo{};
	rasterInfo.setRasterizerDiscardEnable(false);
	rasterInfo.setDepthBiasClamp(false);
    rasterInfo.setPolygonMode(polygonMode); // output filled in fragments
	rasterInfo.setLineWidth(1.0f);
    if (mPipelineDescription.mFrontFace != FaceWindingOrder::None) {
		rasterInfo.setCullMode(vk::CullModeFlagBits::eBack); // cull fragments from the back

        vk::FrontFace frontFace = [frontFace = mPipelineDescription.mFrontFace]()
        {
            switch(frontFace)
            {
                case FaceWindingOrder::CCW:
                    return vk::FrontFace::eCounterClockwise;

                case FaceWindingOrder::CW:
                    return vk::FrontFace::eClockwise;
            }
        }();

        rasterInfo.setFrontFace(frontFace);
	}
	else {
		rasterInfo.setCullMode(vk::CullModeFlagBits::eNone);
	}
	rasterInfo.setDepthBiasEnable(false);

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.setVertexAttributeDescriptionCount(static_cast<uint32_t>(mVertexAttribs.size()));
	vertexInputInfo.setPVertexAttributeDescriptions(mVertexAttribs.data());
    vertexInputInfo.setVertexBindingDescriptionCount(mVertexAttribs.empty() ? 0 : 1);
	vertexInputInfo.setPVertexBindingDescriptions(&mVertexDescription);

	// Viewport and scissor Rects
	vk::Extent2D viewportExtent{ mPipelineDescription.mViewport.x, mPipelineDescription.mViewport.y };
	vk::Extent2D scissorExtent{ mPipelineDescription.mScissorRect.x, mPipelineDescription.mScissorRect.y };

	vk::Viewport viewPort{ 0.0f, 0.0f, static_cast<float>(viewportExtent.width), static_cast<float>(viewportExtent.height), 0.0f, 1.0f };

	vk::Offset2D offset{ 0, 0 };

	vk::Rect2D scissor{ offset, scissorExtent };

	vk::PipelineViewportStateCreateInfo viewPortInfo{};
	viewPortInfo.setPScissors(&scissor);
	viewPortInfo.setPViewports(&viewPort);
	viewPortInfo.setScissorCount(1);
	viewPortInfo.setViewportCount(1);

	vk::PipelineMultisampleStateCreateInfo multiSampInfo{};
	multiSampInfo.setSampleShadingEnable(false);
	multiSampInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1);

	vk::PipelineColorBlendStateCreateInfo blendStateInfo{};
	blendStateInfo.setLogicOpEnable(false);
	blendStateInfo.setAttachmentCount(static_cast<uint32_t>(mBlendStates.size()));
	blendStateInfo.setPAttachments(mBlendStates.data());

	vk::PipelineDepthStencilStateCreateInfo depthStencilInfo{};
	depthStencilInfo.setDepthTestEnable(mPipelineDescription.mDepthTest != DepthTest::None);
	depthStencilInfo.setDepthWriteEnable(mPipelineDescription.mDepthWrite);
	depthStencilInfo.setDepthCompareOp([testOp = mPipelineDescription.mDepthTest]{
	   switch (testOp)
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
	pipelineCreateInfo.setStageCount(static_cast<uint32_t>(shaderInfo.size()));
	pipelineCreateInfo.setPStages(shaderInfo.data());
	pipelineCreateInfo.setPVertexInputState(&vertexInputInfo);
	pipelineCreateInfo.setPInputAssemblyState(&inputAssemblyInfo);
	pipelineCreateInfo.setPTessellationState(nullptr); // We don't handle tesselation shaders yet
	pipelineCreateInfo.setPViewportState(&viewPortInfo);
	pipelineCreateInfo.setPRasterizationState(&rasterInfo);
	pipelineCreateInfo.setPColorBlendState(&blendStateInfo);
	pipelineCreateInfo.setPMultisampleState(&multiSampInfo);
	pipelineCreateInfo.setPDepthStencilState(&depthStencilInfo);
	pipelineCreateInfo.setPDynamicState(nullptr);
	pipelineCreateInfo.setLayout(mPipelineLayout);
	pipelineCreateInfo.setRenderPass(mRenderPass);

	mPipeline = device->createPipeline(pipelineCreateInfo);

	return true;
}


void GraphicsPipeline::setVertexAttributes(const int vertexInputs)
{
	const bool hasPosition2 = vertexInputs & VertexAttributes::Position2;
	const bool hasPosition3 = vertexInputs & VertexAttributes::Position3;
	const bool hasPosition4 = vertexInputs & VertexAttributes::Position4;
	const bool hasTextureCoords = vertexInputs & VertexAttributes::TextureCoordinates;
	const bool hasNormals = vertexInputs & VertexAttributes::Normals;
	const bool hasTangents = vertexInputs & VertexAttributes::Tangents;
	const bool hasAlbedo = vertexInputs & VertexAttributes::Albedo;
	const bool hasBones = (vertexInputs & VertexAttributes::BoneIndices) && (vertexInputs & VertexAttributes::BoneWeights);

	uint32_t positionSize = 0;
	if (hasPosition2)
		positionSize = 8;
	else if (hasPosition3)
		positionSize = 12;
	else if (hasPosition4)
		positionSize = 16;

    const uint32_t vertexStride = positionSize + (hasTextureCoords ? 8 : 0) + (hasNormals ? 4 : 0) + (hasTangents ? 4 : 0) + (hasAlbedo ? 4 : 0)
                                    + (hasBones ? 24 : 0);

	vk::VertexInputBindingDescription bindingDesc{};
	bindingDesc.setStride(vertexStride);
	bindingDesc.setBinding(0);
	bindingDesc.setInputRate(vk::VertexInputRate::eVertex); // only support the same model for instances draws currently.

	std::vector<vk::VertexInputAttributeDescription> attribs;
	uint8_t currentOffset = 0;
	uint8_t currentLocation = 0;

	if (hasPosition2)
	{
		vk::VertexInputAttributeDescription attribDescPos{};
		attribDescPos.setBinding(0);
		attribDescPos.setLocation(currentLocation);
		attribDescPos.setFormat(vk::Format::eR32G32Sfloat);
		attribDescPos.setOffset(currentOffset);

		attribs.push_back(attribDescPos);
		currentOffset += 8;
		++currentLocation;
	}

	if (hasPosition3)
	{
		vk::VertexInputAttributeDescription attribDescPos{};
		attribDescPos.setBinding(0);
		attribDescPos.setLocation(currentLocation);
		attribDescPos.setFormat(vk::Format::eR32G32B32Sfloat);
		attribDescPos.setOffset(currentOffset);

		attribs.push_back(attribDescPos);
		currentOffset += 12;
		++currentLocation;
	}

	if (hasPosition4)
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

	if (hasTextureCoords)
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

	if (hasNormals)
	{
		vk::VertexInputAttributeDescription attribDescNormal{};
		attribDescNormal.setBinding(0);
		attribDescNormal.setLocation(currentLocation);
        attribDescNormal.setFormat(vk::Format::eR8G8B8A8Snorm);
		attribDescNormal.setOffset(currentOffset);

		attribs.push_back(attribDescNormal);
        currentOffset += 4;
		++currentLocation;
	}

	if (hasTangents)
	{
		vk::VertexInputAttributeDescription attribDescTangent{};
		attribDescTangent.setBinding(0);
		attribDescTangent.setLocation(currentLocation);
		attribDescTangent.setFormat(vk::Format::eR8G8B8A8Snorm);
		attribDescTangent.setOffset(currentOffset);

		attribs.push_back(attribDescTangent);
		currentOffset += 4;
		++currentLocation;
	}

	if (hasAlbedo)
	{
		vk::VertexInputAttributeDescription attribDescAlbedo{};
		attribDescAlbedo.setBinding(0);
		attribDescAlbedo.setLocation(currentLocation);
		attribDescAlbedo.setFormat(vk::Format::eR8G8B8A8Unorm);
		attribDescAlbedo.setOffset(currentOffset);

		attribs.push_back(attribDescAlbedo);
        currentOffset += 4;
        ++currentLocation;
	}

	if(hasBones)
    {
	    // Bone indices.
        vk::VertexInputAttributeDescription attribDescBoneIndices{};
        attribDescBoneIndices.setBinding(0);
        attribDescBoneIndices.setLocation(currentLocation);
        attribDescBoneIndices.setFormat(vk::Format::eR16G16B16A16Uint);
        attribDescBoneIndices.setOffset(currentOffset);

        attribs.push_back(attribDescBoneIndices);
        currentOffset += 8;
        ++currentLocation;

        // boneWeights
        vk::VertexInputAttributeDescription attribDescBoneWeights{};
        attribDescBoneWeights.setBinding(0);
        attribDescBoneWeights.setLocation(currentLocation);
        attribDescBoneWeights.setFormat(vk::Format::eR32G32B32A32Sfloat);
        attribDescBoneWeights.setOffset(currentOffset);

        attribs.push_back(attribDescBoneWeights);
        currentOffset += 16;
        ++currentLocation;
    }

	mVertexAttribs = std::move(attribs);
	mVertexDescription = std::move(bindingDesc);
}


void GraphicsPipeline::setFrameBufferBlendStates(const RenderTask& task)
{
	std::vector<vk::PipelineColorBlendAttachmentState> blendState;

	const auto& outputAttachments = task.getOuputAttachments();

	const auto blendAdjuster = [](const BlendMode op)
	{
		vk::BlendOp adjustedOp;

		switch (op)
		{
		case BlendMode::Add:
			adjustedOp = vk::BlendOp::eAdd;
			break;

		case BlendMode::Subtract:
			adjustedOp = vk::BlendOp::eSubtract;
			break;

		case BlendMode::ReverseSubtract:
			adjustedOp = vk::BlendOp::eReverseSubtract;
			break;

		case BlendMode::Min:
			adjustedOp = vk::BlendOp::eMin;
			break;

		case BlendMode::Max:
			adjustedOp = vk::BlendOp::eMax;
			break;

		default:
			adjustedOp = vk::BlendOp::eAdd;
		}

		return adjustedOp;
	};

    for (const auto& [name, type, format, size, loadOp, storeOp, usage] : outputAttachments)
	{
		// Don't generate blend info for depth attachments.
		if (format == Format::D32Float || format == Format::D24S8Float)
			continue;

		const vk::BlendOp alphaBlendOp = blendAdjuster(mPipelineDescription.mAlphaBlendMode);
		const vk::BlendOp colourBlendop = blendAdjuster(mPipelineDescription.mColourBlendMode);

		vk::PipelineColorBlendAttachmentState colorAttachState{};
		// write to all components by default.
		colorAttachState.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA); // write to all color components
		colorAttachState.setBlendEnable(mPipelineDescription.mColourBlendMode != BlendMode::None);
		colorAttachState.setColorBlendOp(colourBlendop);
		colorAttachState.setAlphaBlendOp(alphaBlendOp);
		colorAttachState.setSrcColorBlendFactor(vk::BlendFactor::eSrcColor);
		colorAttachState.setDstColorBlendFactor(vk::BlendFactor::eDstColor);
		colorAttachState.setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha);
		colorAttachState.setDstAlphaBlendFactor(vk::BlendFactor::eDstAlpha);

		blendState.push_back(colorAttachState);
	}

	mBlendStates = std::move(blendState);
}
