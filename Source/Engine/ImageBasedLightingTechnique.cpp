#include "Engine/ImageBasedLightingTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"


DeferredImageBasedLightingTechnique::DeferredImageBasedLightingTechnique(Engine* eng, RenderGraph& graph) :
    Technique("GlobalIBL", eng->getDevice()),
    mPipelineDesc{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                    getDevice()->getSwapChain()->getSwapChainImageHeight()},
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
              getDevice()->getSwapChain()->getSwapChainImageHeight()} },
    mIBLVertexShader(eng->getShader("./Shaders/FullScreenTriangle.vert")),
    mIBLFragmentShader(eng->getShader("./Shaders/DeferredDFGIBL.frag"))
{
    GraphicsTask task{ "GlobalIBL", mPipelineDesc };

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDFGLUT, AttachmentType::Texture2D);
    task.addInput(kGBufferDepth, AttachmentType::Texture2D);
    task.addInput(kGBufferNormals, AttachmentType::Texture2D);
    task.addInput(kGBufferDiffuse, AttachmentType::Texture2D);
    task.addInput(kGBufferSpecularRoughness, AttachmentType::Texture2D);
    task.addInput(kGBufferEmissiveOcclusion, AttachmentType::Texture2D);
    task.addInput(kConvolvedSpecularSkyBox, AttachmentType::CubeMap);
    task.addInput(kConvolvedDiffuseSkyBox, AttachmentType::CubeMap);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);

    if (eng->isPassRegistered(PassType::Shadow) || eng->isPassRegistered(PassType::CascadingShadow) || eng->isPassRegistered(PassType::RayTracedShadows))
        task.addInput(kShadowMap, AttachmentType::Texture2D);

    task.addManagedOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm,
        SizeClass::Swapchain, LoadOp::Clear_Black, StoreOp::Store, ImageUsage::ColourAttachment | ImageUsage::Sampled | ImageUsage::TransferSrc);

    task.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
        {
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mIBLVertexShader, nullptr, nullptr, nullptr, mIBLFragmentShader);

            exec->draw(0, 3);
        }
    );
    mTaskID = graph.addTask(task);
}
