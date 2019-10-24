#include "Engine/GBufferTechnique.hpp"


GBufferTechnique::GBufferTechnique(Engine* eng) :
	Technique{"GBuffer", eng->getDevice()},
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


void GBufferTechnique::render(RenderGraph& graph, const std::vector<const Scene::MeshInstance *>& meshes)
{

}


GBufferPreDepthTechnique::GBufferPreDepthTechnique(Engine* eng) :
    Technique{"GBufferPreDepth", eng->getDevice()},
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


void GBufferPreDepthTechnique::render(RenderGraph& graph, const std::vector<const Scene::MeshInstance *>& meshes)
{

}
