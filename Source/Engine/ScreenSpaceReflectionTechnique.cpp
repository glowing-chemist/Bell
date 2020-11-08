#include "Engine/ScreenSpaceReflectionTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/UtilityTasks.hpp"
#include "Core/Executor.hpp"


constexpr const char SSRSampler[] = "SSRSampler";
constexpr const char RawReflections[] = "RawReflections";


ScreenSpaceReflectionTechnique::ScreenSpaceReflectionTechnique(Engine* eng, RenderGraph& graph) :
	Technique("SSR", eng->getDevice()),
    mTileCount(std::ceil(eng->getSwapChainImage()->getExtent(0, 0).width / 16.0f), std::ceil(eng->getSwapChainImage()->getExtent(0, 0).height / 16.0f)),
    mReflectionUVs(getDevice(), Format::RG32Float, ImageUsage::Storage | ImageUsage::Sampled, eng->getSwapChainImage()->getExtent(0, 0).width / 2, eng->getSwapChainImage()->getExtent(0, 0).height / 2,
        1, 1, 1, 1, "Reflection UV map"),
    mReflectionUVsView(mReflectionUVs, ImageViewType::Colour),
    mReflectionMapRaw(getDevice(), Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled, eng->getSwapChainImage()->getExtent(0, 0).width / 2, eng->getSwapChainImage()->getExtent(0, 0).height / 2,
                   1, 1, 1, 1, "Reflection map raw"),
    mReflectionMapRawView(mReflectionMapRaw, ImageViewType::Colour),
    mReflectionMap(getDevice(), Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled, eng->getSwapChainImage()->getExtent(0, 0).width, eng->getSwapChainImage()->getExtent(0, 0).height,
                   1, 1, 1, 1, "Reflection map"),
    mReflectionMapView(mReflectionMap, ImageViewType::Colour),
    mClampedSampler(SamplerType::Linear)
{
    ComputeTask SSRTask("SSR");
    SSRTask.addInput(kLinearDepth, AttachmentType::Texture2D);
    SSRTask.addInput(kGBufferDepth, AttachmentType::Texture2D);
    SSRTask.addInput(kGBufferNormals, AttachmentType::Texture2D);
    SSRTask.addInput(SSRSampler, AttachmentType::Sampler);
    SSRTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    SSRTask.addInput(kReflecionUVs, AttachmentType::Image2D);

    SSRTask.setRecordCommandsCallback(
      [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            Shader computeShader = eng->getShader("./Shaders/ScreenSpaceReflection.comp");
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, computeShader);

            exec->dispatch(mTileCount.x, mTileCount.y, 1);
        }
    );
    graph.addTask(SSRTask);

    ComputeTask resolveReflections("Ressolve reflections");
    resolveReflections.addInput(kGBufferSpecularRoughness, AttachmentType::Texture2D);
    resolveReflections.addInput(kDownSampledColour, AttachmentType::Texture2D);
    resolveReflections.addInput(kReflecionUVs, AttachmentType::Texture2D);
    resolveReflections.addInput(kDefaultSampler, AttachmentType::Sampler);
    resolveReflections.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    resolveReflections.addInput(RawReflections, AttachmentType::Image2D);
    resolveReflections.setRecordCommandsCallback([this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
    {
        Shader computeShader = eng->getShader("./Shaders/RessolveReflections.comp");
        const RenderTask& task = graph.getTask(taskIndex);
        exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, computeShader);

        exec->dispatch(mTileCount.x, mTileCount.y, 1);
    });
    graph.addTask(resolveReflections);

    addDeferredUpsampleTaskRGBA8("upsample reflections", RawReflections, kReflectionMap, uint2(eng->getSwapChainImage()->getExtent(0, 0).width, eng->getSwapChainImage()->getExtent(0, 0).height),
                                 eng, graph);

    mClampedSampler.setAddressModeU(AddressMode::Clamp);
    mClampedSampler.setAddressModeV(AddressMode::Clamp);
    mClampedSampler.setAddressModeW(AddressMode::Clamp);
}


void ScreenSpaceReflectionTechnique::bindResources(RenderGraph& graph)
{
    if (!graph.isResourceSlotBound(SSRSampler))
    {
        graph.bindImage(RawReflections, mReflectionMapRawView);
        graph.bindImage(kReflectionMap, mReflectionMapView);
        graph.bindImage(kReflecionUVs, mReflectionUVsView);
        graph.bindSampler(SSRSampler, mClampedSampler);
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
