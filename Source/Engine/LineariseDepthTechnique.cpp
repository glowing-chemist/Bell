#include "Engine/LineariseDepthTechnique.hpp"
#include "Engine/Engine.hpp"


LineariseDepthTechnique::LineariseDepthTechnique(Engine* eng, RenderGraph& graph) :
	Technique("linearise depth", eng->getDevice()),
	mPipelienDesc{eng->getShader("./Shaders/LineariseDepth.comp")},
	mTaskID{0},
	mLinearDepth(eng->getDevice(), Format::R32Float, ImageUsage::Sampled | ImageUsage::Storage, eng->getSwapChainImage()->getExtent(0, 0).width, eng->getSwapChainImage()->getExtent(0, 0).height,
		1, 1, 1, 1, "Linear Depth"),
	mLinearDepthView(mLinearDepth, ImageViewType::Colour)
{
	ComputeTask task{ "linearise depth", mPipelienDesc };
	task.addInput(kLinearDepth, AttachmentType::Image2D);
	task.addInput(kGBufferDepth, AttachmentType::Texture2D);
	task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	task.addInput(kDefaultSampler, AttachmentType::Sampler);

	mTaskID = graph.addTask(task);
}


void LineariseDepthTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance*>&)
{
	ComputeTask& task = static_cast<ComputeTask&>(graph.getTask(mTaskID));
	task.clearCalls();

	const auto extent = eng->getDevice()->getSwapChainImageView()->getImageExtent();
	task.addDispatch(std::ceil(extent.width / 32.0f), std::ceil(extent.height / 32.0f), 1);
}
