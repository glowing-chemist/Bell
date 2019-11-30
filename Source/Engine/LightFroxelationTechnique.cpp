#include "Engine/LightFroxelationTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"


LightFroxelationTechnique::LightFroxelationTechnique(Engine* eng) :
	Technique("LightFroxelation", eng->getDevice()),
	mActiveFroxelsDesc{eng->getShader("./Shaders/ActiveFroxels.comp")},
	mActiveFroxels("LightingFroxelation", mActiveFroxelsDesc),
	mActiveFroxelsImage(eng->getDevice(), Format::RGBA16UInt, ImageUsage::Storage | ImageUsage::Sampled, eng->getDevice()->getSwapChainImageView().getImageExtent().width,
		eng->getDevice()->getSwapChainImageView().getImageExtent().height, 1, 1, 1, 1, "ActiveFroxels"),
	mActiveFroxelsImageView(mActiveFroxelsImage, ImageViewType::Colour)
{
	mActiveFroxels.addInput(kActiveFroxels, AttachmentType::Image2D);
	mActiveFroxels.addInput(kGBufferDepth, AttachmentType::Texture2D);
	mActiveFroxels.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	mActiveFroxels.addInput(kDefaultSampler, AttachmentType::Sampler);
}


void LightFroxelationTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance*>&)
{
	mActiveFroxels.clearCalls();

	const auto extent = eng->getDevice()->getSwapChainImageView().getImageExtent();

	mActiveFroxels.addDispatch(std::ceil(extent.width / 16.0f), std::ceil(extent.height / 16.0f), 1);

	graph.addTask(mActiveFroxels);
}


void LightFroxelationTechnique::bindResources(RenderGraph& graph) const
{
	graph.bindImage(kActiveFroxels, mActiveFroxelsImageView);
}
