#include "Engine/DFGGenerationTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"


DFGGenerationTechnique::DFGGenerationTechnique(RenderEngine* eng, RenderGraph& graph) :
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

void DFGGenerationTechnique::render(RenderGraph& graph, RenderEngine*)
{
	mDFGLUT->updateLastAccessed();
	mDFGLUTView->updateLastAccessed();

	ComputeTask& task = static_cast<ComputeTask&>(graph.getTask(mTaskID));

	if (mFirstFrame)
	{
		task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine*, const std::vector<const MeshInstance*>&)
			{
				PROFILER_EVENT("DFG generation");
				PROFILER_GPU_TASK(exec);
				PROFILER_GPU_EVENT("DFG generation");

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
            [](const RenderGraph&, const uint32_t, Executor*, RenderEngine*, const std::vector<const MeshInstance*>&)
			{
				return;
			}
		);
	}
}
