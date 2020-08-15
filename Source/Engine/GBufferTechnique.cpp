#include "Engine/GBufferTechnique.hpp"
#include "Core/Executor.hpp"


GBufferTechnique::GBufferTechnique(Engine* eng, RenderGraph& graph) :
	Technique{"GBuffer", eng->getDevice()},
        mPipelineDescription{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, true, DepthTest::GreaterEqual, Primitive::TriangleList}
{
    GraphicsTask task{ "GBuffer", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput("Model Matrix", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    task.addOutput(kGBufferDiffuse,    AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferNormals,     AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferSpecularRoughness,   AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferVelocity,   AttachmentType::RenderTarget2D, Format::RG16UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferEmissiveOcclusion,   AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_ColourBlack_AlphaWhite);
    task.addOutput(kGBufferDepth,      AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_Black);

    if(eng->isPassRegistered(PassType::OcclusionCulling))
    {
        task.setRecordCommandsCallback(
            [](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                uint64_t currentMaterialFLags = 0;

                const RenderTask& task = graph.getTask(taskIndex);
                Shader vertexShader = eng->getShader("./Shaders/GBufferPassThrough.vert");

                const BufferView& pred = eng->getRenderGraph().getBuffer(kOcclusionPredicationBuffer);

                for (uint32_t i = 0; i < meshes.size(); ++i)
                {
                    const auto& mesh = meshes[i];

                    if (mesh->getMaterialFlags() & MaterialType::Transparent || !(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    //need to set new pipeline.
                    if(mesh->getMaterialFlags() != currentMaterialFLags)
                    {
                        currentMaterialFLags = mesh->getMaterialFlags();

                        ShaderDefine materialDefine("MATERIAL_FLAGS", currentMaterialFLags);
                        Shader fragmentShader = eng->getShader("./Shaders/GBuffer.frag", materialDefine);
                        exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, vertexShader, nullptr, nullptr, nullptr, fragmentShader);
                    }

                    const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

                    const MeshEntry entry = mesh->getMeshShaderEntry();

                    exec->startCommandPredication(pred, i);

                    exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
                    exec->indexedDraw(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());

                    exec->endCommandPredication();
                }
            }
        );
    }
    else
    {
        task.setRecordCommandsCallback(
            [](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                uint64_t currentMaterialFLags = 0;

                const RenderTask& task = graph.getTask(taskIndex);
                Shader vertexShader = eng->getShader("./Shaders/GBufferPassThrough.vert");

                for (const auto& mesh : meshes)
                {
                    if (mesh->getMaterialFlags() & MaterialType::Transparent || !(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    //need to set new pipeline.
                    if(mesh->getMaterialFlags() != currentMaterialFLags)
                    {
                        currentMaterialFLags = mesh->getMaterialFlags();

                        ShaderDefine materialDefine("MATERIAL_FLAGS", currentMaterialFLags);
                        Shader fragmentShader = eng->getShader("./Shaders/GBuffer.frag", materialDefine);
                        exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, vertexShader, nullptr, nullptr, nullptr, fragmentShader);
                    }

                    const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

                    const MeshEntry entry = mesh->getMeshShaderEntry();

                    exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
                    exec->indexedDraw(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
                }
            }
        );
    }

    mTaskID = graph.addTask(task);
}


GBufferPreDepthTechnique::GBufferPreDepthTechnique(Engine* eng, RenderGraph& graph) :
    Technique{"GBufferPreDepth", eng->getDevice()},
        mPipelineDescription{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, Primitive::TriangleList}
{
    GraphicsTask task{ "GBufferPreDepth", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput("Model Matrix", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    task.addOutput(kGBufferDiffuse, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferNormals, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferSpecularRoughness, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferVelocity, AttachmentType::RenderTarget2D, Format::RG16UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addOutput(kGBufferEmissiveOcclusion,   AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_ColourBlack_AlphaWhite);
    task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, SizeClass::Custom, LoadOp::Preserve);

    if(eng->isPassRegistered(PassType::OcclusionCulling))
    {
        task.setRecordCommandsCallback(
            [](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);
                uint64_t currentMaterialFLags = 0;

                const RenderTask& task = graph.getTask(taskIndex);
                Shader vertexShader = eng->getShader("./Shaders/GBufferPassThrough.vert");

                const BufferView& pred = eng->getRenderGraph().getBuffer(kOcclusionPredicationBuffer);

                for (uint32_t i = 0; i < meshes.size(); ++i)
                {
                    const auto& mesh = meshes[i];

                    if (mesh->getMaterialFlags() & MaterialType::Transparent || !(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    //need to set new pipeline.
                    if(mesh->getMaterialFlags() != currentMaterialFLags)
                    {
                        currentMaterialFLags = mesh->getMaterialFlags();

                        ShaderDefine materialDefine("MATERIAL_FLAGS", currentMaterialFLags);
                        Shader fragmentShader = eng->getShader("./Shaders/GBuffer.frag", materialDefine);
                        exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, vertexShader, nullptr, nullptr, nullptr, fragmentShader);
                    }

                    const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

                    const MeshEntry entry = mesh->getMeshShaderEntry();

                    exec->startCommandPredication(pred, i);

                    exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
                    exec->indexedDraw(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());

                    exec->endCommandPredication();
                }
            }
        );
    }
    else
    {
        task.setRecordCommandsCallback(
            [](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                uint64_t currentMaterialFLags = 0;

                const RenderTask& task = graph.getTask(taskIndex);
                Shader vertexShader = eng->getShader("./Shaders/GBufferPassThrough.vert");

                for (const auto& mesh : meshes)
                {
                    if (mesh->getMaterialFlags() & MaterialType::Transparent || !(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    //need to set new pipeline.
                    if(mesh->getMaterialFlags() != currentMaterialFLags)
                    {
                        currentMaterialFLags = mesh->getMaterialFlags();

                        ShaderDefine materialDefine("MATERIAL_FLAGS", currentMaterialFLags);
                        Shader fragmentShader = eng->getShader("./Shaders/GBuffer.frag", materialDefine);
                        exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, vertexShader, nullptr, nullptr, nullptr, fragmentShader);
                    }

                    const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

                    const MeshEntry entry = mesh->getMeshShaderEntry();

                    exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
                    exec->indexedDraw(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
                }
            }
        );
    }

    mTaskID = graph.addTask(task);
}
