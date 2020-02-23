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
    mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Material);

    mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    mTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    mTask.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    mTask.addInput("Model Matrix", AttachmentType::PushConstants);
    mTask.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    mTask.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    mTask.addOutput(kGBufferAlbedo,    AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    mTask.addOutput(kGBufferNormals,     AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    mTask.addOutput(kGBufferMetalnessRoughness,   AttachmentType::RenderTarget2D, Format::RG8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    mTask.addOutput(kGBufferDepth,      AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_Black);
}


void GBufferTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance *>& meshes)
{
	mTask.clearCalls();

	for (const auto& mesh : meshes)
	{
		const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

		mTask.addPushConsatntValue(mesh->mTransformation);
        mTask.addIndexedDrawCall(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
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
                         true, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, Primitive::TriangleList},

    mTask{"GBufferPreDepth", mPipelineDescription}
{
    mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Material);

    mTask.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    mTask.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);
    mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    mTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    mTask.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    mTask.addInput("Model Matrix", AttachmentType::PushConstants);

    mTask.addOutput(kGBufferAlbedo,  AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    mTask.addOutput(kGBufferNormals,   AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    mTask.addOutput(kGBufferMetalnessRoughness,   AttachmentType::RenderTarget2D, Format::RG8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    mTask.addOutput(kGBufferDepth,    AttachmentType::Depth, Format::D32Float, SizeClass::Custom, LoadOp::Preserve);
}


void GBufferPreDepthTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance *>& meshes)
{
	mTask.clearCalls();

    for (const auto& mesh : meshes)
    {
        const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

        mTask.addPushConsatntValue(mesh->mTransformation);
        mTask.addIndexedDrawCall(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
    }

	graph.addTask(mTask);
}
