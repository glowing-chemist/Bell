#include "Engine/DeferredAnalyticalLightingTechnique.hpp"
#include "Engine/Engine.hpp"


DeferredAnalyticalLightingTechnique::DeferredAnalyticalLightingTechnique(Engine* eng) :
	Technique("deferred analytical lighting", eng->getDevice()),
	mPipelineDesc{ eng->getShader("./Shaders/DeferredAnalyticalLighting.comp") },
	mTask("deferred analytical lighting", mPipelineDesc),
	mAnalyticalLighting(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::Storage, eng->getSwapChainImage().getExtent(0, 0).width, 
		eng->getSwapChainImage().getExtent(0, 0).height, 1, 1, 1, 1, kAnalyticLighting),
	mAnalyticalLightingView(mAnalyticalLighting, ImageViewType::Colour),
	mPointSampler(SamplerType::Point)
{
	mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	mTask.addInput(kDFGLUT, AttachmentType::Texture2D);
	mTask.addInput(kGBufferDepth, AttachmentType::Texture2D);
	mTask.addInput(kGBufferNormals, AttachmentType::Texture2D);
	mTask.addInput(kGBufferAlbedo, AttachmentType::Texture2D);
	mTask.addInput(kGBufferMetalnessRoughness, AttachmentType::Texture2D);
	mTask.addInput(kDefaultSampler, AttachmentType::Sampler);
	mTask.addInput("PointSampler", AttachmentType::Sampler);
	mTask.addInput(kSparseFroxels, AttachmentType::DataBufferRO);
	mTask.addInput(kActiveFroxels, AttachmentType::Texture2D);
	mTask.addInput(kAnalyticLighting, AttachmentType::Image2D);
	mTask.addInput(kLightBuffer, AttachmentType::ShaderResourceSet);

	mTask.addDispatch(	eng->getSwapChainImage().getExtent(0, 0).width / 8,
						eng->getSwapChainImage().getExtent(0, 0).height / 8,
						1);
}
