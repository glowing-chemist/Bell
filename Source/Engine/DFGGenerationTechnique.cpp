#include "Engine/DFGGenerationTechnique.hpp"
#include "Engine/Engine.hpp"


DFGGenerationTechnique::DFGGenerationTechnique(Engine* eng, RenderGraph& graph) :
	Technique("DFGGeneration", eng->getDevice()),
	mPipelineDesc{ eng->getShader("./Shaders/DFGLutGenerate.comp") },
	mDFGLUT(eng->getDevice(), Format::RG16UNorm, ImageUsage::Sampled | ImageUsage::Storage, 512, 512, 1, 1, 1, 1, "DFGLUT"),
	mDFGLUTView(mDFGLUT, ImageViewType::Colour),
	mFirstFrame(true)
{
	ComputeTask task{ "DFGGeneration", mPipelineDesc };
	task.addInput(kDFGLUT, AttachmentType::Image2D);

	mTaskID = graph.addTask(task);

}

void DFGGenerationTechnique::render(RenderGraph& graph, Engine*, const std::vector<const Scene::MeshInstance*>&)
{
	mDFGLUT->updateLastAccessed();
	mDFGLUTView->updateLastAccessed();

	ComputeTask& task = static_cast<ComputeTask&>(graph.getTask(mTaskID));
	task.clearCalls();

	if (mFirstFrame)
	{
		task.addDispatch(32, 32, 1);

		mFirstFrame = false;
	}
}