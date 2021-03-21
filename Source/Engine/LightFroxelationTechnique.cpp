#include "Engine/LightFroxelationTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"

constexpr const char* kFroxelIndirectArgs = "FroxelIndirectArgs";
constexpr const char* kLightIndexCounter = "lightIndexCounter";
constexpr const char* kActiveFroxelsCounter = "ActiveFroxelsCounter";


LightFroxelationTechnique::LightFroxelationTechnique(RenderEngine* eng, RenderGraph& graph) :
	Technique("LightFroxelation", eng->getDevice()),
    mActiveFroxelsShader(eng->getShader("./Shaders/ActiveFroxels.comp")),
    mIndirectArgsShader(eng->getShader("./Shaders/IndirectFroxelArgs.comp")),
    mClearCoutersShader(eng->getShader("./Shaders/LightFroxelationClearCounters.comp")),
    mLightAsignmentShader(eng->getShader("./Shaders/FroxelationGenerateLightLists.comp")),

    mXTiles(eng->getDevice()->getSwapChainImageView()->getImageExtent().width / 32),
    mYTiles(eng->getDevice()->getSwapChainImageView()->getImageExtent().height / 32),

    mLightBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, (sizeof(Scene::Light) * 1000) + std::max(sizeof(uint4), getDevice()->getMinStorageBufferAlignment()), (sizeof(Scene::Light) * 1000) + std::max(sizeof(uint4), getDevice()->getMinStorageBufferAlignment()), "LightBuffer"),
    mLightBufferView(mLightBuffer, std::max(sizeof(uint4), getDevice()->getMinStorageBufferAlignment())),
    mLightCountView(mLightBuffer, 0, sizeof(uint4)),
    mLightsSRS(getDevice(), 2),

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
    // set light buffers SRS
    for(uint32_t i = 0; i < getDevice()->getSwapChainImageCount(); ++i)
    {
        mLightsSRS.get(i)->addDataBufferRO(mLightCountView.get(i));
        mLightsSRS.get(i)->addDataBufferRW(mLightBufferView.get(i));
        mLightsSRS.get(i)->finalise();
    }

    ComputeTask clearCountersTask{ "clearFroxelationCounters" };
    clearCountersTask.addInput(kActiveFroxelsCounter, AttachmentType::DataBufferWO);
    clearCountersTask.addInput(kLightIndexCounter, AttachmentType::DataBufferWO);
    clearCountersTask.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine*, const std::vector<const MeshInstance*>&)
        {
            PROFILER_EVENT("Clear froxel counter");
            PROFILER_GPU_TASK(exec);
            PROFILER_GPU_EVENT("Clear froxel counter");

            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mClearCoutersShader);

            exec->dispatch(1, 1, 1);
        }
    );
    mClearCounters = graph.addTask(clearCountersTask);

    ComputeTask activeFroxelTask{ "LightingFroxelation" };
    activeFroxelTask.addInput(kActiveFroxels, AttachmentType::Image2D);
    activeFroxelTask.addInput(kLinearDepth, AttachmentType::Texture2D);
    activeFroxelTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    activeFroxelTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    activeFroxelTask.addInput(kActiveFroxelBuffer, AttachmentType::DataBufferWO);
    activeFroxelTask.addInput(kActiveFroxelsCounter, AttachmentType::DataBufferRW);
    activeFroxelTask.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>&)
        {
            PROFILER_EVENT("light froxelation");
            PROFILER_GPU_TASK(exec);
            PROFILER_GPU_EVENT("light froxelation");

            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mActiveFroxelsShader);

            const auto extent = eng->getDevice()->getSwapChainImageView()->getImageExtent();

            exec->dispatch(static_cast<uint32_t>(std::ceil(extent.width / 32.0f)), static_cast<uint32_t>(std::ceil(extent.height / 32.0f)), 1);
        }
    );
    mActiveFroxels = graph.addTask(activeFroxelTask);

    ComputeTask indirectArgsTask{ "generateFroxelIndirectArgs" };
    indirectArgsTask.addInput(kActiveFroxelsCounter, AttachmentType::DataBufferRO);
    indirectArgsTask.addInput(kFroxelIndirectArgs, AttachmentType::DataBufferWO);
    indirectArgsTask.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine*, const std::vector<const MeshInstance*>&)
        {
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mIndirectArgsShader);

            exec->dispatch(1, 1, 1);
        }
    );
    mIndirectArgs = graph.addTask(indirectArgsTask);

    ComputeTask lightListAsignmentTask{ "LightAsignment" };
    lightListAsignmentTask.addInput(kActiveFroxelBuffer, AttachmentType::DataBufferRO);
    lightListAsignmentTask.addInput(kActiveFroxelsCounter, AttachmentType::DataBufferRO);
    lightListAsignmentTask.addInput(kLightIndicies, AttachmentType::DataBufferWO);
    lightListAsignmentTask.addInput(kLightIndexCounter, AttachmentType::DataBufferRW);
    lightListAsignmentTask.addInput(kSparseFroxels, AttachmentType::DataBufferWO);
    lightListAsignmentTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    lightListAsignmentTask.addInput(kLightBuffer, AttachmentType::ShaderResourceSet);
    lightListAsignmentTask.addInput(kFroxelIndirectArgs, AttachmentType::IndirectBuffer);
    lightListAsignmentTask.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine*, const std::vector<const MeshInstance*>&)
        {
            PROFILER_EVENT("build light lists");
            PROFILER_GPU_TASK(exec);
            PROFILER_GPU_EVENT("build light lists");

            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mLightAsignmentShader);

            exec->dispatchIndirect(this->mIndirectArgsView);
        }
    );
    mLightListAsignment = graph.addTask(lightListAsignmentTask);
}


