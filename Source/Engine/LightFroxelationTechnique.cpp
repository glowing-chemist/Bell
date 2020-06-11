#include "Engine/LightFroxelationTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"

constexpr const char* kFroxelIndirectArgs = "FroxelIndirectArgs";
constexpr const char* kLightIndexCounter = "lightIndexCounter";
constexpr const char* kActiveFroxelsCounter = "ActiveFroxelsCounter";


LightFroxelationTechnique::LightFroxelationTechnique(Engine* eng, RenderGraph& graph) :
	Technique("LightFroxelation", eng->getDevice()),
	mActiveFroxelsDesc{eng->getShader("./Shaders/ActiveFroxels.comp")},
	mIndirectArgsDesc{ eng->getShader("./Shaders/IndirectFroxelArgs.comp") },
    mClearCountersDesc{eng->getShader("./Shaders/LightFroxelationClearCounters.comp")},
    mLightAsignmentDesc{eng->getShader("./Shaders/FroxelationGenerateLightLists.comp")},

    mXTiles(eng->getDevice()->getSwapChainImageView()->getImageExtent().width / 32),
    mYTiles(eng->getDevice()->getSwapChainImageView()->getImageExtent().height / 32),

	mActiveFroxelsImage(eng->getDevice(), Format::R32Uint, ImageUsage::Storage | ImageUsage::Sampled, eng->getDevice()->getSwapChainImageView()->getImageExtent().width,
		eng->getDevice()->getSwapChainImageView()->getImageExtent().height, 1, 1, 1, 1, "ActiveFroxels"),
	mActiveFroxelsImageView(mActiveFroxelsImage, ImageViewType::Colour),

    // Assumes an avergae max of 10 active froxels per screen space tile.
    mActiveFroxlesBuffer(eng->getDevice(), BufferUsage::DataBuffer | BufferUsage::Uniform, sizeof(float4) * (mXTiles * mYTiles * 10), sizeof(float4) * (mXTiles* mYTiles * 10), "ActiveFroxelBuffer"),
	mActiveFroxlesBufferView(mActiveFroxlesBuffer, std::max(eng->getDevice()->getMinStorageBufferAlignment(), sizeof(uint32_t))),
    mActiveFroxelsCounter(mActiveFroxlesBuffer, 0u, static_cast<uint32_t>(sizeof(uint32_t))),

    mIndirectArgsBuffer(eng->getDevice(), BufferUsage::DataBuffer | BufferUsage::IndirectArgs, sizeof(uint32_t) * 3, sizeof(uint32_t) * 3, "FroxelIndirectArgs"),
    mIndirectArgsView(mIndirectArgsBuffer, 0, sizeof(uint32_t) * 3),

    mSparseFroxelBuffer(eng->getDevice(), BufferUsage::DataBuffer, sizeof(float2) * (mXTiles * mYTiles * 32), sizeof(float2) * (mXTiles * mYTiles * 32), kSparseFroxels),
    mSparseFroxelBufferView(mSparseFroxelBuffer),

    mLightIndexBuffer(eng->getDevice(), BufferUsage::DataBuffer, sizeof(uint32_t) * (mXTiles * mYTiles * 16 * 16), sizeof(uint32_t) * (mXTiles * mYTiles * 16 * 16), kLightIndicies),
    mLightIndexBufferView(mLightIndexBuffer, std::max(eng->getDevice()->getMinStorageBufferAlignment(), sizeof(uint32_t))),
    mLightIndexCounterView(mLightIndexBuffer, 0, static_cast<uint32_t>(sizeof(uint32_t)))
{
    ComputeTask clearCountersTask{ "clearFroxelationCounters", mClearCountersDesc };
    clearCountersTask.addInput(kActiveFroxelsCounter, AttachmentType::DataBufferWO);
    clearCountersTask.addInput(kLightIndexCounter, AttachmentType::DataBufferWO);
    clearCountersTask.setRecordCommandsCallback(
        [](Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
        {
            exec->dispatch(1, 1, 1);
        }
    );
    mClearCounters = graph.addTask(clearCountersTask);

    ComputeTask activeFroxelTask{ "LightingFroxelation", mActiveFroxelsDesc };
    activeFroxelTask.addInput(kActiveFroxels, AttachmentType::Image2D);
    activeFroxelTask.addInput(kLinearDepth, AttachmentType::Texture2D);
    activeFroxelTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    activeFroxelTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    activeFroxelTask.addInput(kActiveFroxelBuffer, AttachmentType::DataBufferWO);
    activeFroxelTask.addInput(kActiveFroxelsCounter, AttachmentType::DataBufferRW);
    mActiveFroxels = graph.addTask(activeFroxelTask);

    ComputeTask indirectArgsTask{ "generateFroxelIndirectArgs", mIndirectArgsDesc };
    indirectArgsTask.addInput(kActiveFroxelsCounter, AttachmentType::DataBufferRO);
    indirectArgsTask.addInput(kFroxelIndirectArgs, AttachmentType::DataBufferWO);
    indirectArgsTask.setRecordCommandsCallback(
        [](Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
        {
            exec->dispatch(1, 1, 1);
        }
    );
    mIndirectArgs = graph.addTask(indirectArgsTask);

    ComputeTask lightListAsignmentTask{ "LightAsignment", mLightAsignmentDesc };
    lightListAsignmentTask.addInput(kActiveFroxelBuffer, AttachmentType::DataBufferRO);
    lightListAsignmentTask.addInput(kActiveFroxelsCounter, AttachmentType::DataBufferRO);
    lightListAsignmentTask.addInput(kLightIndicies, AttachmentType::DataBufferWO);
    lightListAsignmentTask.addInput(kLightIndexCounter, AttachmentType::DataBufferRW);
    lightListAsignmentTask.addInput(kSparseFroxels, AttachmentType::DataBufferWO);
    lightListAsignmentTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    lightListAsignmentTask.addInput(kLightBuffer, AttachmentType::ShaderResourceSet);
    lightListAsignmentTask.addInput(kFroxelIndirectArgs, AttachmentType::IndirectBuffer);
    lightListAsignmentTask.setRecordCommandsCallback(
        [this](Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
        {
            exec->dispatchIndirect(this->mIndirectArgsView);
        }
    );
    mLightListAsignment = graph.addTask(lightListAsignmentTask);
}


void LightFroxelationTechnique::render(RenderGraph& graph, Engine* eng)
{
    ComputeTask& activeFroxelsTask = static_cast<ComputeTask&>(graph.getTask(mActiveFroxels));

    activeFroxelsTask.setRecordCommandsCallback([](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            const auto extent = eng->getDevice()->getSwapChainImageView()->getImageExtent();

            exec->dispatch(static_cast<uint32_t>(std::ceil(extent.width / 32.0f)), static_cast<uint32_t>(std::ceil(extent.height / 32.0f)), 1);
        }
    );

    mActiveFroxelsImageView->updateLastAccessed();
    mActiveFroxelsImage->updateLastAccessed();
    mActiveFroxlesBuffer->updateLastAccessed();
    mSparseFroxelBuffer->updateLastAccessed();
    mLightIndexBuffer->updateLastAccessed();
    mIndirectArgsBuffer->updateLastAccessed();
}


void LightFroxelationTechnique::bindResources(RenderGraph& graph)
{
    graph.bindImage(kActiveFroxels, mActiveFroxelsImageView);
    graph.bindBuffer(kActiveFroxelBuffer, mActiveFroxlesBufferView);
    graph.bindBuffer(kSparseFroxels, mSparseFroxelBufferView);
    graph.bindBuffer(kLightIndicies, mLightIndexBufferView);
    graph.bindBuffer(kActiveFroxelsCounter, mActiveFroxelsCounter);
    graph.bindBuffer(kFroxelIndirectArgs, mIndirectArgsView);
    graph.bindBuffer(kLightIndexCounter, mLightIndexCounterView);
}
