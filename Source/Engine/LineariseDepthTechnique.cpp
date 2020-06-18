#include "Engine/LineariseDepthTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"

constexpr const char* kLinearDepthMip1 = "linearDepth1";
constexpr const char* kLinearDepthMip2 = "linearDepth2";
constexpr const char* kLinearDepthMip3 = "linearDepth3";
constexpr const char* kLinearDepthMip4 = "linearDepth4";


LineariseDepthTechnique::LineariseDepthTechnique(Engine* eng, RenderGraph& graph) :
	Technique("linearise depth", eng->getDevice()),
	mPipelienDesc{eng->getShader("./Shaders/LineariseDepth.comp")},
	mTaskID{0},
	mLinearDepth(eng->getDevice(), Format::R32Float, ImageUsage::Sampled | ImageUsage::Storage, eng->getSwapChainImage()->getExtent(0, 0).width, eng->getSwapChainImage()->getExtent(0, 0).height,
        1, 5, 1, 1, "Linear Depth"),
    mLinearDepthView(mLinearDepth, ImageViewType::Colour, 0, 1, 0, 5),
    mLinearDepthViewMip1(mLinearDepth, ImageViewType::Colour, 0, 1, 1, 1),
    mLinearDepthViewMip2(mLinearDepth, ImageViewType::Colour, 0, 1, 2, 1),
    mLinearDepthViewMip3(mLinearDepth, ImageViewType::Colour, 0, 1, 3, 1),
    mLinearDepthViewMip4(mLinearDepth, ImageViewType::Colour, 0, 1, 4, 1)
{
	ComputeTask task{ "linearise depth", mPipelienDesc };
	task.addInput(kLinearDepth, AttachmentType::Image2D);
    task.addInput(kLinearDepthMip1, AttachmentType::Image2D);
    task.addInput(kLinearDepthMip2, AttachmentType::Image2D);
    task.addInput(kLinearDepthMip3, AttachmentType::Image2D);
    task.addInput(kLinearDepthMip4, AttachmentType::Image2D);
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
    mLinearDepthViewMip1->updateLastAccessed();
    mLinearDepthViewMip2->updateLastAccessed();
    mLinearDepthViewMip3->updateLastAccessed();
    mLinearDepthViewMip4->updateLastAccessed();

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
    if(!graph.isResourceSlotBound(kLinearDepth))
    {
        graph.bindImage(kLinearDepth, mLinearDepthView);
        graph.bindImage(kLinearDepthMip1, mLinearDepthViewMip1);
        graph.bindImage(kLinearDepthMip2, mLinearDepthViewMip2);
        graph.bindImage(kLinearDepthMip3, mLinearDepthViewMip3);
        graph.bindImage(kLinearDepthMip4, mLinearDepthViewMip4);
    }
}
