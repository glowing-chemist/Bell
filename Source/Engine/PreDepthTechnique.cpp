#include "Engine/PreDepthTechnique.hpp"


PreDepthTechnique::PreDepthTechnique(Engine* eng, RenderGraph& graph) :
    Technique{"PreDepth", eng->getDevice()},
    mPipelineDescription{eng->getShader("./Shaders/DepthOnly.vert"),
                         eng->getShader("./Shaders/AlphaTestDepthOnly.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, true, DepthTest::GreaterEqual, Primitive::TriangleList}
{
    GraphicsTask task{ "PreDepth", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Material);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput("Matrix", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_Black);

    mTaskID = graph.addTask(task);
}


void PreDepthTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance *>& meshes)
{
    GraphicsTask& task = static_cast<GraphicsTask&>(graph.getTask(mTaskID));

    task.clearCalls();

    for (const auto& mesh : meshes)
    {
        // Don't render transparent geometry.
        if((mesh->mMesh->getAttributes() & MeshAttributes::Transparent) > 0)
            continue;

        const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

        const MeshEntry entry = mesh->getMeshShaderEntry();

        task.addPushConsatntValue(&entry, sizeof(MeshEntry));
        task.addIndexedDrawCall(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
    }
}
