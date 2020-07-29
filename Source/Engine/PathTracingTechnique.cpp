#include "Engine/PathTracingTechnique.hpp"

#include "Engine/Engine.hpp"

#include "Core/Executor.hpp"


PathTracingTechnique::PathTracingTechnique(Engine* eng, RenderGraph& graph) :
    Technique("PathTracing", eng->getDevice()),
    mGloballighting(getDevice(), Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled | ImageUsage::ColourAttachment, getDevice()->getSwapChain()->getSwapChainImageWidth(),
                    getDevice()->getSwapChain()->getSwapChainImageHeight(), 1, 1, 1, 1, "Global lighting"),
    mGlobalLightingView(mGloballighting, ImageViewType::Colour),
    mPipelineDescription{eng->getShader("./Shaders/PathTracer.comp")}
{
    ComputeTask task("PathTracing", mPipelineDescription);
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kGlobalLighting, AttachmentType::Image2D);
    task.addInput(kSkyBox, AttachmentType::CubeMap);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput(kBVH, AttachmentType::ShaderResourceSet);

    task.setRecordCommandsCallback(
                [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    const uint32_t width =  eng->getDevice()->getSwapChain()->getSwapChainImageWidth();
                    const uint32_t height =  eng->getDevice()->getSwapChain()->getSwapChainImageHeight();

                    exec->dispatch(std::ceil(width / 16.0f), std::ceil(height / 16.0f), 1);
                }
    );

    mTaskID = graph.addTask(task);
}
