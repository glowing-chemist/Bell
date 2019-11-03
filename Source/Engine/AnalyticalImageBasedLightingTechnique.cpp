#include "Engine/AnalyticalImageBasedLightingTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

AnalyticalImageBasedLightingTechnique::AnalyticalImageBasedLightingTechnique(Engine* eng) :
	Technique("Analytical IBL", eng->getDevice()),
	mPipelineDesc{eng->getShader("./Shaders/FullScreenTriangle.vert")
			  ,eng->getShader("./Shaders/AnalyticalIBLDeferredTexture.frag"),
			  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
					getDevice()->getSwapChain()->getSwapChainImageHeight()},
			  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
			  getDevice()->getSwapChain()->getSwapChainImageHeight()}},
	mTask("AnalyticalIBL", mPipelineDesc)
{
	mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	mTask.addInput(kGBufferDepth, AttachmentType::Texture2D);
	mTask.addInput(kGBufferNormals, AttachmentType::Texture2D);
	mTask.addInput(kGBufferMaterialID, AttachmentType::Texture2D);
	mTask.addInput(kGBufferUV, AttachmentType::Texture2D);
	mTask.addInput(kSkyBox, AttachmentType::Texture2D);
	mTask.addInput(kConvolvedSkyBox, AttachmentType::Texture2D);
	mTask.addInput(kDefaultSampler, AttachmentType::Sampler);
	mTask.addInput(kMaterials, AttachmentType::TextureArray);

	mTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, eng->getDevice()->getSwapChainImage().getFormat(),
					SizeClass::Custom, LoadOp::Clear_Black);

	mTask.addDrawCall(0, 3);
}