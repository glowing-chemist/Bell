#include "Engine/ConvolveSkyboxTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"


constexpr const char* slots[] = { "convolved0", "convolved1", "convolved2", "convolved3", "convolved4", "convolved5", "convolved6", "convolved7", "convolved8", "convolved9" };


ConvolveSkyBoxTechnique::ConvolveSkyBoxTechnique(RenderEngine* eng, RenderGraph& graph) :
	Technique("convolveskybox", eng->getDevice()),
    mConvolveSkyboxShader(eng->getShader("./Shaders/SkyBoxConvolve.comp")),
    mConvolvedSpecularSkybox(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::CubeMap | ImageUsage::Sampled | ImageUsage::Storage,
                     512, 512, 1, 10, 6, 1, "convolved skybox specular"),
    mConvolvedSpecularView(mConvolvedSpecularSkybox, ImageViewType::CubeMap, 0, 6, 0, 10),
    mConvolvedDiffuseSkybox(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::CubeMap | ImageUsage::Sampled | ImageUsage::Storage,
                     512, 512, 1, 1, 6, 1, "convolved skybox diffuse"),
    mConvolvedDiffuseView(mConvolvedDiffuseSkybox, ImageViewType::CubeMap, 0, 6),
    mConvolvedMips{},
	mFirstFrame(true)
{
	for (uint32_t i = 0; i < 10; ++i)
	{
        mConvolvedMips.push_back(ImageView{ mConvolvedSpecularSkybox, ImageViewType::Colour, 0, 6, i });
	}

    ComputeTask convolveTask("skybox convolve");
    convolveTask.addInput(kSkyBox, AttachmentType::CubeMap);
	convolveTask.addInput(kDefaultSampler, AttachmentType::Sampler);

	for (uint32_t i = 0; i < 10; ++i)
	{
        convolveTask.addInput(slots[i], AttachmentType::Image2D);
	}
    convolveTask.addInput(kConvolvedDiffuseSkyBox, AttachmentType::Image2D);
    convolveTask.addInput(kConvolvedSpecularSkyBox, AttachmentType::Image2D);

	mTaskID = graph.addTask(convolveTask);
}


void ConvolveSkyBoxTechnique::render(RenderGraph& graph, RenderEngine*)
{
    mConvolvedSpecularSkybox->updateLastAccessed();
    mConvolvedSpecularView->updateLastAccessed();
    mConvolvedDiffuseSkybox->updateLastAccessed();
    mConvolvedDiffuseView->updateLastAccessed();

	ComputeTask& convolveTask = static_cast<ComputeTask&>(graph.getTask(mTaskID));

	if(mFirstFrame)
	{
		convolveTask.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine*, const std::vector<const MeshInstance*>&)
			{
                PROFILER_EVENT("Convolve skybox");
                PROFILER_GPU_TASK(exec);
                PROFILER_GPU_EVENT("Convolve skybox");

                const RenderTask& task = graph.getTask(taskIndex);
                exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mConvolveSkyboxShader);

				exec->dispatch(64, 64, 1);
			}
		);

		mFirstFrame = false;
	}
	else
	{
		convolveTask.setRecordCommandsCallback(
            [](const RenderGraph&, const uint32_t, Executor*, RenderEngine*, const std::vector<const MeshInstance*>&)
			{
				return;
			}
		);
	}
}


void ConvolveSkyBoxTechnique::bindResources(RenderGraph &graph)
{
    if(!graph.isResourceSlotBound(kConvolvedSpecularSkyBox))
    {
        for (uint32_t i = 0; i < 10; ++i)
        {
            graph.bindImage(slots[i], mConvolvedMips[i], BindingFlags::ManualBarriers);
        }

        graph.bindImage(kConvolvedSpecularSkyBox, mConvolvedSpecularView);
        graph.bindImage(kConvolvedDiffuseSkyBox, mConvolvedDiffuseView);
    }
}
