#include "Engine/ImageBasedLightingTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

ImageBasedLightingDeferredTexturingTechnique::ImageBasedLightingDeferredTexturingTechnique(Engine* eng) :
	Technique("GlobalIBL", eng->getDevice()),
	mPipelineDesc{ eng->getShader("./Shaders/FullScreenTriangle.vert")
              ,eng->getShader("./Shaders/DFGIBLDeferredTexturing.frag"),
			  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
					getDevice()->getSwapChain()->getSwapChainImageHeight()},
			  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
			  getDevice()->getSwapChain()->getSwapChainImageHeight()} },
	mTask("GlobalIBL", mPipelineDesc)
{
	mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	mTask.addInput(kDFGLUT, AttachmentType::Texture2D);
	mTask.addInput(kGBufferDepth, AttachmentType::Texture2D);
	mTask.addInput(kGBufferNormals, AttachmentType::Texture2D);
	mTask.addInput(kGBufferMaterialID, AttachmentType::Texture2D);
	mTask.addInput(kGBufferUV, AttachmentType::Texture2D);
	mTask.addInput(kSkyBox, AttachmentType::Texture2D);
	mTask.addInput(kConvolvedSkyBox, AttachmentType::Texture2D);
    mTask.addInput(kDefaultSampler, AttachmentType::Sampler);

    if (eng->isPassRegistered(PassType::Shadow))
        mTask.addInput(kShadowMap, AttachmentType::Texture2D);

	mTask.addInput(kMaterials, AttachmentType::ShaderResourceSet);

	mTask.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm,
		SizeClass::Swapchain, LoadOp::Clear_Black);

	mTask.addDrawCall(0, 3);
}


DeferredImageBasedLightingTechnique::DeferredImageBasedLightingTechnique(Engine* eng) :
    Technique("GlobalIBL", eng->getDevice()),
    mPipelineDesc{ eng->getShader("./Shaders/FullScreenTriangle.vert")
              ,eng->getShader("./Shaders/DeferredDFGIBL.frag"),
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                    getDevice()->getSwapChain()->getSwapChainImageHeight()},
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
              getDevice()->getSwapChain()->getSwapChainImageHeight()} },
    mTask("GlobalIBL", mPipelineDesc)
{
    mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    mTask.addInput(kDFGLUT, AttachmentType::Texture2D);
    mTask.addInput(kGBufferDepth, AttachmentType::Texture2D);
    mTask.addInput(kGBufferNormals, AttachmentType::Texture2D);
    mTask.addInput(kGBufferAlbedo, AttachmentType::Texture2D);
    mTask.addInput(kGBufferMetalnessRoughness, AttachmentType::Texture2D);
    mTask.addInput(kSkyBox, AttachmentType::Texture2D);
    mTask.addInput(kConvolvedSkyBox, AttachmentType::Texture2D);
    mTask.addInput(kDefaultSampler, AttachmentType::Sampler);

    if (eng->isPassRegistered(PassType::Shadow))
        mTask.addInput(kShadowMap, AttachmentType::Texture2D);

    mTask.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm,
        SizeClass::Swapchain, LoadOp::Clear_Black);

    mTask.addDrawCall(0, 3);
}
