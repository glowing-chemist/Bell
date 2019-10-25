#include "Engine/GBufferMaterialTechnique.hpp"


GBufferMaterialTechnique::GBufferMaterialTechnique(Engine* eng) :
    Technique{"GBufferMaterial", eng->getDevice()},
    mPipelineDescription{eng->getShader("./Shaders/GBufferPassThroughMaterial.vert"),
                         eng->getShader("./Shaders/GBufferMaterial.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, true, DepthTest::GreaterEqual, Primitive::TriangleList},

    mTask{"GBufferMaterial", mPipelineDescription}
{
    mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Material |
                              VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    mTask.addInput("CameraBuffer", AttachmentType::UniformBuffer);
    mTask.addInput("ModelMatrix", AttachmentType::PushConstants);

	mTask.addOutput("GBuffer Normals",  AttachmentType::Texture2D, Format::R16G16Unorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput("GBuffer UV",       AttachmentType::Texture2D, Format::RGBA8SRGB, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput("GBuffer Material", AttachmentType::Texture2D, Format::R8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput("GBuffer Depth",    AttachmentType::Depth, Format::D24S8Float, SizeClass::Swapchain, LoadOp::Clear_White);
}


void GBufferMaterialTechnique::render(RenderGraph& graph, Engine *, const std::vector<const Scene::MeshInstance *>& meshes)
{

}


GBufferMaterialPreDepthTechnique::GBufferMaterialPreDepthTechnique(Engine* eng) :
    Technique{"GBufferMaterialPreDepth", eng->getDevice()},

    mDepthName{kPreDepth},
    mPipelineDescription{eng->getShader("./Shaders/GBufferPassThroughMaterial.vert"),
                         eng->getShader("./Shaders/GBufferMaterial.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, Primitive::TriangleList},

    mTask{"GBufferMaterialPreDepth", mPipelineDescription}
{
    mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Material |
                              VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    mTask.addInput("ModelMatrix", AttachmentType::PushConstants);

	mTask.addOutput(kGBufferNormals,  AttachmentType::Texture2D, Format::R16G16Unorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(kGBufferUV,       AttachmentType::Texture2D, Format::RGBA8SRGB, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(kGBufferMaterialID, AttachmentType::Texture2D, Format::R8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(mDepthName,         AttachmentType::Depth, Format::D24S8Float, SizeClass::Swapchain, LoadOp::Preserve);

}


void GBufferMaterialPreDepthTechnique::render(RenderGraph& graph, Engine*, const std::vector<const Scene::MeshInstance *>& meshes)
{

}
