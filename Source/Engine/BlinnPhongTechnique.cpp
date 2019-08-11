#include "Engine/BlinnPhongTechnique.hpp"

#include "Core/RenderDevice.hpp"
#include "Engine/Engine.hpp"


BlinnPhongDeferredTextures::BlinnPhongDeferredTextures(Engine* eng) :
	Technique{ "BlinnPhongDeferredTextures", eng->getDevice() },
	mLightingTexture{ getDevice(), Format::RGBA8SRGB, ImageUsage::Sampled | ImageUsage::ColourAttachment,
			   getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(), 1 },
	mLightingView{ mLightingTexture, ImageViewType::Colour },
	mPipelineDesc{ eng->getShader("FullScreentriangle.vert")
				  ,eng->getShader("BlinnPhongDeferredTexture.frag") ,
				  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
						getDevice()->getSwapChain()->getSwapChainImageHeight()},
				  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
				  getDevice()->getSwapChain()->getSwapChainImageHeight()} },
	mTask{ "BlinnPhongDeferredTextures", mPipelineDesc },
	mTaskInitialised{ false }
{
}


GraphicsTask& BlinnPhongDeferredTextures::getTaskToRecord()
{

	if (!mTaskInitialised)
	{
		// full scren triangle so no vertex attributes.
		mTask.setVertexAttributes(0);

		// These are always availble, and updated and bound by the engine.
		mTask.addInput("LightBuffer", AttachmentType::UniformBuffer);
		mTask.addInput("CameraBuffer", AttachmentType::UniformBuffer);
		mTask.addInput("MaterialIndex", AttachmentType::UniformBuffer);

		mTask.addInput(mDepthName, AttachmentType::Texture2D);
		mTask.addInput(mVertexNormalsName, AttachmentType::Texture2D);
		mTask.addInput(mMaterialName, AttachmentType::Texture2D);
		mTask.addInput(mUVName, AttachmentType::Texture2D);

		// Bound by the engine.
		mTask.addInput("DefaultSampler", AttachmentType::Sampler);

		mTask.addOutput(getLightTextureName(), AttachmentType::RenderTarget2D, Format::RGBA8SRGB, LoadOp::Clear_Black);

		mTaskInitialised = true;
	}

	// CLear the draw calls first.
	mTask.clearCalls();

	return mTask;
}