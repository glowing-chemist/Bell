#include "Engine/TAATechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"

constexpr const char* kTAASAmpler = "TAASampler";

TAATechnique::TAATechnique(Engine* eng, RenderGraph& graph) :
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
    mTAAShader(eng->getShader("./Shaders/TAAResolve.comp")),
	mFirstFrame{true}
{
    mTAASAmpler.setAddressModeU(AddressMode::Clamp);
    mTAASAmpler.setAddressModeV(AddressMode::Clamp);
    mTAASAmpler.setAddressModeW(AddressMode::Clamp);


    ComputeTask resolveTAA{ "Resolve TAA" };
	resolveTAA.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    resolveTAA.addInput(kGBufferDepth, AttachmentType::Texture2D);
	resolveTAA.addInput(kGBufferVelocity, AttachmentType::Texture2D);
	resolveTAA.addInput(kCompositeOutput, AttachmentType::Texture2D);
	resolveTAA.addInput(kTAAHistory, AttachmentType::Texture2D);
	resolveTAA.addInput(kNewTAAHistory, AttachmentType::Image2D);
	resolveTAA.addInput(kTAASAmpler, AttachmentType::Sampler);

	mTaskID = graph.addTask(resolveTAA);
}


void TAATechnique::render(RenderGraph& graph, Engine*)
{
	if (mFirstFrame)
	{
		mHistoryImage->clear();
		mFirstFrame = false;
	}

	ComputeTask& resolveTAA = static_cast<ComputeTask&>(graph.getTask(mTaskID));

	resolveTAA.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
		{
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mTAAShader);

			const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
			const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

			exec->dispatch(	static_cast<uint32_t>(std::ceil(threadGroupWidth / 32.0f)),
							static_cast<uint32_t>(std::ceil(threadGroupHeight / 32.0f)),
							1);
		}
	);
}


void TAATechnique::bindResources(RenderGraph& graph)
{
	const uint64_t submissionIndex = getDevice()->getCurrentSubmissionIndex();
	mHistoryImage->updateLastAccessed();
	mHistoryImageView->updateLastAccessed();
	mNextHistoryImage->updateLastAccessed();
	mNextHistoryImageView->updateLastAccessed();

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

    graph.bindSampler(kTAASAmpler, mTAASAmpler);
}
