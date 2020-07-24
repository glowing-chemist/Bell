#include "Engine/LineariseDepthTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"

constexpr const char* kLinearDepthMip1 = "linearDepth1";
constexpr const char* kLinearDepthMip2 = "linearDepth2";
constexpr const char* kLinearDepthMip3 = "linearDepth3";
constexpr const char* kLinearDepthMip4 = "linearDepth4";
constexpr const char* kLinearDepthMip5 = "linearDepth5";


LineariseDepthTechnique::LineariseDepthTechnique(Engine* eng, RenderGraph& graph) :
	Technique("linearise depth", eng->getDevice()),
	mPipelienDesc{eng->getShader("./Shaders/LineariseDepth.comp")},
	mTaskID{0},
	mLinearDepth(eng->getDevice(), Format::R32Float, ImageUsage::Sampled | ImageUsage::Storage, eng->getSwapChainImage()->getExtent(0, 0).width, eng->getSwapChainImage()->getExtent(0, 0).height,
        1, 6, 1, 1, "Linear Depth"),
    mLinearDepthView(mLinearDepth, ImageViewType::Colour, 0, 1, 0, 5),
    mPreviousLinearDepth(eng->getDevice(), Format::R32Float, ImageUsage::Sampled | ImageUsage::Storage, eng->getSwapChainImage()->getExtent(0, 0).width, eng->getSwapChainImage()->getExtent(0, 0).height,
        1, 6, 1, 1, "Linear Depth"),
    mPreviousLinearDepthView(mPreviousLinearDepth, ImageViewType::Colour, 0, 1, 0, 5),
    mLinearDepthViewMip1{ImageView(mLinearDepth, ImageViewType::Colour, 0, 1, 1, 1), ImageView(mPreviousLinearDepth, ImageViewType::Colour, 0, 1, 1, 1)},
    mLinearDepthViewMip2{ImageView(mLinearDepth, ImageViewType::Colour, 0, 1, 2, 1), ImageView(mPreviousLinearDepth, ImageViewType::Colour, 0, 1, 2, 1)},
    mLinearDepthViewMip3{ImageView(mLinearDepth, ImageViewType::Colour, 0, 1, 3, 1), ImageView(mPreviousLinearDepth, ImageViewType::Colour, 0, 1, 3, 1)},
    mLinearDepthViewMip4{ImageView(mLinearDepth, ImageViewType::Colour, 0, 1, 4, 1), ImageView(mPreviousLinearDepth, ImageViewType::Colour, 0, 1, 4, 1)},
    mLinearDepthViewMip5{ImageView(mLinearDepth, ImageViewType::Colour, 0, 1, 5, 1), ImageView(mPreviousLinearDepth, ImageViewType::Colour, 0, 1, 5, 1)}
{    
	ComputeTask task{ "linearise depth", mPipelienDesc };
	task.addInput(kLinearDepth, AttachmentType::Image2D);
    task.addInput(kLinearDepthMip1, AttachmentType::Image2D);
    task.addInput(kLinearDepthMip2, AttachmentType::Image2D);
    task.addInput(kLinearDepthMip3, AttachmentType::Image2D);
    task.addInput(kLinearDepthMip4, AttachmentType::Image2D);
    task.addInput(kLinearDepthMip5, AttachmentType::Image2D);
	task.addInput(kGBufferDepth, AttachmentType::Texture2D);
	task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	task.addInput(kDefaultSampler, AttachmentType::Sampler);

	mTaskID = graph.addTask(task);
}


void LineariseDepthTechnique::render(RenderGraph& graph, Engine*)
{
	ComputeTask& task = static_cast<ComputeTask&>(graph.getTask(mTaskID));

    mLinearDepth->updateLastAccessed();
    mLinearDepthView->updateLastAccessed();
    mLinearDepthViewMip1[0]->updateLastAccessed();
    mLinearDepthViewMip2[0]->updateLastAccessed();
    mLinearDepthViewMip3[0]->updateLastAccessed();
    mLinearDepthViewMip4[0]->updateLastAccessed();
    mLinearDepthViewMip5[0]->updateLastAccessed();
    mLinearDepthViewMip1[1]->updateLastAccessed();
    mLinearDepthViewMip2[1]->updateLastAccessed();
    mLinearDepthViewMip3[1]->updateLastAccessed();
    mLinearDepthViewMip4[1]->updateLastAccessed();
    mLinearDepthViewMip5[1]->updateLastAccessed();

	task.setRecordCommandsCallback(
		[](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
		{
			const auto extent = eng->getDevice()->getSwapChainImageView()->getImageExtent();
			exec->dispatch(std::ceil(extent.width / 32.0f), std::ceil(extent.height / 32.0f), 1);
		}
	);
}


void LineariseDepthTechnique::bindResources(RenderGraph& graph)
{
    const uint64_t submissionIndex = getDevice()->getCurrentSubmissionIndex();

    graph.bindImage(kLinearDepthMip1, mLinearDepthViewMip1[submissionIndex % 2]);
    graph.bindImage(kLinearDepthMip2, mLinearDepthViewMip2[submissionIndex % 2]);
    graph.bindImage(kLinearDepthMip3, mLinearDepthViewMip3[submissionIndex % 2]);
    graph.bindImage(kLinearDepthMip4, mLinearDepthViewMip4[submissionIndex % 2]);
    graph.bindImage(kLinearDepthMip5, mLinearDepthViewMip5[submissionIndex % 2]);

    if((submissionIndex % 2) == 0)
    {
        graph.bindImage(kLinearDepth, mLinearDepthView);
        graph.bindImage(kPreviousLinearDepth, mPreviousLinearDepthView);
    }
    else
    {
        graph.bindImage(kLinearDepth, mPreviousLinearDepthView);
        graph.bindImage(kPreviousLinearDepth, mLinearDepthView);
    }
}
