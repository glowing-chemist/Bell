#include "Engine/DFGGenerationTechnique.hpp"
#include "ENgine/Engine.hpp"


DFGGenerationTechnique::DFGGenerationTechnique(Engine* eng) :
	Technique("DFGGeneration", eng->getDevice()),
	mPipelineDesc{ eng->getShader("./Shaders/DFGLutGenerate.comp") },
	mTask("DFGGeneration", mPipelineDesc),
	mDFGLUT(eng->getDevice(), Format::RG16UNorm, ImageUsage::Sampled | ImageUsage::Storage, 512, 512, 1, 1, 1, 1, "DFGLUT"),
	mDFGLUTView(mDFGLUT, ImageViewType::Colour),
	mFirstFrame(true)
{
	mTask.addInput(kDFGLUT, AttachmentType::Image2D);
	mTask.addDispatch(32, 32, 1);
}

void DFGGenerationTechnique::render(RenderGraph& graph, Engine*, const std::vector<const Scene::MeshInstance*>&)
{
	if (mFirstFrame)
	{
		graph.addTask(mTask);

		mFirstFrame = false;
	}
}