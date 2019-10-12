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
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, true, DepthTest::GreaterEqual, Primitive::TriangleList},

    mTask{"GBuffer", mPipelineDescription}
{
    mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    mTask.addInput("Model Matrix", AttachmentType::PushConstants);

	mTask.addOutput(kGBufferNormals, AttachmentType::Texture2D, Format::R16G16Unorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(kGBufferAlbedo, AttachmentType::Texture2D, Format::RGBA8SRGB, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(kGBufferSpecular, AttachmentType::Texture2D, Format::R8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_White);
}


void GBufferTechnique::bindResources(RenderGraph& graph) const
{
    graph.bindImage(kGBufferAlbedo, mAlbedoView);
    graph.bindImage(kGBufferNormals, mNormalsView);
    graph.bindImage(kGBufferDepth, mDepthView);
    graph.bindImage(kGBufferSpecular, mSpecularView);
}


GBufferPreDepthTechnique::GBufferPreDepthTechnique(Engine* eng) :
    Technique{"GBufferPreDepth", eng->getDevice()},

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
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, Primitive::TriangleList},

    mTask{"GBufferPreDepth", mPipelineDescription}
{
    mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Albedo |
                              VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    mTask.addInput("Model Matrix", AttachmentType::PushConstants);

	mTask.addOutput(kGBufferNormals,  AttachmentType::Texture2D, Format::R16G16Unorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(kGBufferAlbedo,   AttachmentType::Texture2D, Format::RGBA8SRGB, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(kGBufferSpecular, AttachmentType::Texture2D, Format::R8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(mDepthName,         AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Preserve);
}


void GBufferPreDepthTechnique::bindResources(RenderGraph& graph) const
{
    graph.bindImage(kGBufferAlbedo, mAlbedoView);
    graph.bindImage(kGBufferNormals, mNormalsView);
    graph.bindImage(kGBufferSpecular, mSpecularView);
}
