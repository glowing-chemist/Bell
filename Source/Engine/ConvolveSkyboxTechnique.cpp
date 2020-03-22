#include "Engine/ConvolveSkyboxTechnique.hpp"
#include "Engine/Engine.hpp"

ConvolveSkyBoxTechnique::ConvolveSkyBoxTechnique(Engine* eng) :
	Technique("convolveskybox", eng->getDevice()),
	mPipelineDesc{eng->getShader("./Shaders/SkyBoxConvolve.comp")},
	mConvolvedSkybox(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::CubeMap | ImageUsage::Sampled | ImageUsage::Storage,
                     512, 512, 1, 10, 6, 1, "convolved skybox"),
    mConvolvedView(mConvolvedSkybox, ImageViewType::CubeMap, 0, 6, 0, 10),
	mFirstFrame(true)
{

}


void ConvolveSkyBoxTechnique::render(RenderGraph& graph, Engine*, const std::vector<const Scene::MeshInstance *>&)
{
	mConvolvedSkybox->updateLastAccessed();
	mConvolvedView->updateLastAccessed();

	if(mFirstFrame)
	{
		std::vector<ImageView> convolvedMips{};
		for(uint32_t i = 0; i < 10; ++i)
		{
			convolvedMips.push_back(ImageView{mConvolvedSkybox, ImageViewType::Colour, 0, 6, i});
		}

		ComputeTask convolveTask("skybox convolve", mPipelineDesc);
		convolveTask.addInput(kSkyBox, AttachmentType::Texture2D);
		convolveTask.addInput(kDefaultSampler, AttachmentType::Sampler);

		const char* slots[] = {"convolved0", "convolved1", "convolved2", "convolved3", "convolved4", "convolved5", "convolved6", "convolved7", "convolved8", "convolved9"};
		for(uint32_t i = 0; i < 10; ++i)
		{
			convolveTask.addInput(slots[i], AttachmentType::Image2D);
		}
		convolveTask.addInput(kConvolvedSkyBox, AttachmentType::Image2D);

		convolveTask.addDispatch(64, 64, 1);

		graph.addTask(convolveTask);

		for(uint32_t i = 0; i < 10; ++i)
		{
			graph.bindImage(slots[i], convolvedMips[i]);
		}

		mFirstFrame = false;
	}
}
