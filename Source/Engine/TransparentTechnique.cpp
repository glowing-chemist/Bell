#include "Engine/TransparentTechnique.hpp"
#include "Engine/Engine.hpp"

#include "Core/Executor.hpp"

TransparentTechnique::TransparentTechnique(Engine* eng, RenderGraph& graph) :
    Technique{"GBuffer", eng->getDevice()},
        mPipelineDescription{eng->getShader("./Shaders/ForwardMaterial.vert"),
                         eng->getShader("./Shaders/Transparent.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::Add, BlendMode::Add, false, DepthTest::GreaterEqual, Primitive::TriangleList}
{
    GraphicsTask task{ "Transparent", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

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

    task.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Custom, LoadOp::Preserve);
    task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, SizeClass::Custom, LoadOp::Preserve);

    task.setRecordCommandsCallback(
                [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
                {
                    exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                    exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                    for (const auto& mesh : meshes)
                    {
                        if (!(mesh->getMaterialFlags() & MaterialType::Transparent) || !(mesh->getInstanceFlags() & InstanceFlags::Draw))
                            continue;

                        const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

                        const MeshEntry entry = mesh->getMeshShaderEntry();

                        exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
                        exec->indexedDraw(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
                    }
                }
    );

    mTaskID = graph.addTask(task);
}
