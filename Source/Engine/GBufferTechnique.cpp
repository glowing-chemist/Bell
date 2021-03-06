#include "Engine/GBufferTechnique.hpp"
#include "Engine/UberShaderStateCache.hpp"
#include "Core/Executor.hpp"


GBufferTechnique::GBufferTechnique(RenderEngine* eng, RenderGraph& graph) :
	Technique{"GBuffer", eng->getDevice()},
        mPipelineDescription{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         FaceWindingOrder::CW, BlendMode::None, BlendMode::None, true, DepthTest::GreaterEqual, FillMode::Fill, Primitive::TriangleList}
{
    GraphicsTask task{ "GBuffer", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::Tangents | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput("Model Matrix", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    task.addManagedOutput(kGBufferDiffuse,    AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferNormals,     AttachmentType::RenderTarget2D, Format::RG8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferSpecularRoughness,   AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferVelocity,   AttachmentType::RenderTarget2D, Format::RG16UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferEmissiveOcclusion,   AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_ColourBlack_AlphaWhite);
    task.addManagedOutput(kGBufferDepth,      AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_Black, StoreOp::Store, ImageUsage::DepthStencil | ImageUsage::Sampled);

    if(eng->isPassRegistered(PassType::OcclusionCulling))
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                PROFILER_EVENT("gbuffer fill");
                PROFILER_GPU_TASK(exec);
                PROFILER_GPU_EVENT("gbuffer fill");

                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                const RenderTask& task = graph.getTask(taskIndex);
                Shader vertexShader = eng->getShader("./Shaders/GBufferPassThrough.vert");

                const BufferView& pred = eng->getRenderGraph().getBuffer(kOcclusionPredicationBuffer);

                UberShaderMaterialStateCache stateCache(exec, eng, graph, task, vertexShader, "./Shaders/GBuffer.frag");

                for (uint32_t i = 0; i < meshes.size(); ++i)
                {
                    const auto* mesh = meshes[i];

                    if (!(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->getMesh());

                    exec->startCommandPredication(pred, i);

                    mesh->draw(exec, &stateCache, vertexOffset, indexOffset);

                    exec->endCommandPredication();
                }

                exec->setSubmitFlag();
            }
        );
    }
    else
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                PROFILER_EVENT("gbuffer fill");
                PROFILER_GPU_TASK(exec);
                PROFILER_GPU_EVENT("gbuffer fill");

                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                const RenderTask& task = graph.getTask(taskIndex);
                Shader vertexShader = eng->getShader("./Shaders/GBufferPassThrough.vert");

                UberShaderMaterialStateCache stateCache(exec, eng, graph, task, vertexShader, "./Shaders/GBuffer.frag");

                for (const auto& mesh : meshes)
                {
                    if (!(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->getMesh());
                    mesh->draw(exec, &stateCache, vertexOffset, indexOffset);

                   }

                exec->setSubmitFlag();
            }
        );
    }

    mTaskID = graph.addTask(task);
}


GBufferPreDepthTechnique::GBufferPreDepthTechnique(RenderEngine* eng, RenderGraph& graph) :
    Technique{"GBufferPreDepth", eng->getDevice()},
        mPipelineDescription{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         FaceWindingOrder::CW, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, FillMode::Fill, Primitive::TriangleList}
{
    GraphicsTask task{ "GBufferPreDepth", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::Tangents | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput("Model Matrix", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    task.addManagedOutput(kGBufferDiffuse, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferNormals, AttachmentType::RenderTarget2D, Format::RG8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferSpecularRoughness, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferVelocity, AttachmentType::RenderTarget2D, Format::RG16UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferEmissiveOcclusion, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_ColourBlack_AlphaWhite);
    task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, LoadOp::Preserve);

    if(eng->isPassRegistered(PassType::OcclusionCulling))
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                PROFILER_EVENT("gbuffer fill");
                PROFILER_GPU_TASK(exec);
                PROFILER_GPU_EVENT("gbuffer fill");

                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                const RenderTask& task = graph.getTask(taskIndex);
                Shader vertexShader = eng->getShader("./Shaders/GBufferPassThrough.vert");

                const BufferView& pred = eng->getRenderGraph().getBuffer(kOcclusionPredicationBuffer);

                UberShaderMaterialStateCache stateCache(exec, eng, graph, task, vertexShader, "./Shaders/GBuffer.frag");

                for (uint32_t i = 0; i < meshes.size(); ++i)
                {
                    const auto& mesh = meshes[i];

                    if (!(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->getMesh());


                    exec->startCommandPredication(pred, i);

                    mesh->draw(exec, &stateCache, vertexOffset, indexOffset);

                    exec->endCommandPredication();
                }

                exec->setSubmitFlag();
            }
        );
    }
    else
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                PROFILER_EVENT("gbuffer fill");
                PROFILER_GPU_TASK(exec);
                PROFILER_GPU_EVENT("gbuffer fill");

                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);


                const RenderTask& task = graph.getTask(taskIndex);
                Shader vertexShader = eng->getShader("./Shaders/GBufferPassThrough.vert");

                UberShaderMaterialStateCache stateCache(exec, eng, graph, task, vertexShader, "./Shaders/GBuffer.frag");

                for (const auto& mesh : meshes)
                {
                    if (!(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->getMesh());

                    mesh->draw(exec, &stateCache, vertexOffset, indexOffset);
                  }

                exec->setSubmitFlag();
            }
        );
    }

    mTaskID = graph.addTask(task);
}
