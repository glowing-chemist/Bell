#include "Engine/TransparentTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/UberShaderStateCache.hpp"

#include "Core/Executor.hpp"

TransparentTechnique::TransparentTechnique(RenderEngine* eng, RenderGraph& graph) :
    Technique{"GBuffer", eng->getDevice()},
        mPipelineDescription{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         FaceWindingOrder::CW, BlendMode::Add, BlendMode::Add, false, DepthTest::GreaterEqual, FillMode::Fill, Primitive::TriangleList},
        mTransparentVertexShader(eng->getShader("./Shaders/ForwardMaterial.vert")),
        mTransparentFragmentShader(eng->getShader("./Shaders/Transparent.frag"))
{
    GraphicsTask task{ "Transparent", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::Tangents | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDFGLUT, AttachmentType::Texture2D);
    task.addInput(kConvolvedSpecularSkyBox, AttachmentType::Texture2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);

    if (eng->isPassRegistered(PassType::Shadow) || eng->isPassRegistered(PassType::CascadingShadow))
        task.addInput(kShadowMap, AttachmentType::Texture2D);

    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput("model", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    task.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm);
    task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float);

    task.setRecordCommandsCallback(
                [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
                {
                    PROFILER_EVENT("transparent render");
                    PROFILER_GPU_TASK(exec);
                    PROFILER_GPU_EVENT("transparent render");

                    exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                    exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                    const RenderTask& task = graph.getTask(taskIndex);
                    exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mTransparentVertexShader, nullptr, nullptr, nullptr, mTransparentFragmentShader);

                    UberShaderStateCache stateCache(exec);

                    for (const auto& mesh : meshes)
                    {
                        if (!(mesh->getInstanceFlags() & InstanceFlags::Draw))
                            continue;

                        const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->getMesh());

                        mesh->draw(exec, &stateCache, vertexOffset, indexOffset);
                    }
                }
    );

    mTaskID = graph.addTask(task);
}
