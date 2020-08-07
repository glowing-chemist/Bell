#include "Engine/PathTracingTechnique.hpp"

#include "Engine/Engine.hpp"

#include "Core/Executor.hpp"


PathTracingTechnique::PathTracingTechnique(Engine* eng, RenderGraph& graph) :
    Technique("PathTracing", eng->getDevice()),
    mGloballighting(getDevice(), Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled | ImageUsage::ColourAttachment, getDevice()->getSwapChain()->getSwapChainImageWidth() / 4,
                    getDevice()->getSwapChain()->getSwapChainImageHeight() / 4, 1, 1, 1, 1, "Global lighting"),
    mGlobalLightingView(mGloballighting, ImageViewType::Colour),
    mPathTracingShader(eng->getShader("./Shaders/PathTracer.comp"))
{
    ComputeTask task("PathTracing");
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kGBufferNormals, AttachmentType::Texture2D);
    task.addInput(kGBufferDepth, AttachmentType::Texture2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kGlobalLighting, AttachmentType::Image2D);
    task.addInput(kSkyBox, AttachmentType::CubeMap);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput(kBVH, AttachmentType::ShaderResourceSet);

    task.setRecordCommandsCallback(
                [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    const RenderTask& task = graph.getTask(taskIndex);
                    exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mPathTracingShader);

                    const uint32_t width =  eng->getDevice()->getSwapChain()->getSwapChainImageWidth() / 4;
                    const uint32_t height =  eng->getDevice()->getSwapChain()->getSwapChainImageHeight() / 4;

                    exec->dispatch(std::ceil(width / 16.0f), std::ceil(height / 16.0f), 1);
                }
    );

    mTaskID = graph.addTask(task);
}
