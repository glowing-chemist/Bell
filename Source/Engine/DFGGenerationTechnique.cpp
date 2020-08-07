#include "Engine/DFGGenerationTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"


DFGGenerationTechnique::DFGGenerationTechnique(Engine* eng, RenderGraph& graph) :
	Technique("DFGGeneration", eng->getDevice()),
    mDFGGenerationShader( eng->getShader("./Shaders/DFGLutGenerate.comp") ),
    mDFGLUT(eng->getDevice(), Format::RGBA16UNorm, ImageUsage::Sampled | ImageUsage::Storage, 512, 512, 1, 1, 1, 1, "DFGLUT"),
	mDFGLUTView(mDFGLUT, ImageViewType::Colour),
	mFirstFrame(true)
{
    ComputeTask task{ "DFGGeneration" };
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
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
			{
                const RenderTask& task = graph.getTask(taskIndex);
                exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mDFGGenerationShader);

				exec->dispatch(32, 32, 1);
			}
		);

		mFirstFrame = false;
	}
	else
	{
		task.setRecordCommandsCallback(
            [](const RenderGraph&, const uint32_t, Executor*, Engine*, const std::vector<const MeshInstance*>&)
			{
				return;
			}
		);
	}
}
