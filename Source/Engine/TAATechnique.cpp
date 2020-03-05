#include "Engine/TAATechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"


TAATechnique::TAATechnique(Engine* eng) :
	Technique("TAA", eng->getDevice()),
	mHistoryImage(	eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled | ImageUsage::TransferDest, 
					eng->getSwapChainImage()->getExtent(0, 0).width, 
					eng->getSwapChainImage()->getExtent(0, 0).height,
					1, 1, 1, 1, "TAA history"),
	mHistoryImageView(mHistoryImage, ImageViewType::Colour),
	mNextHistoryImage(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled | ImageUsage::TransferDest,
		eng->getSwapChainImage()->getExtent(0, 0).width,
		eng->getSwapChainImage()->getExtent(0, 0).height,
		1, 1, 1, 1, "next TAA history"),
	mNextHistoryImageView(mNextHistoryImage, ImageViewType::Colour),
    mTAASAmpler(SamplerType::Linear),
	mPipeline{eng->getShader("./Shaders/TAAResolve.comp")},
	mFirstFrame{true}
{
    mTAASAmpler.setAddressModeU(AddressMode::Clamp);
    mTAASAmpler.setAddressModeV(AddressMode::Clamp);
    mTAASAmpler.setAddressModeW(AddressMode::Clamp);
}


void TAATechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance*>&)
{
	if (mFirstFrame)
	{
		mHistoryImage->clear();
		mFirstFrame = false;
	}

	ComputeTask ResolveTAA{"Resolve TAA", mPipeline };
	ResolveTAA.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	ResolveTAA.addInput(kGBufferDepth, AttachmentType::Texture2D);
    ResolveTAA.addInput(kGBufferVelocity, AttachmentType::Texture2D);
	ResolveTAA.addInput(kCompositeOutput, AttachmentType::Texture2D);
	ResolveTAA.addInput(kTAAHistory, AttachmentType::Texture2D);
	ResolveTAA.addInput(kNewTAAHistory, AttachmentType::Image2D);
    ResolveTAA.addInput("TAASampler", AttachmentType::Sampler);

	const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
	const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;
	ResolveTAA.addDispatch(static_cast<uint32_t>(std::ceil(threadGroupWidth / 32.0f)),
							static_cast<uint32_t>(std::ceil(threadGroupHeight / 32.0f)),
							1);

	graph.addTask(ResolveTAA);
}


void TAATechnique::bindResources(RenderGraph& graph)
{
	const uint64_t submissionIndex = getDevice()->getCurrentSubmissionIndex();

	// Ping-Pong.
	if ((submissionIndex % 2) == 0)
	{
		graph.bindImage(kTAAHistory, mHistoryImageView);
		graph.bindImage(kNewTAAHistory, mNextHistoryImageView);
	}
	else
	{
		graph.bindImage(kTAAHistory, mNextHistoryImageView);
		graph.bindImage(kNewTAAHistory, mHistoryImageView);
	}

    graph.bindSampler("TAASampler", mTAASAmpler);
}
