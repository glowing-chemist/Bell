#include "Engine/LightFroxelationTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

constexpr const char* kFroxelIndirectArgs = "FroxelIndirectArgs";
constexpr const char* kLightIndexCounter = "lightIndexCounter";
constexpr const char* kActiveFroxelsCounter = "ActiveFroxelsCounter";

static_assert(sizeof(vk::DispatchIndirectCommand) == sizeof(uint32_t[3]), "Indirect args struct does not match required shader layout");

LightFroxelationTechnique::LightFroxelationTechnique(Engine* eng) :
	Technique("LightFroxelation", eng->getDevice()),
	mActiveFroxelsDesc{eng->getShader("./Shaders/ActiveFroxels.comp")},
	mActiveFroxels("LightingFroxelation", mActiveFroxelsDesc),
	mIndirectArgsDesc{ eng->getShader("./Shaders/IndirectFroxelArgs.comp") },
	mIndirectArgs("generateFroxelIndirectArgs", mIndirectArgsDesc),
    mClearCountersDesc{eng->getShader("./Shaders/LightFroxelationClearCounters.comp")},
    mClearCounters("clearFroxelationCounters", mClearCountersDesc),
    mLightAsignmentDesc{eng->getShader("./Shaders/FroxelationGenerateLightLists.comp")},
    mLightListAsignment("LightAsignment", mLightAsignmentDesc),

	mActiveFroxelsImage(eng->getDevice(), Format::R32Uint, ImageUsage::Storage | ImageUsage::Sampled, eng->getDevice()->getSwapChainImageView().getImageExtent().width,
		eng->getDevice()->getSwapChainImageView().getImageExtent().height, 1, 1, 1, 1, "ActiveFroxels"),
	mActiveFroxelsImageView(mActiveFroxelsImage, ImageViewType::Colour),

    // Assumes an avergae max of 10 active froxels per screen space tile.
    mActiveFroxlesBuffer(eng->getDevice(), BufferUsage::DataBuffer | BufferUsage::Uniform, sizeof(uint32_t) * 3 * (30 * 50 * 10), sizeof(uint32_t) * 3 * (30 * 50 * 10), "ActiveFroxelBuffer"),
	mActiveFroxlesBufferView(mActiveFroxlesBuffer, std::max(eng->getDevice()->getLimits().minStorageBufferOffsetAlignment, sizeof(uint32_t))),
    mActiveFroxelsCounter(mActiveFroxlesBuffer, 0u, static_cast<uint32_t>(sizeof(uint32_t))),

    mIndirectArgsBuffer(eng->getDevice(), BufferUsage::DataBuffer | BufferUsage::IndirectArgs, sizeof(vk::DispatchIndirectCommand), sizeof(vk::DispatchIndirectCommand), "FroxelIndirectArgs"),
    mIndirectArgsView(mIndirectArgsBuffer, 0, sizeof(vk::DispatchIndirectCommand)),

    mSparseFroxelBuffer(eng->getDevice(), BufferUsage::DataBuffer, sizeof(uint32_t) * 2 * (30 * 50 * 32), sizeof(uint32_t) * 2 * (30 * 50 * 32), kSparseFroxels),
    mSparseFroxelBufferView(mSparseFroxelBuffer),

    mLightIndexBuffer(eng->getDevice(), BufferUsage::DataBuffer, sizeof(uint32_t) * (30 * 50 * 10 * 16), sizeof(uint32_t) * (30 * 50 * 10 * 16), kLightIndicies),
    mLightIndexBufferView(mLightIndexBuffer, std::max(eng->getDevice()->getLimits().minStorageBufferOffsetAlignment, sizeof(uint32_t))),
    mLightIndexCounterView(mLightIndexBuffer, 0, static_cast<uint32_t>(sizeof(uint32_t)))
{
    mClearCounters.addInput(kActiveFroxelsCounter, AttachmentType::DataBufferWO);
    mClearCounters.addInput(kLightIndexCounter, AttachmentType::DataBufferWO);
    mClearCounters.addDispatch(1, 1, 1);

	mActiveFroxels.addInput(kActiveFroxels, AttachmentType::Image2D);
	mActiveFroxels.addInput(kGBufferDepth, AttachmentType::Texture2D);
	mActiveFroxels.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	mActiveFroxels.addInput(kDefaultSampler, AttachmentType::Sampler);
    mActiveFroxels.addInput(kActiveFroxelsCounter, AttachmentType::DataBufferRW);
    mActiveFroxels.addInput(kActiveFroxelBuffer, AttachmentType::DataBufferWO);

    mIndirectArgs.addInput(kActiveFroxelsCounter, AttachmentType::DataBufferRO);
    mIndirectArgs.addInput(kFroxelIndirectArgs, AttachmentType::DataBufferWO);
	mIndirectArgs.addDispatch(1, 1, 1);

    mLightListAsignment.addInput(kActiveFroxelsCounter, AttachmentType::DataBufferRO);
    mLightListAsignment.addInput(kActiveFroxelBuffer, AttachmentType::DataBufferRO);
    mLightListAsignment.addInput(kLightIndexCounter, AttachmentType::DataBufferRW);
    mLightListAsignment.addInput(kLightIndicies, AttachmentType::DataBufferWO);
    mLightListAsignment.addInput(kSparseFroxels, AttachmentType::DataBufferWO);
    mLightListAsignment.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    mLightListAsignment.addInput(kLightBuffer, AttachmentType::ShaderResourceSet);
    mLightListAsignment.addInput(kFroxelIndirectArgs, AttachmentType::IndirectBuffer);
    mLightListAsignment.addIndirectDispatch(kFroxelIndirectArgs);
}


void LightFroxelationTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance*>&)
{
	mActiveFroxels.clearCalls();

	const auto extent = eng->getDevice()->getSwapChainImageView().getImageExtent();

    mActiveFroxels.addDispatch(static_cast<uint32_t>(std::ceil(extent.width / 32.0f)), static_cast<uint32_t>(std::ceil(extent.height / 32.0f)), 1);

    graph.addTask(mClearCounters);
	graph.addTask(mActiveFroxels);
	graph.addTask(mIndirectArgs);
    graph.addTask(mLightListAsignment);

    mActiveFroxelsImageView->updateLastAccessed();
    mActiveFroxelsImage->updateLastAccessed();
    mActiveFroxlesBuffer->updateLastAccessed();
    mSparseFroxelBuffer->updateLastAccessed();
    mLightIndexBuffer->updateLastAccessed();
    mIndirectArgsBuffer->updateLastAccessed();
}


void LightFroxelationTechnique::bindResources(RenderGraph& graph) const
{
    graph.bindImage(kActiveFroxels, *mActiveFroxelsImageView);
    graph.bindBuffer(kActiveFroxelBuffer, *mActiveFroxlesBufferView);
    graph.bindBuffer(kSparseFroxels, *mSparseFroxelBufferView);
    graph.bindBuffer(kLightIndicies, *mLightIndexBufferView);
    graph.bindBuffer(kActiveFroxelsCounter, *mActiveFroxelsCounter);
    graph.bindBuffer(kFroxelIndirectArgs, *mIndirectArgsView);
    graph.bindBuffer(kLightIndexCounter, *mLightIndexCounterView);
}
