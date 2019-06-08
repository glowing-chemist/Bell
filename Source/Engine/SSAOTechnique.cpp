#include "Engine/SSAOTechnique.hpp"
#include "Core/RenderDevice.hpp"


SSAOTechnique::SSAOTechnique(Engine* eng) :
	Technique{"SSAO", eng->getDevice()},
	mSSAOImage{getDevice(), vk::Format::eR8Snorm, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment,
			   getDevice()->getSwapChain()->getSwapChainImageWidth() / 2, getDevice()->getSwapChain()->getSwapChainImageHeight() / 2, 1},
	mSSAOIMageView{mSSAOImage, ImageViewType::Colour},
	mLinearSampler{SamplerType::Linear},
	mFullScreenTriangleVertex{eng->getShader("Shaders/FullScreenTriangle.vert")},
	mSSAOFragmentShader{eng->getShader("Shaders/SSAO.frag")},
	mPipelineDesc{mFullScreenTriangleVertex
				  ,mSSAOFragmentShader,
				  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
						getDevice()->getSwapChain()->getSwapChainImageHeight() / 2},
				  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
				  getDevice()->getSwapChain()->getSwapChainImageHeight() / 2}},
	mTask{"SSAO", mPipelineDesc}
{
}


RenderTask& SSAOTechnique::getTaskToRecord()
{
	// full scren triangle so no vertex attributes.
	mTask.setVertexAttributes(0);

	// These are always availble, and updated and bound by the engine.
	mTask.addInput("NormalsOffset", AttachmentType::UniformBuffer);
	mTask.addInput("CameraBuffer", AttachmentType::UniformBuffer);

	mTask.addInput(mDepthNameSlot, AttachmentType::Texture2D);
	mTask.addInput("linearSampler", AttachmentType::Sampler);

	mTask.addOutput("ssaoRenderTarget", AttachmentType::RenderTarget2D, Format::RGBA8UNorm, LoadOp::Clear_Black);

	return mTask;
}
