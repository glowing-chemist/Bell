#include "Core/Pipeline.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/BellLogging.hpp"

Pipeline::Pipeline(RenderDevice* dev) :
	DeviceChild{ dev } {}


bool ComputePipeline::compile(const RenderTask&)
{
	if (!mPipelineDescription.mComputeShader.hasBeenCompiled())
	{
		mPipelineDescription.mComputeShader.compile();
		BELL_LOG("Compiling shaders at pipeline compile time, possible stall")
	}

	vk::PipelineShaderStageCreateInfo computeShaderInfo{};
	computeShaderInfo.setModule(mPipelineDescription.mComputeShader.getShaderModule());
	computeShaderInfo.setPName("main");
	computeShaderInfo.setStage(vk::ShaderStageFlagBits::eCompute);

	vk::ComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.setStage(computeShaderInfo);
	pipelineInfo.setLayout(mPipelineLayout);

	mPipeline = getDevice()->createPipeline(pipelineInfo);

	return true;
}


bool GraphicsPipeline::compile(const RenderTask& task)
{
	std::vector<vk::PipelineShaderStageCreateInfo> shaderInfo = getDevice()->generateShaderStagesInfo(static_cast<const GraphicsTask&>(task));

	std::vector<vk::PipelineShaderStageCreateInfo> indexedShaderInfo;
	if (mPipelineDescription.mIndexedVertexShader)
	{
		indexedShaderInfo = getDevice()->generateIndexedShaderStagesInfo(static_cast<const GraphicsTask&>(task));
	}

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

	vk::PipelineRasterizationStateCreateInfo rasterInfo{};
	rasterInfo.setRasterizerDiscardEnable(false);
	rasterInfo.setDepthBiasClamp(false);
	rasterInfo.setPolygonMode(vk::PolygonMode::eFill); // output filled in fragments
	rasterInfo.setLineWidth(1.0f);
	if (mPipelineDescription.mUseBackFaceCulling) {
		rasterInfo.setCullMode(vk::CullModeFlagBits::eBack); // cull fragments from the back
		rasterInfo.setFrontFace(vk::FrontFace::eClockwise);
	}
	else {
		rasterInfo.setCullMode(vk::CullModeFlagBits::eNone);
	}
	rasterInfo.setDepthBiasEnable(false);

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.setVertexAttributeDescriptionCount(static_cast<uint32_t>(mVertexAttribs.size()));
	vertexInputInfo.setPVertexAttributeDescriptions(mVertexAttribs.data());
	vertexInputInfo.setVertexBindingDescriptionCount(1);
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

	mPipeline = getDevice()->createPipeline(pipelineCreateInfo);

	// Now resuse most of the state to create the index variant
	pipelineCreateInfo.setStageCount(static_cast<uint32_t>(indexedShaderInfo.size()));
	pipelineCreateInfo.setPStages(indexedShaderInfo.data());	
	mIndexedVariant = getDevice()->createPipeline(pipelineCreateInfo);

	return true;
}


void GraphicsPipeline::setVertexAttributes(const int vertexInputs)
{
	const bool hasPosition2 = vertexInputs & VertexAttributes::Position2;
	const bool hasPosition3 = vertexInputs & VertexAttributes::Position3;
	const bool hasPosition4 = vertexInputs & VertexAttributes::Position4;
	const bool hasTextureCoords = vertexInputs & VertexAttributes::TextureCoordinates;
	const bool hasNormals = vertexInputs & VertexAttributes::Normals;
	const bool hasAlbedo = vertexInputs & VertexAttributes::Albedo;
	const bool hasMaterial = vertexInputs & VertexAttributes::Material;

	uint32_t positionSize = 0;
	if (hasPosition2)
		positionSize = 8;
	else if (hasPosition3)
		positionSize = 12;
	else if (hasPosition4)
		positionSize = 16;

	const uint32_t vertexStride = positionSize + (hasTextureCoords ? 8 : 0) + (hasNormals ? 16 : 0) + (hasAlbedo ? 4 : 0) + (hasMaterial ? 4 : 0);

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
		attribDescNormal.setFormat(vk::Format::eR32G32B32A32Sfloat);
		attribDescNormal.setOffset(currentOffset);

		attribs.push_back(attribDescNormal);
		currentOffset += 16;
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

	if (hasMaterial)
	{
		vk::VertexInputAttributeDescription attribDescMaterial{};
		attribDescMaterial.setBinding(0);
		attribDescMaterial.setLocation(currentLocation);
		attribDescMaterial.setFormat(vk::Format::eR32Uint);
		attribDescMaterial.setOffset(currentOffset);

		attribs.push_back(attribDescMaterial);
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

	for (const auto& [name, type, format, loadOp] : outputAttachments)
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