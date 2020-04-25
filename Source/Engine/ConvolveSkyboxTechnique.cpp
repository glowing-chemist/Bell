#include "Engine/ConvolveSkyboxTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"

ConvolveSkyBoxTechnique::ConvolveSkyBoxTechnique(Engine* eng, RenderGraph& graph) :
	Technique("convolveskybox", eng->getDevice()),
	mPipelineDesc{eng->getShader("./Shaders/SkyBoxConvolve.comp")},
    mConvolvedSpecularSkybox(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::CubeMap | ImageUsage::Sampled | ImageUsage::Storage,
                     512, 512, 1, 10, 6, 1, "convolved skybox"),
    mConvolvedSpecularView(mConvolvedSpecularSkybox, ImageViewType::CubeMap, 0, 6, 0, 10),
    mConvolvedDiffuseSkybox(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::CubeMap | ImageUsage::Sampled | ImageUsage::Storage,
                     512, 512, 1, 0, 6, 1, "convolved skybox"),
    mConvolvedDiffuseView(mConvolvedDiffuseSkybox, ImageViewType::CubeMap, 0, 6),
	mFirstFrame(true)
{
	std::vector<ImageView> convolvedMips{};
	for (uint32_t i = 0; i < 10; ++i)
	{
        convolvedMips.push_back(ImageView{ mConvolvedSpecularSkybox, ImageViewType::Colour, 0, 6, i });
	}
    ImageView diffuseskybox{mConvolvedDiffuseSkybox, ImageViewType::Colour, 0, 6};

	ComputeTask convolveTask("skybox convolve", mPipelineDesc);
	convolveTask.addInput(kSkyBox, AttachmentType::Texture2D);
	convolveTask.addInput(kDefaultSampler, AttachmentType::Sampler);

	const char* slots[] = { "convolved0", "convolved1", "convolved2", "convolved3", "convolved4", "convolved5", "convolved6", "convolved7", "convolved8", "convolved9" };
	for (uint32_t i = 0; i < 10; ++i)
	{
		convolveTask.addInput(slots[i], AttachmentType::Image2D);
	}
    convolveTask.addInput("diffuseSkybox", AttachmentType::Image2D);
    convolveTask.addInput(kConvolvedSpecularSkyBox, AttachmentType::Image2D);
    convolveTask.addInput(kConvolvedDiffuseSkyBox, AttachmentType::Image2D);

	mTaskID = graph.addTask(convolveTask);

	for (uint32_t i = 0; i < 10; ++i)
	{
		graph.bindImage(slots[i], convolvedMips[i]);
	}
    graph.bindImage("diffuseSkybox", diffuseskybox);
}


void ConvolveSkyBoxTechnique::render(RenderGraph& graph, Engine*)
{
    mConvolvedSpecularSkybox->updateLastAccessed();
    mConvolvedSpecularView->updateLastAccessed();
    mConvolvedDiffuseSkybox->updateLastAccessed();
    mConvolvedDiffuseView->updateLastAccessed();

	ComputeTask& convolveTask = static_cast<ComputeTask&>(graph.getTask(mTaskID));

	if(mFirstFrame)
	{
		convolveTask.setRecordCommandsCallback(
            [](Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
			{
				exec->dispatch(64, 64, 1);
			}
		);

		mFirstFrame = false;
	}
	else
	{
		convolveTask.setRecordCommandsCallback(
            [](Executor*, Engine*, const std::vector<const MeshInstance*>&)
			{
				return;
			}
		);
	}
}
