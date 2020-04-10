#include "Engine/ImageBasedLightingTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"

ImageBasedLightingDeferredTexturingTechnique::ImageBasedLightingDeferredTexturingTechnique(Engine* eng, RenderGraph& graph) :
	Technique("GlobalIBL", eng->getDevice()),
	mPipelineDesc{ eng->getShader("./Shaders/FullScreenTriangle.vert")
              ,eng->getShader("./Shaders/DFGIBLDeferredTexturing.frag"),
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
    task.addInput(kGBufferMaterialID, AttachmentType::Texture2D);
    task.addInput(kGBufferUV, AttachmentType::Texture2D);
    task.addInput(kSkyBox, AttachmentType::Texture2D);
    task.addInput(kConvolvedSkyBox, AttachmentType::Texture2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);

    if (eng->isPassRegistered(PassType::Shadow))
        task.addInput(kShadowMap, AttachmentType::Texture2D);

    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);

    task.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm,
		SizeClass::Swapchain, LoadOp::Clear_Black);

    task.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            exec->draw(0, 3);
        }
    );
    mTaskID = graph.addTask(task);
}


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
    task.addInput(kGBufferAlbedo, AttachmentType::Texture2D);
    task.addInput(kGBufferMetalnessRoughness, AttachmentType::Texture2D);
    task.addInput(kSkyBox, AttachmentType::Texture2D);
    task.addInput(kConvolvedSkyBox, AttachmentType::Texture2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);

    if (eng->isPassRegistered(PassType::Shadow))
        task.addInput(kShadowMap, AttachmentType::Texture2D);

    task.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm,
        SizeClass::Swapchain, LoadOp::Clear_Black);

    task.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            exec->draw(0, 3);
        }
    );
    mTaskID = graph.addTask(task);
}
