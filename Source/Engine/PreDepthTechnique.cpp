#include "Engine/PreDepthTechnique.hpp"
#include "Engine/UberShaderStateCache.hpp"
#include "Core/Executor.hpp"


PreDepthTechnique::PreDepthTechnique(RenderEngine* eng, RenderGraph& graph) :
    Technique{"PreDepth", eng->getDevice()},
    mPipelineDescription{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         FaceWindingOrder::CW, BlendMode::None, BlendMode::None, true, DepthTest::GreaterEqual, FillMode::Fill, Primitive::TriangleList},
    mPreDepthVertexShader(eng->getShader("./Shaders/DepthOnly.vert")),
    mPreDepthFragmentShader(eng->getShader("./Shaders/AlphaTestDepthOnly.frag"))
{
    GraphicsTask task{ "PreDepth", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::Tangents | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

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
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                PROFILER_EVENT("Pre-Z");
                PROFILER_GPU_TASK(exec);
                PROFILER_GPU_EVENT("Pre-Z");

                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                const RenderTask& task = graph.getTask(taskIndex);
                exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mPreDepthVertexShader, nullptr, nullptr, nullptr, mPreDepthFragmentShader);

                UberShaderStateCache stateCache(exec, graph, task);

                const BufferView& pred = eng->getRenderGraph().getBuffer(kOcclusionPredicationBuffer);

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
            }
        );
    }
    else
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                PROFILER_EVENT("Pre-Z");
                PROFILER_GPU_TASK(exec);
                PROFILER_GPU_EVENT("Pre-Z");

                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                const RenderTask& task = graph.getTask(taskIndex);
                exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mPreDepthVertexShader, nullptr, nullptr, nullptr, mPreDepthFragmentShader);

                UberShaderStateCache stateCache(exec, graph, task);

                for (const auto& mesh : meshes)
                {
                    // Don't render transparent geometry.
                    if (!(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->getMesh());

                    mesh->draw(exec, &stateCache, vertexOffset, indexOffset);
                }
            }
        );
    }

    mTaskID = graph.addTask(task);
}
