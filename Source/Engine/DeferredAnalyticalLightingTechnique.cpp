#include "Engine/DeferredAnalyticalLightingTechnique.hpp"
#include "Engine/Engine.hpp"


DeferredAnalyticalLightingTechnique::DeferredAnalyticalLightingTechnique(Engine* eng, RenderGraph& graph) :
	Technique("deferred analytical lighting", eng->getDevice()),
	mPipelineDesc{ eng->getShader("./Shaders/DeferredAnalyticalLighting.comp") },
	mAnalyticalLighting(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::Storage, eng->getSwapChainImageView()->getImageExtent().width, 
		eng->getSwapChainImageView()->getImageExtent().height, 1, 1, 1, 1, kAnalyticLighting),
	mAnalyticalLightingView(mAnalyticalLighting, ImageViewType::Colour),
	mPointSampler(SamplerType::Point)
{
	ComputeTask task{ "deferred analytical lighting", mPipelineDesc };
	task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	task.addInput(kDFGLUT, AttachmentType::Texture2D);
	task.addInput(kLTCMat, AttachmentType::Texture2D);
	task.addInput(kLTCAmp, AttachmentType::Texture2D);
	task.addInput(kGBufferDepth, AttachmentType::Texture2D);
	task.addInput(kGBufferNormals, AttachmentType::Texture2D);
	task.addInput(kGBufferAlbedo, AttachmentType::Texture2D);
	task.addInput(kGBufferMetalnessRoughness, AttachmentType::Texture2D);
	task.addInput(kDefaultSampler, AttachmentType::Sampler);
	task.addInput("PointSampler", AttachmentType::Sampler);
	task.addInput(kSparseFroxels, AttachmentType::DataBufferRO);
	task.addInput(kLightIndicies, AttachmentType::DataBufferRO);
	task.addInput(kActiveFroxels, AttachmentType::Texture2D);
	task.addInput(kAnalyticLighting, AttachmentType::Image2D);
	task.addInput(kLightBuffer, AttachmentType::ShaderResourceSet);

	const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
	const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

	task.addDispatch(	static_cast<uint32_t>(std::ceil(threadGroupWidth / 32.0f)),
						static_cast<uint32_t>(std::ceil(threadGroupHeight / 32.0f)),
						1);

	mTaskID = graph.addTask(task);
}
