#include "Engine/PreDepthTechnique.hpp"


PreDepthTechnique::PreDepthTechnique(Engine* eng) :
    Technique{"PreDepth", eng->getDevice()},
    mPipelineDescription{eng->getShader("./Shaders/DepthOnly.vert"),
                         eng->getShader("./Shaders/Empty.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, true, DepthTest::GreaterEqual, Primitive::TriangleList},

    mTask{"PreDepth", mPipelineDescription}
{
    mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Material);

    mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	mTask.addInput("Matrix", AttachmentType::PushConstants);
    mTask.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    mTask.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

	mTask.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_Black);
}


void PreDepthTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance *>& meshes)
{
	mTask.clearCalls();

    for (const auto& mesh : meshes)
    {
        // Don't render transparent or alpha tested geometry.
        if((mesh->mMesh->getAttributes() & (MeshAttributes::AlphaTested | MeshAttributes::Transparent)) > 0)
            continue;

        const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

        mTask.addPushConsatntValue(mesh->mTransformation);
        mTask.addIndexedDrawCall(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
    }

	graph.addTask(mTask);
}
