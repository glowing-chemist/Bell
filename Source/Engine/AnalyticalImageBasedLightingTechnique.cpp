#include "Engine/AnalyticalImageBasedLightingTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

AnalyticalImageBasedLightingTechnique::AnalyticalImageBasedLightingTechnique(Engine* eng, RenderGraph& graph) :
	Technique("Analytical IBL", eng->getDevice()),
	mPipelineDesc{eng->getShader("./Shaders/FullScreenTriangle.vert")
			  ,eng->getShader("./Shaders/AnalyticalIBLDeferredTexture.frag"),
			  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
					getDevice()->getSwapChain()->getSwapChainImageHeight()},
			  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
			  getDevice()->getSwapChain()->getSwapChainImageHeight()}}
{
	GraphicsTask task{ "AnalyticalIBL", mPipelineDesc };
	task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
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

	task.addDrawCall(0, 3);
	mTaskID = graph.addTask(task);
}
