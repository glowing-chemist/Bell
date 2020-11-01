#include "Engine/DeferredProbeGITechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

#include "Core/Executor.hpp"

DeferredProbeGITechnique::DeferredProbeGITechnique(Engine* eng, RenderGraph& graph) :
    Technique("DeferredProbeGI", eng->getDevice()),
    mPipelineDesc{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                    getDevice()->getSwapChain()->getSwapChainImageHeight()},
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
              getDevice()->getSwapChain()->getSwapChainImageHeight()} },
    mProbeVertexShader(eng->getShader("./Shaders/FullScreenTriangle.vert")),
    mProbeFragmentShader(eng->getShader("./Shaders/DeferredProbeGI.frag"))
{
    GraphicsTask task{"DeferredProbeGI", mPipelineDesc};
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDFGLUT, AttachmentType::Texture2D);
    task.addInput(kGBufferDepth, AttachmentType::Texture2D);
    task.addInput(kGBufferNormals, AttachmentType::Texture2D);
    task.addInput(kGBufferDiffuse, AttachmentType::Texture2D);
    task.addInput(kGBufferSpecularRoughness, AttachmentType::Texture2D);
    task.addInput(kGBufferEmissiveOcclusion, AttachmentType::Texture2D);
    task.addInput(kConvolvedSpecularSkyBox, AttachmentType::CubeMap);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kLightProbes, AttachmentType::ShaderResourceSet);

    if (eng->isPassRegistered(PassType::Shadow) || eng->isPassRegistered(PassType::CascadingShadow) || eng->isPassRegistered(PassType::RayTracedShadows))
        task.addInput(kShadowMap, AttachmentType::Texture2D);

    task.addManagedOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm,
        SizeClass::Swapchain, LoadOp::Clear_Black, StoreOp::Store, ImageUsage::ColourAttachment | ImageUsage::Sampled | ImageUsage::TransferSrc);

    task.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
        {
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mProbeVertexShader, nullptr, nullptr, nullptr, mProbeFragmentShader);

            exec->draw(0, 3);
        }
    );
    mTaskID = graph.addTask(task);
}