void LightFroxelationTechnique::render(RenderGraph&, RenderEngine* engine)
{
    mActiveFroxelsImageView->updateLastAccessed();
    mActiveFroxelsImage->updateLastAccessed();
    mActiveFroxlesBuffer->updateLastAccessed();
    mSparseFroxelBuffer->updateLastAccessed();
    mLightIndexBuffer->updateLastAccessed();
    mIndirectArgsBuffer->updateLastAccessed();

    (*mLightBuffer)->updateLastAccessed();
    (*mLightsSRS)->updateLastAccessed();


    // Frustum cull the lights and send to the gpu.
    Frustum frustum = engine->getCurrentSceneCamera().getFrustum();
    std::vector<Scene::Light*> visibleLightPtrs = engine->getScene()->getVisibleLights(frustum);
    std::vector<Scene::Light> visibleLights(visibleLightPtrs.size());
    std::transform(visibleLightPtrs.begin(), visibleLightPtrs.end(), std::back_inserter(visibleLights), []
            (const Scene::Light* light) { return *light; });

    mLightBuffer.get()->setContents(static_cast<int>(visibleLights.size()), sizeof(uint32_t));

    if(!visibleLights.empty())
    {
        mLightBuffer.get()->resize(visibleLights.size() * sizeof(Scene::Light), false);
        mLightBuffer.get()->setContents(visibleLights.data(), static_cast<uint32_t>(visibleLights.size() * sizeof(Scene::Light)), std::max(sizeof(uint4), engine->getDevice()->getMinStorageBufferAlignment()));
    }
}


void LightFroxelationTechnique::bindResources(RenderGraph& graph)
{
    graph.bindShaderResourceSet(kLightBuffer, *mLightsSRS);

    if(!graph.isResourceSlotBound(kActiveFroxels))
    {
        graph.bindImage(kActiveFroxels, mActiveFroxelsImageView);
        graph.bindBuffer(kActiveFroxelBuffer, mActiveFroxlesBufferView);
        graph.bindBuffer(kSparseFroxels, mSparseFroxelBufferView);
        graph.bindBuffer(kLightIndicies, mLightIndexBufferView);
        graph.bindBuffer(kActiveFroxelsCounter, mActiveFroxelsCounter);
        graph.bindBuffer(kFroxelIndirectArgs, mIndirectArgsView);
        graph.bindBuffer(kLightIndexCounter, mLightIndexCounterView);
    }
}
