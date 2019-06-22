#include "Engine/GBufferTechnique.hpp"


GBufferTechnique::GBufferTechnique(Engine* eng) :
	Technique{"GBuffer", eng->getDevice()},

	mDepthImage{getDevice(), Format::D24S8Float, ImageUsage::DepthStencil | ImageUsage::Sampled,
				getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
				1, 1, 1, 1, "Depth Buffer"},
	mDepthView{mDepthImage, ImageViewType::Depth},

	mAlbedoImage{getDevice(), Format::RGBA8SRGB, ImageUsage::ColourAttachment | ImageUsage::Sampled,
				 getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
				 1, 1, 1, 1, "Albedo Buffer"},
	mAlbedoView{mAlbedoImage, ImageViewType::Colour},

	mNormalsImage{getDevice(), Format::R16G16Unorm, ImageUsage::ColourAttachment | ImageUsage::Sampled,
				  getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
				  1, 1, 1, 1, "Normals Buffer"},
	mNormalsView{mNormalsImage, ImageViewType::Colour},

	mSpecularImage{getDevice(), Format::R8UNorm, ImageUsage::ColourAttachment | ImageUsage::Sampled,
				   getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
				   1, 1, 1, 1, "Specular Buffer"},
	mSpecularView{mSpecularImage, ImageViewType::Colour},

	mPipelineDescription{eng->getShader("./Shaders/GBufferPassThrough.vert"),
						 eng->getShader("./Shaders/GBuffer.frag"),
						 Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
							   getDevice()->getSwapChain()->getSwapChainImageHeight()},
						 Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
						 getDevice()->getSwapChain()->getSwapChainImageHeight()}},

	mTask{"GBuffer", mPipelineDescription},
	mTaskInitialised{false}
{}


GraphicsTask &GBufferTechnique::getTaskToRecord()
{

	if(!mTaskInitialised)
	{
		mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Aledo |
								  VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

		mTask.addInput("Model Matrix", AttachmentType::PushConstants);

		mTask.addOutput("GBuffer Normals", AttachmentType::Texture2D, Format::R16G16Unorm, LoadOp::Clear_Black);
		mTask.addOutput("GBuffer Albedo", AttachmentType::Texture2D, Format::RGBA8SRGB, LoadOp::Clear_Black);
		mTask.addOutput("GBuffer Specular", AttachmentType::Texture2D, Format::R8UNorm, LoadOp::Clear_Black);

		mTaskInitialised = true;
	}

	mTask.clearCalls();

	return mTask;
}
