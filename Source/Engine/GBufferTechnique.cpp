#include "Engine/GBufferTechnique.hpp"
#include "Core/Executor.hpp"


GBufferTechnique::GBufferTechnique(Engine* eng, RenderGraph& graph) :
	Technique{"GBuffer", eng->getDevice()},
    	mPipelineDescription{eng->getShader("./Shaders/GBufferPassThrough.vert"),
                         eng->getShader("./Shaders/GBuffer.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, true, DepthTest::GreaterEqual, Primitive::TriangleList}
{
    GraphicsTask task{ "GBuffer", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput("Model Matrix", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    task.addOutput(kGBufferAlbedo,    AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferNormals,     AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferMetalnessRoughness,   AttachmentType::RenderTarget2D, Format::RG8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferVelocity,   AttachmentType::RenderTarget2D, Format::RG16UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferDepth,      AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_Black);

    mTaskID = graph.addTask(task);
}


void GBufferTechnique::render(RenderGraph& graph, Engine*)
{
    GraphicsTask& task = static_cast<GraphicsTask&>(graph.getTask(mTaskID));

    task.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
        {
            exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
            exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

            for (const auto& mesh : meshes)
            {
                if (mesh->mMesh->getAttributes() & MeshAttributes::Transparent)
                    continue;

                const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

                const MeshEntry entry = mesh->getMeshShaderEntry();

                exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
                exec->indexedDraw(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
            }
        }
    );
}


GBufferPreDepthTechnique::GBufferPreDepthTechnique(Engine* eng, RenderGraph& graph) :
    Technique{"GBufferPreDepth", eng->getDevice()},
        mPipelineDescription{eng->getShader("./Shaders/GBufferPassThrough.vert"),
                         eng->getShader("./Shaders/GBuffer.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, Primitive::TriangleList}
{
    GraphicsTask task{ "GBuffer", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput("Model Matrix", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    task.addOutput(kGBufferAlbedo, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferNormals, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferMetalnessRoughness, AttachmentType::RenderTarget2D, Format::RG8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferVelocity, AttachmentType::RenderTarget2D, Format::RG16UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_Black);

    mTaskID = graph.addTask(task);
}


void GBufferPreDepthTechnique::render(RenderGraph& graph, Engine* eng)
{
    GraphicsTask& task = static_cast<GraphicsTask&>(graph.getTask(mTaskID));

    task.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
        {
            exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
            exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

            for (const auto& mesh : meshes)
            {
                if (mesh->mMesh->getAttributes() & MeshAttributes::Transparent)
                    continue;

                const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

                const MeshEntry entry = mesh->getMeshShaderEntry();

                exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
                exec->indexedDraw(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
            }
        }
    );
}
