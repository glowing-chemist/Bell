#include "Engine/ShadowMappingTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"


ShadowMappingTechnique::ShadowMappingTechnique(Engine* eng, RenderGraph& graph) :
    Technique("ShadowMapping", eng->getDevice()),
    mDesc(eng->getShader("./Shaders/ShadowMap.vert"),
          eng->getShader("./Shaders/VarianceShadowMap.frag"),
          Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() * 2,
                getDevice()->getSwapChain()->getSwapChainImageHeight() * 2},
          Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() * 2,
          getDevice()->getSwapChain()->getSwapChainImageHeight() * 2},
        true, BlendMode::None, BlendMode::None, true, DepthTest::LessEqual, Primitive::TriangleList),
    mBlurXDesc{ eng->getShader("Shaders/blurXrg32f.comp") },
    mBlurYDesc{ eng->getShader("Shaders/blurYrg32f.comp") },
    mResolveDesc{eng->getShader("./Shaders/ResolveVarianceShadowMap.comp")},

    mShadowMap(getDevice(), Format::R8UNorm, ImageUsage::Storage | ImageUsage::Sampled, getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
               1, 1, 1, 1, "ShadowMap"),
    mShadowMapView(mShadowMap, ImageViewType::Colour),
    mShadowMapIntermediate(getDevice(), Format::RG32Float, ImageUsage::Storage | ImageUsage::Sampled, getDevice()->getSwapChain()->getSwapChainImageWidth() * 2, getDevice()->getSwapChain()->getSwapChainImageHeight() * 2,
               1, 1, 1, 1, "ShadowMapIntermediate"),
    mShadowMapIntermediateView(mShadowMapIntermediate, ImageViewType::Colour),
    mShadowMapBlured(getDevice(), Format::RG32Float, ImageUsage::Storage | ImageUsage::Sampled, getDevice()->getSwapChain()->getSwapChainImageWidth() * 2, getDevice()->getSwapChain()->getSwapChainImageHeight() * 2,
               1, 1, 1, 1, "ShadowMapBlured"),
    mShadowMapBluredView(mShadowMapBlured, ImageViewType::Colour)
{
    GraphicsTask shadowTask{ "ShadowMapping", mDesc };
    shadowTask.setVertexAttributes(VertexAttributes::Position4 |
                              VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    shadowTask.addInput(kShadowingLights, AttachmentType::UniformBuffer);
    shadowTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    shadowTask.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    shadowTask.addInput("lightMatrix", AttachmentType::PushConstants);
    shadowTask.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    shadowTask.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    shadowTask.addOutput(kShadowMapRaw, AttachmentType::RenderTarget2D, Format::RG32Float, SizeClass::DoubleSwapchain, LoadOp::Clear_Float_Max);
    shadowTask.addOutput("ShadowMapDepth", AttachmentType::Depth, Format::D32Float, SizeClass::DoubleSwapchain, LoadOp::Clear_White);
    mShadowTask = graph.addTask(shadowTask);

    ComputeTask blurXTask{ "ShadowMapBlurX", mBlurXDesc };
    blurXTask.addInput(kShadowMapRaw, AttachmentType::Texture2D);
    blurXTask.addInput(kShadowMapBlurIntermediate, AttachmentType::Image2D);
    blurXTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    mBlurXTaskID = graph.addTask(blurXTask);

    ComputeTask blurYTask{ "ShadowMapBlurY", mBlurYDesc };
    blurYTask.addInput(kShadowMapBlurIntermediate, AttachmentType::Texture2D);
    blurYTask.addInput(kShadowMapBlured, AttachmentType::Image2D);
    blurYTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    mBlurYTaskID = graph.addTask(blurYTask);

    ComputeTask resolveTask{ "Resolve shadow mapping", mResolveDesc };
    resolveTask.addInput(kGBufferDepth, AttachmentType::Texture2D);
    resolveTask.addInput(kShadowMapBlured, AttachmentType::Texture2D);
    resolveTask.addInput(kShadowMap, AttachmentType::Image2D);
    resolveTask.addInput(kShadowingLights, AttachmentType::UniformBuffer);
    resolveTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    resolveTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    mResolveTaskID = graph.addTask(resolveTask);
}


void ShadowMappingTechnique::render(RenderGraph& graph, Engine*)
{
    (*mShadowMap)->updateLastAccessed();
    (*mShadowMapView)->updateLastAccessed();
    (*mShadowMapIntermediate)->updateLastAccessed();
    (*mShadowMapIntermediateView)->updateLastAccessed();
    (*mShadowMapBlured)->updateLastAccessed();
    (*mShadowMapBluredView)->updateLastAccessed();

    GraphicsTask& shadowTask = static_cast<GraphicsTask&>(graph.getTask(mShadowTask));
    ComputeTask& blurXTask = static_cast<ComputeTask&>(graph.getTask(mBlurXTaskID));
    ComputeTask& blurYTask = static_cast<ComputeTask&>(graph.getTask(mBlurYTaskID));
    ComputeTask& resolveTask = static_cast<ComputeTask&>(graph.getTask(mResolveTaskID));

    shadowTask.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            const Frustum lightFrustum = eng->getScene()->getShadowingLightFrustum();
            std::vector<const MeshInstance*> meshes = eng->getScene()->getViewableMeshes(lightFrustum);
            std::sort(meshes.begin(), meshes.end(), [lightPosition = eng->getScene()->getShadowingLight().mPosition](const MeshInstance* lhs, const MeshInstance* rhs)
            {
                const float3 centralLeft = lhs->mMesh->getAABB().getCentralPoint();
                const float leftDistance = glm::length(centralLeft - float3(lightPosition));

                const float3 centralright = rhs->mMesh->getAABB().getCentralPoint();
                const float rightDistance = glm::length(centralright - float3(lightPosition));

                return leftDistance < rightDistance;
            });

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

    blurXTask.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
            const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

            exec->dispatch(std::ceil(threadGroupWidth / 128.0f), threadGroupHeight * 2, 1.0f);
        }
    );

    blurYTask.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
            const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

            exec->dispatch(threadGroupWidth * 2, std::ceil(threadGroupHeight / 128.0f), 1.0f);
        }
    );

    resolveTask.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
            const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

            exec->dispatch( static_cast<uint32_t>(std::ceil(threadGroupWidth / 32.0f)),
                            static_cast<uint32_t>(std::ceil(threadGroupHeight / 32.0f)),
                            1);
        }
    );
}
