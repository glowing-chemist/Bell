#include "Engine/DownSampleColourTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"


DownSampleColourTechnique::DownSampleColourTechnique(Engine* eng, RenderGraph& graph) :
	Technique("Downsample colour", eng->getDevice()),
    mFirstFrame(true),
    mDowmSampledColour{Image(getDevice(), Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled | ImageUsage::TransferDest | ImageUsage::TransferSrc, eng->getSwapChainImage()->getExtent(0, 0).width,
        eng->getSwapChainImage()->getExtent(0, 0).height, 1, 5, 1, 1, "downsampled colour"),
                       Image(getDevice(), Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled | ImageUsage::TransferDest | ImageUsage::TransferSrc, eng->getSwapChainImage()->getExtent(0, 0).width,
                               eng->getSwapChainImage()->getExtent(0, 0).height, 1, 5, 1, 1, "prev downsampled colour")},
    mDownSampledColourViews{ImageView(mDowmSampledColour[0], ImageViewType::Colour, 0, 1, 0, 1),
                            ImageView(mDowmSampledColour[0], ImageViewType::Colour, 0, 1, 1, 1),
                            ImageView(mDowmSampledColour[0], ImageViewType::Colour, 0, 1, 2, 1),
                            ImageView(mDowmSampledColour[0], ImageViewType::Colour, 0, 1, 3, 1),
                            ImageView(mDowmSampledColour[0], ImageViewType::Colour, 0, 1, 4, 1),
                            ImageView(mDowmSampledColour[0], ImageViewType::Colour, 0, 1, 0, 5),
                            ImageView(mDowmSampledColour[1], ImageViewType::Colour, 0, 1, 0, 5),},
	mDowmSampledColourInternal(getDevice(), Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled | ImageUsage::TransferDest | ImageUsage::TransferSrc, eng->getSwapChainImage()->getExtent(0, 0).width,
		eng->getSwapChainImage()->getExtent(0, 0).height, 1, 5, 1, 1, "downsampled internal colour"),
	mDownSampledColourInternalViews{ImageView(mDowmSampledColourInternal, ImageViewType::Colour, 0, 1, 0, 1),
									ImageView(mDowmSampledColourInternal, ImageViewType::Colour, 0, 1, 1, 1),
									ImageView(mDowmSampledColourInternal, ImageViewType::Colour, 0, 1, 2, 1),
									ImageView(mDowmSampledColourInternal, ImageViewType::Colour, 0, 1, 3, 1),
									ImageView(mDowmSampledColourInternal, ImageViewType::Colour, 0, 1, 4, 1),
									ImageView(mDowmSampledColourInternal, ImageViewType::Colour, 0, 1, 0, 5) }
{
	ComputeTask downSampleColourTask("DownSample Colour");
	downSampleColourTask.addInput(kGlobalLighting, AttachmentType::TransferSource);
	downSampleColourTask.addInput(kDownSampledColour, AttachmentType::TransferDestination);
	downSampleColourTask.setRecordCommandsCallback([this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
	{
		// put internal resourece in top layout trasnferDst
		{
			BarrierRecorder barrier{ getDevice() };
			barrier->transitionLayout(mDownSampledColourInternalViews[5], ImageLayout::TransferDst, Hazard::ReadAfterWrite, SyncPoint::TopOfPipe, SyncPoint::TransferSource);
			exec->recordBarriers(barrier);
		}

		// Blit
		const ImageView& colour = graph.getImageView(kGlobalLighting);

		exec->blitImage(mDownSampledColourInternalViews[0], colour, SamplerType::Linear);

		// blit images to internal image to create mip chain.
		for (uint32_t i = 1; i < 5; ++i)
		{
			BarrierRecorder barrier{ getDevice() };
			barrier->transitionLayout(mDownSampledColourInternalViews[i - 1], ImageLayout::TransferSrc, Hazard::ReadAfterWrite, SyncPoint::TopOfPipe, SyncPoint::TransferSource);

			exec->recordBarriers(barrier);

			// Blit images.
			exec->blitImage(mDownSampledColourInternalViews[i], mDownSampledColourInternalViews[i - 1], SamplerType::Linear);
		}

		// Now blit from internal to the exported resource.
		{
			// First transition internal to transfer dest layout.
			BarrierRecorder barrier{ getDevice() };
			barrier->transitionLayout(mDownSampledColourInternalViews[5], ImageLayout::TransferSrc, Hazard::ReadAfterWrite, SyncPoint::TopOfPipe, SyncPoint::TransferSource);
			exec->recordBarriers(barrier);

            const uint64_t submissionOffset = getDevice()->getCurrentSubmissionIndex() % 2;
            exec->blitImage(mDownSampledColourViews[5 + submissionOffset], mDownSampledColourInternalViews[5], SamplerType::Linear);
		}
	});

	graph.addTask(downSampleColourTask);
}


void DownSampleColourTechnique::render(RenderGraph &, Engine *)
{
    if(mFirstFrame)
    {
        mDowmSampledColour[1]->clear(float4(0.f, 0.0f, 0.0f, 0.0f));
        mFirstFrame = false;
    }

    mDowmSampledColour[0]->updateLastAccessed();
    mDowmSampledColour[1]->updateLastAccessed();
    for(auto& view : mDownSampledColourViews)
        view->updateLastAccessed();

    mDowmSampledColourInternal->updateLastAccessed();
    for(auto& view : mDownSampledColourInternalViews)
        view->updateLastAccessed();
}


void DownSampleColourTechnique::bindResources(RenderGraph& graph)
{
    const uint64_t submissionIndex = getDevice()->getCurrentSubmissionIndex();

    if((submissionIndex % 2) == 0)
    {
		graph.bindImage(kDownSampledColour, mDownSampledColourViews[5]);
        graph.bindImage(kPreviousDownSampledColour, mDownSampledColourViews[6]);
	}
    else
    {
        graph.bindImage(kDownSampledColour, mDownSampledColourViews[6]);
        graph.bindImage(kPreviousDownSampledColour, mDownSampledColourViews[5]);
    }
}
