#include "Engine/GBufferMaterialTechnique.hpp"

#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"

GBufferMaterialTechnique::GBufferMaterialTechnique(Engine* eng, RenderGraph& graph) :
    Technique{"GBufferMaterial", eng->getDevice()},
    mPipelineDescription{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, true, DepthTest::GreaterEqual, FillMode::Fill, Primitive::TriangleList},
    mGbufferVertexShader(eng->getShader("./Shaders/GBufferPassThrough.vert")),
    mGbufferFragmentShader(eng->getShader("./Shaders/GBufferDeferredTexturing.frag"))
{
    GraphicsTask task{ "GBufferMaterial", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 |
                              VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput("ModelMatrix", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    task.addManagedOutput(kGBufferNormals,  AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferUV,       AttachmentType::RenderTarget2D, Format::RGBA32Float, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferMaterialID, AttachmentType::RenderTarget2D, Format::R32Uint, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferVelocity,   AttachmentType::RenderTarget2D, Format::RG16UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferDepth,    AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_Black, StoreOp::Store, ImageUsage::DepthStencil | ImageUsage::Sampled);

    if(eng->isPassRegistered(PassType::OcclusionCulling))
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                const RenderTask& task = graph.getTask(taskIndex);
                exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mGbufferVertexShader, nullptr, nullptr, nullptr, mGbufferFragmentShader);

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
                exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mGbufferVertexShader, nullptr, nullptr, nullptr, mGbufferFragmentShader);

                for (const auto& mesh : meshes)
                {
                    if (mesh->getMaterialFlags() & MaterialType::Transparent || !(mesh->getInstanceFlags() & InstanceFlags::Draw))
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


GBufferMaterialPreDepthTechnique::GBufferMaterialPreDepthTechnique(Engine* eng, RenderGraph& graph) :
    Technique{"GBufferMaterialPreDepth", eng->getDevice()},

    mDepthName{kGBufferDepth},
    mPipelineDescription{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, FillMode::Fill, Primitive::TriangleList},
    mGbufferVertexShader(eng->getShader("./Shaders/GBufferPassThrough.vert")),
    mGbufferFragmentShader(eng->getShader("./Shaders/GBufferDeferredTexturing.frag"))
{
    GraphicsTask task{ "GBufferMaterial", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 |
        VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput("ModelMatrix", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    task.addManagedOutput(kGBufferNormals, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferUV, AttachmentType::RenderTarget2D, Format::RGBA32Float, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferMaterialID, AttachmentType::RenderTarget2D, Format::R32Uint, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferVelocity, AttachmentType::RenderTarget2D, Format::RG16UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, LoadOp::Preserve);

    if(eng->isPassRegistered(PassType::OcclusionCulling))
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                const RenderTask& task = graph.getTask(taskIndex);
                exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mGbufferVertexShader, nullptr, nullptr, nullptr, mGbufferFragmentShader);

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
                exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mGbufferVertexShader, nullptr, nullptr, nullptr, mGbufferFragmentShader);

                for (const auto& mesh : meshes)
                {
                    if (mesh->getMaterialFlags() & MaterialType::Transparent || !(mesh->getInstanceFlags() & InstanceFlags::Draw))
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
