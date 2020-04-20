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
    task.addInput(kConvolvedSpecularSkyBox, AttachmentType::Texture2D);
    task.addInput(kConvolvedDiffuseSkyBox, AttachmentType::Texture2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);

    if (eng->isPassRegistered(PassType::Shadow))
        task.addInput(kShadowMap, AttachmentType::Texture2D);

    task.addInput("IBLMaterial", AttachmentType::PushConstants);

    task.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm,
        SizeClass::Swapchain, LoadOp::Clear_Black);

    task.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            const uint32_t materialFlags = eng->getScene().getMaterialFlags();
            exec->insertPushConsatnt(&materialFlags, sizeof(uint32_t));
            exec->draw(0, 3);
        }
    );
    mTaskID = graph.addTask(task);
}
