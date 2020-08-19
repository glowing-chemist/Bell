#include "Engine/ScreenSpaceReflectionTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"


constexpr const char SSRSampler[] = "SSRSampler";

constexpr const char SSRRoughIndirectArgs[] = "SSRRoughArgs";
constexpr const char SSRSmoothIndirectArgs[] = "SSRSmoothArgs";
constexpr const char SSRRoughTileList[] = "SSRRoughTileList";
constexpr const char SSRRoughTileCounter[] = "SSRRoughTileCounter";
constexpr const char SSRSmoothTileList[] = "SSRSmoothTileList";
constexpr const char SSRSmoothTileCounter[] = "SSRSMothTileCounter";


ScreenSpaceReflectionTechnique::ScreenSpaceReflectionTechnique(Engine* eng, RenderGraph& graph) :
	Technique("SSR", eng->getDevice()),
    mTileCount(std::ceil(eng->getSwapChainImage()->getExtent(0, 0).width / 16.0f), std::ceil(eng->getSwapChainImage()->getExtent(0, 0).height / 16.0f)),
    mIndirectArgs(getDevice(), BufferUsage::DataBuffer | BufferUsage::IndirectArgs, std::max(getDevice()->getMinStorageBufferAlignment(), sizeof(uint3)) + sizeof(uint3),
                  std::max(getDevice()->getMinStorageBufferAlignment(), sizeof(uint3)) + sizeof(uint3), "SSR indirect args"),
    mSmoothIdirectArgsView(mIndirectArgs, 0, sizeof(uint3)),
    mRoughIndirectArgs(mIndirectArgs, std::max(getDevice()->getMinStorageBufferAlignment(), sizeof(uint3))),
    mSmoothTileList(getDevice(), BufferUsage::DataBuffer, mTileCount.x * mTileCount.y * sizeof(uint2), mTileCount.x * mTileCount.y * sizeof(uint2), "SSR smooth tile list"),
    mSmoothTileListView(mSmoothTileList, std::max(sizeof(uint32_t), getDevice()->getMinStorageBufferAlignment())),
    mSmoothTileCountView(mSmoothTileList, 0, sizeof(uint32_t)),
    mRoughTileList(getDevice(), BufferUsage::DataBuffer, mTileCount.x * mTileCount.y * sizeof(uint2), mTileCount.x * mTileCount.y * sizeof(uint2), "SSR Rough tile list"),
    mRoughTileListView(mRoughTileList, std::max(sizeof(uint32_t), getDevice()->getMinStorageBufferAlignment())),
    mRoughTileCountView(mRoughTileList, 0, sizeof(uint32_t)),
    mReflectionMap(getDevice(), Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled, eng->getSwapChainImage()->getExtent(0, 0).width / 2, eng->getSwapChainImage()->getExtent(0, 0).height / 2,
                   1, 1, 1, 1, "Reflection map"),
    mReflectionMapView(mReflectionMap, ImageViewType::Colour),
    mClampedSampler(SamplerType::Linear)
{
    ComputeTask tileAllocationTask("SSR tile allocation");
    tileAllocationTask.addInput(kGBufferSpecularRoughness, AttachmentType::Texture2D);
    tileAllocationTask.addInput(SSRRoughIndirectArgs, AttachmentType::DataBufferWO);
    tileAllocationTask.addInput(SSRSmoothIndirectArgs, AttachmentType::DataBufferWO);
    tileAllocationTask.addInput(SSRRoughTileList, AttachmentType::DataBufferWO);
    tileAllocationTask.addInput(SSRRoughTileCounter, AttachmentType::DataBufferWO);
    tileAllocationTask.addInput(SSRSmoothTileList, AttachmentType::DataBufferWO);
    tileAllocationTask.addInput(SSRSmoothTileCounter, AttachmentType::DataBufferWO);
    tileAllocationTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    tileAllocationTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    tileAllocationTask.setRecordCommandsCallback(
      [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            RenderDevice* device = eng->getDevice();
            const uint32_t xSize = eng->getSwapChainImage()->getExtent(0, 0).width / 2;
            const uint32_t ySize = eng->getSwapChainImage()->getExtent(0, 0).height / 2;

            Shader clearCountersShader = eng->getShader("./Shaders/ClearSSRTileCounters.comp");
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, clearCountersShader);
            exec->dispatch(1, 1, 1);

            {
                BarrierRecorder publishCounters(device);
                publishCounters->memoryBarrier(mSmoothTileCountView, Hazard::ReadAfterWrite, SyncPoint::ComputeShader, SyncPoint::ComputeShader);
                publishCounters->memoryBarrier(mRoughTileCountView, Hazard::ReadAfterWrite, SyncPoint::ComputeShader, SyncPoint::ComputeShader);
                exec->recordBarriers(publishCounters);
            }

            Shader allocateTilesShader = eng->getShader("./Shaders/ScreenSpaceReflectionAllocateTiles.comp");
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, allocateTilesShader);
            exec->dispatch(std::ceil(xSize / 8.0f), std::ceil(ySize / 8.0f), 1);

            {
                BarrierRecorder publishTileLists(device);
                publishTileLists->memoryBarrier(mSmoothTileList, Hazard::ReadAfterWrite, SyncPoint::ComputeShader, SyncPoint::ComputeShader);
                publishTileLists->memoryBarrier(mRoughTileList, Hazard::ReadAfterWrite, SyncPoint::ComputeShader, SyncPoint::ComputeShader);
                publishTileLists->memoryBarrier(mSmoothTileCountView, Hazard::ReadAfterWrite, SyncPoint::ComputeShader, SyncPoint::ComputeShader);
                publishTileLists->memoryBarrier(mRoughTileCountView, Hazard::ReadAfterWrite, SyncPoint::ComputeShader, SyncPoint::ComputeShader);
                exec->recordBarriers(publishTileLists);
            }

            Shader indirectArgsShader = eng->getShader("./Shaders/ScreenSpaceReflectionIndirectArgs.comp");
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, indirectArgsShader);
            exec->dispatch(1, 1, 1);
        }
    );
    graph.addTask(tileAllocationTask);


    ComputeTask roughTilesTask("SSR rough tiles");
    roughTilesTask.addInput(kLinearDepth, AttachmentType::Texture2D);
    roughTilesTask.addInput(kGBufferDepth, AttachmentType::Texture2D);
    roughTilesTask.addInput(kGBufferDiffuse, AttachmentType::Texture2D);
    roughTilesTask.addInput(kGBufferNormals, AttachmentType::Texture2D);
    roughTilesTask.addInput(kGBufferSpecularRoughness, AttachmentType::Texture2D);
    roughTilesTask.addInput(kConvolvedSpecularSkyBox, AttachmentType::CubeMap);
    roughTilesTask.addInput(SSRSampler, AttachmentType::Sampler);
    roughTilesTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    roughTilesTask.addInput(SSRRoughTileList, AttachmentType::DataBufferRO);
    roughTilesTask.addInput(kReflectionMap, AttachmentType::Image2D);
    roughTilesTask.addInput(SSRRoughIndirectArgs, AttachmentType::IndirectBuffer);

    roughTilesTask.setRecordCommandsCallback(
      [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            Shader computeShader = eng->getShader("./Shaders/ScreenSpaceReflectionRough.comp");
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, computeShader);

            exec->dispatchIndirect(mRoughIndirectArgs);
        }
    );
    graph.addTask(roughTilesTask);

    ComputeTask smoothTilesTask("SSR smooth tiles");
    smoothTilesTask.addInput(kLinearDepth, AttachmentType::Texture2D);
    smoothTilesTask.addInput(kGBufferDepth, AttachmentType::Texture2D);
    smoothTilesTask.addInput(kGBufferDiffuse, AttachmentType::Texture2D);
    smoothTilesTask.addInput(kGBufferSpecularRoughness, AttachmentType::Texture2D);
    smoothTilesTask.addInput(kGBufferNormals, AttachmentType::Texture2D);
    smoothTilesTask.addInput(SSRSmoothTileList, AttachmentType::DataBufferRO);
    smoothTilesTask.addInput(kConvolvedSpecularSkyBox, AttachmentType::CubeMap);
    smoothTilesTask.addInput(SSRSampler, AttachmentType::Sampler);
    smoothTilesTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    smoothTilesTask.addInput(kReflectionMap, AttachmentType::Image2D);
    smoothTilesTask.addInput(SSRSmoothIndirectArgs, AttachmentType::IndirectBuffer);

    smoothTilesTask.setRecordCommandsCallback(
      [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            Shader computeShader = eng->getShader("./Shaders/ScreenSpaceReflectionSmooth.comp");
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, computeShader);

            exec->dispatchIndirect(mSmoothIdirectArgsView);
        }
    );
    graph.addTask(smoothTilesTask);


    mClampedSampler.setAddressModeU(AddressMode::Clamp);
    mClampedSampler.setAddressModeV(AddressMode::Clamp);
    mClampedSampler.setAddressModeW(AddressMode::Clamp);
}


