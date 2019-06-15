#include "Engine/GBufferTechnique.hpp"


GBufferTechnique::GBufferTechnique(Engine* eng) :
	Technique{"GBuffer", eng->getDevice()},

	mDepthImage{getDevice(), vk::Format::eD24UnormS8Uint, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
				getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
				1, 1, 1, 1, "Depth Buffer"},
	mDepthView{mDepthImage, ImageViewType::Depth},

	mAlbedoImage{getDevice(), vk::Format::eB8G8R8Srgb, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
				 getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
				 1, 1, 1, 1, "Albedo Buffer"},
	mAlbedoView{mAlbedoImage, ImageViewType::Colour},

	mNormalsImage{getDevice(), vk::Format::eR16G16Snorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
				  getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
				  1, 1, 1, 1, "Normals Buffer"},
	mNormalsView{mNormalsImage, ImageViewType::Colour},

	mSpecularImage{getDevice(), vk::Format::eR8Snorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
				   getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
				   1, 1, 1, 1, "Specular Buffer"},
	mSpecularView{mSpecularImage, ImageViewType::Colour},

	mPipelineDescription{eng->getShader("./Shaders/GBufferPassThrough.vert"),
						 eng->getShader("./Shaders/GBuffer.frag"),
						 Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
							   getDevice()->getSwapChain()->getSwapChainImageHeight()},
						 Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
						 getDevice()->getSwapChain()->getSwapChainImageHeight()}},

	mTask{"GBuffer", mPipelineDescription}
{}


RenderTask& GBufferTechnique::getTaskToRecord()
{
	mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Aledo |
							  VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

	mTask.addInput("Model Matrix", AttachmentType::PushConstants);

	mTask.addOutput("GBuffer Normals", AttachmentType::Texture2D, Format::R16G16Unorm, LoadOp::Clear_Black);
	mTask.addOutput("GBuffer Albedo", AttachmentType::Texture2D, Format::RGBA8SRGB, LoadOp::Clear_Black);
	mTask.addOutput("GBuffer Specular", AttachmentType::Texture2D, Format::R8UNorm, LoadOp::Clear_Black);


	return mTask;
}
