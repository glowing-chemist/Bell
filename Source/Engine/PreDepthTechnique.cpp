#include "Engine/PreDepthTechnique.hpp"
#include "Core/Executor.hpp"


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

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput("Matrix", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_Black);

    mTaskID = graph.addTask(task);
}


void PreDepthTechnique::render(RenderGraph& graph, Engine*)
{
    GraphicsTask& task = static_cast<GraphicsTask&>(graph.getTask(mTaskID));

    task.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
        {
            exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
            exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

            for (const auto& mesh : meshes)
            {
                // Don't render transparent geometry.
                if ((mesh->getMaterialFlags() & MaterialType::Transparent) > 0 || !(mesh->getInstanceFlags() & InstanceFlags::Draw))
                    continue;

                const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

                const MeshEntry entry = mesh->getMeshShaderEntry();

                exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
                exec->indexedDraw(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
            }
        }
    );
}
