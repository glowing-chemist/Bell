#include "Engine/DFGGenerationTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"


DFGGenerationTechnique::DFGGenerationTechnique(Engine* eng, RenderGraph& graph) :
	Technique("DFGGeneration", eng->getDevice()),
	mPipelineDesc{ eng->getShader("./Shaders/DFGLutGenerate.comp") },
    mDFGLUT(eng->getDevice(), Format::RGBA16UNorm, ImageUsage::Sampled | ImageUsage::Storage, 512, 512, 1, 1, 1, 1, "DFGLUT"),
	mDFGLUTView(mDFGLUT, ImageViewType::Colour),
	mFirstFrame(true)
{
	ComputeTask task{ "DFGGeneration", mPipelineDesc };
	task.addInput(kDFGLUT, AttachmentType::Image2D);

	mTaskID = graph.addTask(task);

}

void DFGGenerationTechnique::render(RenderGraph& graph, Engine*)
{
	mDFGLUT->updateLastAccessed();
	mDFGLUTView->updateLastAccessed();

	ComputeTask& task = static_cast<ComputeTask&>(graph.getTask(mTaskID));

	if (mFirstFrame)
	{
		task.setRecordCommandsCallback(
            [](Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
			{
				exec->dispatch(32, 32, 1);
			}
		);

		mFirstFrame = false;
	}
	else
	{
		task.setRecordCommandsCallback(
            [](Executor*, Engine*, const std::vector<const MeshInstance*>&)
			{
				return;
			}
		);
	}
}
