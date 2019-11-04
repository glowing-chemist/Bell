#include "Engine/GBufferTechnique.hpp"


GBufferTechnique::GBufferTechnique(Engine* eng) :
	Technique{"GBuffer", eng->getDevice()},
    	mPipelineDescription{eng->getShader("./Shaders/GBufferPassThrough.vert"),
                         eng->getShader("./Shaders/GBuffer.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, true, DepthTest::LessEqual, Primitive::TriangleList},

    mTask{"GBuffer", mPipelineDescription}
{
    mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    mTask.addInput("Model Matrix", AttachmentType::PushConstants);

	mTask.addOutput(kGBufferNormals, AttachmentType::Texture2D, Format::R16G16Unorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(kGBufferAlbedo, AttachmentType::Texture2D, Format::RGBA8SRGB, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(kGBufferSpecular, AttachmentType::Texture2D, Format::R8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_White);
}


void GBufferTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance *>& meshes)
{
	mTask.clearCalls();

	for(const auto& mesh : meshes)
	{
		const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

		mTask.addPushConsatntValue(mesh->mTransformation);
		mTask.addIndexedDrawCall(vertexOffset, indexOffset, mesh->mMesh->getIndexData().size());
	}

	graph.addTask(mTask);
}


GBufferPreDepthTechnique::GBufferPreDepthTechnique(Engine* eng) :
    Technique{"GBufferPreDepth", eng->getDevice()},
        mPipelineDescription{eng->getShader("./Shaders/GBufferPassThrough.vert"),
                         eng->getShader("./Shaders/GBuffer.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, false, DepthTest::LessEqual, Primitive::TriangleList},

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


void GBufferPreDepthTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance *>& meshes)
{
	mTask.clearCalls();

	for(const auto& mesh : meshes)
	{
		const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

		mTask.addPushConsatntValue(mesh->mTransformation);
		mTask.addIndexedDrawCall(vertexOffset, indexOffset, mesh->mMesh->getIndexData().size());
	}

	graph.addTask(mTask);
}
