#include "Engine/GBufferMaterialTechnique.hpp"

#include "Engine/DefaultResourceSlots.hpp"


GBufferMaterialTechnique::GBufferMaterialTechnique(Engine* eng, RenderGraph& graph) :
    Technique{"GBufferMaterial", eng->getDevice()},
    mPipelineDescription{eng->getShader("./Shaders/GBufferPassThroughMaterial.vert"),
                         eng->getShader("./Shaders/GBufferDeferredTexturing.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, true, DepthTest::GreaterEqual, Primitive::TriangleList}
{
    GraphicsTask task{ "GBufferMaterial", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Material |
                              VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput("ModelMatrix", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    task.addOutput(kGBufferNormals,  AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferUV,       AttachmentType::RenderTarget2D, Format::RGBA32Float, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferMaterialID, AttachmentType::RenderTarget2D, Format::R32Uint, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferVelocity,   AttachmentType::RenderTarget2D, Format::RG16UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferDepth,    AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_Black);

    mTaskID = graph.addTask(task);
}


void GBufferMaterialTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance *>& meshes)
{
    GraphicsTask& task = static_cast<GraphicsTask&>(graph.getTask(mTaskID));

    task.clearCalls();

    for (const auto& mesh : meshes)
    {
        const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

        const MeshEntry entry = mesh->getMeshShaderEntry();

        task.addPushConsatntValue(&entry, sizeof(MeshEntry));
        task.addIndexedDrawCall(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
    }
}


GBufferMaterialPreDepthTechnique::GBufferMaterialPreDepthTechnique(Engine* eng, RenderGraph& graph) :
    Technique{"GBufferMaterialPreDepth", eng->getDevice()},

    mDepthName{kGBufferDepth},
    mPipelineDescription{eng->getShader("./Shaders/GBufferPassThroughMaterial.vert"),
                         eng->getShader("./Shaders/GBufferDeferredTexturing.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, Primitive::TriangleList}
{
    GraphicsTask task{ "GBufferMaterial", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Material |
        VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput("ModelMatrix", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    task.addOutput(kGBufferNormals, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferUV, AttachmentType::RenderTarget2D, Format::RGBA32Float, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferMaterialID, AttachmentType::RenderTarget2D, Format::R32Uint, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferVelocity, AttachmentType::RenderTarget2D, Format::RG16UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_Black);

    mTaskID = graph.addTask(task);

}


void GBufferMaterialPreDepthTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance *>& meshes)
{
    GraphicsTask& task = static_cast<GraphicsTask&>(graph.getTask(mTaskID));

    task.clearCalls();

    for (const auto& mesh : meshes)
    {
        const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

        const MeshEntry entry = mesh->getMeshShaderEntry();

        task.addPushConsatntValue(&entry, sizeof(MeshEntry));
        task.addIndexedDrawCall(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
    }
}
