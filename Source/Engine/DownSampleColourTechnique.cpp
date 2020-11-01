#include "Engine/DownSampleColourTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"


DownSampleColourTechnique::DownSampleColourTechnique(Engine* eng, RenderGraph& graph) :
	Technique("Downsample colour", eng->getDevice()),
	mDowmSampledColour(getDevice(), Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled | ImageUsage::TransferDest | ImageUsage::TransferSrc, eng->getSwapChainImage()->getExtent(0, 0).width,
		eng->getSwapChainImage()->getExtent(0, 0).height, 1, 5, 1, 1, "downsampled colour"),
	mDownSampledColourViews{ImageView(mDowmSampledColour, ImageViewType::Colour, 0, 1, 0, 1),
							ImageView(mDowmSampledColour, ImageViewType::Colour, 0, 1, 1, 1),
							ImageView(mDowmSampledColour, ImageViewType::Colour, 0, 1, 2, 1),
							ImageView(mDowmSampledColour, ImageViewType::Colour, 0, 1, 3, 1),
							ImageView(mDowmSampledColour, ImageViewType::Colour, 0, 1, 4, 1),
							ImageView(mDowmSampledColour, ImageViewType::Colour, 0, 1, 0, 5)},
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

			exec->blitImage(mDownSampledColourViews[5], mDownSampledColourInternalViews[5], SamplerType::Point);
		}
	});

	graph.addTask(downSampleColourTask);
}


void DownSampleColourTechnique::bindResources(RenderGraph& graph)
{
	if (!graph.isResourceSlotBound(kDownSampledColour))
	{
		graph.bindImage(kDownSampledColour, mDownSampledColourViews[5]);
	}
}