void ScreenSpaceReflectionTechnique::bindResources(RenderGraph& graph)
{
    if (!graph.isResourceSlotBound(SSRSampler))
    {
        graph.bindImage(kReflectionMap, mReflectionMapView);
        graph.bindSampler(SSRSampler, mClampedSampler);
        graph.bindBuffer(SSRRoughIndirectArgs, mRoughIndirectArgs);
        graph.bindBuffer(SSRSmoothIndirectArgs, mSmoothIdirectArgsView);
        graph.bindBuffer(SSRSmoothTileList, mSmoothTileListView);
        graph.bindBuffer(SSRRoughTileList, mRoughTileListView);
        graph.bindBuffer(SSRRoughTileCounter, mRoughTileCountView);
        graph.bindBuffer(SSRSmoothTileCounter, mSmoothTileCountView);
    }
}


RayTracedReflectionTechnique::RayTracedReflectionTechnique(Engine* eng, RenderGraph& graph) :
    Technique("Ray traced reflections", eng->getDevice()),
    mReflectionMap(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled,
                   eng->getSwapChainImage()->getExtent(0, 0).width / 4, eng->getSwapChainImage()->getExtent(0, 0).height / 4,
                   1, 1, 1, 1, "Reflection map"),
    mReflectionMapView(mReflectionMap, ImageViewType::Colour),
    mSampleNumber(0)
{
    ComputeTask task("Ray traced reflections");
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kGBufferNormals, AttachmentType::Texture2D);
    task.addInput(kGBufferSpecularRoughness, AttachmentType::Texture2D);
    task.addInput(kGBufferDepth, AttachmentType::Texture2D);
    task.addInput(kBlueNoise, AttachmentType::Texture2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kReflectionMap, AttachmentType::Image2D);
    task.addInput(kConvolvedSpecularSkyBox, AttachmentType::CubeMap);
    task.addInput("SampleNumber", AttachmentType::PushConstants);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput(kBVH, AttachmentType::ShaderResourceSet);

    task.setRecordCommandsCallback(
                [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    Shader computeShader = eng->getShader("./Shaders/RayTracedReflections.comp");
                    const RenderTask& task = graph.getTask(taskIndex);
                    exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, computeShader);

                    const uint32_t width =  eng->getDevice()->getSwapChain()->getSwapChainImageWidth() / 4;
                    const uint32_t height =  eng->getDevice()->getSwapChain()->getSwapChainImageHeight() / 4;

                    exec->insertPushConsatnt(&mSampleNumber, sizeof(uint32_t));
                    mSampleNumber = (mSampleNumber + 1) % 16;
                    exec->dispatch(std::ceil(width / 16.0f), std::ceil(height / 16.0f), 1);
                }
    );

    mTaskID = graph.addTask(task);
}


void RayTracedReflectionTechnique::bindResources(RenderGraph& graph)
{
    if (!graph.isResourceSlotBound(kReflectionMap))
    {
        graph.bindImage(kReflectionMap, mReflectionMapView);
    }
}
