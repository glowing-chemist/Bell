#include "Engine/ImageBasedLightingTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"


DeferredImageBasedLightingTechnique::DeferredImageBasedLightingTechnique(Engine* eng, RenderGraph& graph) :
    Technique("GlobalIBL", eng->getDevice()),
    mPipelineDesc{ eng->getShader("./Shaders/FullScreenTriangle.vert")
              ,eng->getShader("./Shaders/DeferredDFGIBL.frag"),
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                    getDevice()->getSwapChain()->getSwapChainImageHeight()},
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
              getDevice()->getSwapChain()->getSwapChainImageHeight()} }
{
    GraphicsTask task{ "GlobalIBL", mPipelineDesc };

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDFGLUT, AttachmentType::Texture2D);
    task.addInput(kGBufferDepth, AttachmentType::Texture2D);
    task.addInput(kGBufferNormals, AttachmentType::Texture2D);
    task.addInput(kGBufferDiffuse, AttachmentType::Texture2D);
    task.addInput(kGBufferSpecularRoughness, AttachmentType::Texture2D);
    task.addInput(kGBufferEmissiveOcclusion, AttachmentType::Texture2D);

    if(eng->isPassRegistered(PassType::SSR) || eng->isPassRegistered(PassType::RayTracedReflections))
       task.addInput(kReflectionMap, AttachmentType::Texture2D);
    else
        task.addInput(kConvolvedSpecularSkyBox, AttachmentType::CubeMap);
    task.addInput(kConvolvedDiffuseSkyBox, AttachmentType::CubeMap);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);

    if (eng->isPassRegistered(PassType::Shadow) || eng->isPassRegistered(PassType::CascadingShadow) || eng->isPassRegistered(PassType::RayTracedShadows))
        task.addInput(kShadowMap, AttachmentType::Texture2D);

    task.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm,
        SizeClass::Swapchain, LoadOp::Clear_Black);

    task.setRecordCommandsCallback(
        [](Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
        {
            exec->draw(0, 3);
        }
    );
    mTaskID = graph.addTask(task);
}
