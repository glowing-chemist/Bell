#include "Engine/PreDepthTechnique.hpp"
#include "Core/Executor.hpp"


PreDepthTechnique::PreDepthTechnique(Engine* eng, RenderGraph& graph) :
    Technique{"PreDepth", eng->getDevice()},
    mPipelineDescription{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, true, DepthTest::GreaterEqual, FillMode::Fill, Primitive::TriangleList},
    mPreDepthVertexShader(eng->getShader("./Shaders/DepthOnly.vert")),
    mPreDepthFragmentShader(eng->getShader("./Shaders/AlphaTestDepthOnly.frag"))
{
    GraphicsTask task{ "PreDepth", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput("Matrix", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    if(eng->isPassRegistered(PassType::OcclusionCulling))
        task.addInput(kOcclusionPredicationBuffer, AttachmentType::CommandPredicationBuffer);

    task.addManagedOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_Black, StoreOp::Store, ImageUsage::DepthStencil | ImageUsage::Sampled);

    if(eng->isPassRegistered(PassType::OcclusionCulling))
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                const RenderTask& task = graph.getTask(taskIndex);
                exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mPreDepthVertexShader, nullptr, nullptr, nullptr, mPreDepthFragmentShader);

                const BufferView& pred = eng->getRenderGraph().getBuffer(kOcclusionPredicationBuffer);

                for (uint32_t i = 0; i < meshes.size(); ++i)
                {
                    const auto& mesh = meshes[i];

                    if (mesh->getMaterialFlags() & MaterialType::Transparent || !(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->getMesh());

                    const MeshEntry entry = mesh->getMeshShaderEntry();

                    exec->startCommandPredication(pred, i);

                    exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
                    exec->indexedDraw(vertexOffset / mesh->getMesh()->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->getMesh()->getIndexData().size());

                    exec->endCommandPredication();
                }
            }
        );
    }
    else
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                const RenderTask& task = graph.getTask(taskIndex);
                exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mPreDepthVertexShader, nullptr, nullptr, nullptr, mPreDepthFragmentShader);

                for (const auto& mesh : meshes)
                {
                    // Don't render transparent geometry.
                    if ((mesh->getMaterialFlags() & MaterialType::Transparent) > 0 || !(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->getMesh());

                    const MeshEntry entry = mesh->getMeshShaderEntry();

                    exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
                    exec->indexedDraw(vertexOffset / mesh->getMesh()->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->getMesh()->getIndexData().size());
                }
            }
        );
    }

    mTaskID = graph.addTask(task);
}
