#include "Engine/LightFroxelationTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"


LightFroxelationTechnique::LightFroxelationTechnique(Engine* eng) :
	Technique("LightFroxelation", eng->getDevice()),
	mActiveFroxelsDesc{eng->getShader("./Shaders/ActiveFroxels.comp")},
	mActiveFroxels("LightingFroxelation", mActiveFroxelsDesc),
	mIndirectArgsDesc{ eng->getShader("./Shaders/IndirectFroxelArgs.comp") },
	mIndirectArgs("generateFroxelIndirectArgs", mIndirectArgsDesc),
	mActiveFroxelsImage(eng->getDevice(), Format::R32Uint, ImageUsage::Storage | ImageUsage::Sampled, eng->getDevice()->getSwapChainImageView().getImageExtent().width,
		eng->getDevice()->getSwapChainImageView().getImageExtent().height, 1, 1, 1, 1, "ActiveFroxels"),
	mActiveFroxelsImageView(mActiveFroxelsImage, ImageViewType::Colour),
	// Assumes an avergae max of 10 active froxels per screen space tile.
	mActiveFroxlesBuffer(eng->getDevice(), BufferUsage::DataBuffer | BufferUsage::Uniform | BufferUsage::TransferDest, sizeof(uint32_t) * 3 * (28 * 50 * 10), sizeof(uint32_t) * 3 *(28 * 50 * 10), "ActiveFroxelBuffer"),
	mActiveFroxlesBufferView(mActiveFroxlesBuffer, eng->getDevice()->getLimits().minStorageBufferOffsetAlignment),
	mActiveFroxelsCounter(mActiveFroxlesBuffer, 0u, static_cast<uint32_t>(sizeof(uint32_t))),
	mIndirectArgsBuffer(eng->getDevice(), BufferUsage::DataBuffer | BufferUsage::IndirectArgs, sizeof(uint32_t) * 3, sizeof(uint32_t) * 3, "FroxelIndirectArgs"),
	mIndirectArgsView(mIndirectArgsBuffer)
{
	mActiveFroxels.addInput(kActiveFroxels, AttachmentType::Image2D);
	mActiveFroxels.addInput(kGBufferDepth, AttachmentType::Texture2D);
	mActiveFroxels.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	mActiveFroxels.addInput(kDefaultSampler, AttachmentType::Sampler);
	mActiveFroxels.addInput("ActiveFroxelBufferCounter", AttachmentType::DataBuffer);
	mActiveFroxels.addInput(kActiveFroxelBuffer, AttachmentType::DataBuffer);

	mIndirectArgs.addInput("ActiveFroxelBufferCounter", AttachmentType::UniformBuffer);
	mIndirectArgs.addInput("FroxelIndirectArgs", AttachmentType::DataBuffer);
	mIndirectArgs.addDispatch(1, 1, 1);

	uint32_t zeroCounter = 0;
	mActiveFroxlesBuffer.setContents(&zeroCounter, sizeof(uint32_t));
}


void LightFroxelationTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance*>&)
{
	mActiveFroxels.clearCalls();

	const auto extent = eng->getDevice()->getSwapChainImageView().getImageExtent();

	mActiveFroxels.addDispatch(std::ceil(extent.width / 32.0f), std::ceil(extent.height / 32.0f), 1);

	graph.addTask(mActiveFroxels);
	graph.addTask(mIndirectArgs);
}


void LightFroxelationTechnique::bindResources(RenderGraph& graph) const
{
	graph.bindImage(kActiveFroxels, mActiveFroxelsImageView);
	graph.bindBuffer(kActiveFroxelBuffer, mActiveFroxlesBufferView);
	graph.bindBuffer("ActiveFroxelBufferCounter", mActiveFroxelsCounter);
	graph.bindBuffer("FroxelIndirectArgs", mIndirectArgsView);
}
