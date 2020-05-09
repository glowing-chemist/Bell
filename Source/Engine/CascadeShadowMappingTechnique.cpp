#include "Engine/CascadeShadowMappingTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"


CascadeShadowMappingTechnique::CascadeShadowMappingTechnique(Engine* eng, RenderGraph& graph) :
    Technique("Cascade shadow mapping", eng->getDevice()),
    mBlurXDesc{ eng->getShader("Shaders/blurXrg32f.comp") },
    mBlurYDesc{ eng->getShader("Shaders/blurYrg32f.comp") },
    mResolveDesc{eng->getShader("./Shaders/ResolveCascadeVarianceShadowMap.comp")},

    mCascadeShadowMaps(eng->getDevice(), Format::RG32Float, ImageUsage::ColourAttachment | ImageUsage::Sampled,
                       getDevice()->getSwapChain()->getSwapChainImageWidth() * 2,
                       getDevice()->getSwapChain()->getSwapChainImageHeight() * 2,
                       1, 3, 1, 1, "cascade shadow map"),
    mCascadeShaowMapsViewMip0(mCascadeShadowMaps, ImageViewType::Colour, 0, 1, 0, 1),
    mCascadeShaowMapsViewMip1(mCascadeShadowMaps, ImageViewType::Colour, 0, 1, 1, 1),
    mCascadeShaowMapsViewMip2(mCascadeShadowMaps, ImageViewType::Colour, 0, 1, 2, 1),

    mResolvedShadowMap(eng->getDevice(), Format::R8UNorm, ImageUsage::Storage | ImageUsage::Sampled,
                       getDevice()->getSwapChain()->getSwapChainImageWidth(),
                       getDevice()->getSwapChain()->getSwapChainImageHeight(),
                       1, 1, 1, 1, "resolved shadow map"),
    mResolvedShadowMapView(mResolvedShadowMap, ImageViewType::Colour),

    mIntermediateShadowMap(eng->getDevice(), Format::RG32Float, ImageUsage::Sampled | ImageUsage::Storage,
                       getDevice()->getSwapChain()->getSwapChainImageWidth() * 2,
                       getDevice()->getSwapChain()->getSwapChainImageHeight() * 2,
                       1, 3, 1, 1, "cascade shadow map blure intermediate"),
    mIntermediateShadowMapViewMip0(mIntermediateShadowMap, ImageViewType::Colour, 0, 1, 0, 1),
    mIntermediateShadowMapViewMip1(mIntermediateShadowMap, ImageViewType::Colour, 0, 1, 1, 1),
    mIntermediateShadowMapViewMip2(mIntermediateShadowMap, ImageViewType::Colour, 0, 1, 2, 1),

    mBluredShadowMap(eng->getDevice(), Format::RG32Float, ImageUsage::Sampled | ImageUsage::Storage,
                       getDevice()->getSwapChain()->getSwapChainImageWidth() * 2,
                       getDevice()->getSwapChain()->getSwapChainImageHeight() * 2,
                       1, 3, 1, 1, "blured cascade shadow map"),
    mBluredShadowMapViewMip0(mBluredShadowMap, ImageViewType::Colour, 0, 1, 0, 1),
    mBluredShadowMapViewMip1(mBluredShadowMap, ImageViewType::Colour, 0, 1, 1, 1),
    mBluredShadowMapViewMip2(mBluredShadowMap, ImageViewType::Colour, 0, 1, 2, 1),

    mCascadesBuffer(eng->getDevice(), BufferUsage::Uniform, sizeof(Scene::ShadowCascades), sizeof(Scene::ShadowCascades), "Cascades"),
    mCascadesBufferView(mCascadesBuffer)
{
    // Render cascades
    {
           GraphicsPipelineDescription cascade0Desc(eng->getShader("./Shaders/ShadowMap.vert"),
              eng->getShader("./Shaders/VarianceShadowMap.frag"),
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() * 2,
                    getDevice()->getSwapChain()->getSwapChainImageHeight() * 2},
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() * 2,
              getDevice()->getSwapChain()->getSwapChainImageHeight() * 2},
            true, BlendMode::None, BlendMode::None, true, DepthTest::LessEqual, Primitive::TriangleList);

           GraphicsTask cascade0Task{"cascade0", cascade0Desc};
           cascade0Task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates);
           cascade0Task.addInput(kShadowingLights, AttachmentType::UniformBuffer);
           cascade0Task.addInput(kDefaultSampler, AttachmentType::Sampler);
           cascade0Task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
           cascade0Task.addInput("lightMatrix", AttachmentType::PushConstants);
           cascade0Task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
           cascade0Task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);
           cascade0Task.addOutput(kCascadeShadowMapRaw0, AttachmentType::RenderTarget2D, Format::RG32Float, SizeClass::Custom, LoadOp::Clear_Float_Max);
           cascade0Task.addOutput("ShadowMapDepth0", AttachmentType::Depth, Format::D32Float, SizeClass::DoubleSwapchain, LoadOp::Clear_White);
           mRenderCascade0 = graph.addTask(cascade0Task);

           GraphicsPipelineDescription cascade1Desc(eng->getShader("./Shaders/ShadowMap.vert"),
              eng->getShader("./Shaders/VarianceShadowMap.frag"),
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                    getDevice()->getSwapChain()->getSwapChainImageHeight()},
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
              getDevice()->getSwapChain()->getSwapChainImageHeight()},
            true, BlendMode::None, BlendMode::None, true, DepthTest::LessEqual, Primitive::TriangleList);

           GraphicsTask cascade1Task{"cascade1", cascade1Desc};
           cascade1Task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates);
           cascade1Task.addInput(kShadowingLights, AttachmentType::UniformBuffer);
           cascade1Task.addInput(kDefaultSampler, AttachmentType::Sampler);
           cascade1Task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
           cascade1Task.addInput("lightMatrix", AttachmentType::PushConstants);
           cascade1Task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
           cascade1Task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);
           cascade1Task.addOutput(kCascadeShadowMapRaw1, AttachmentType::RenderTarget2D, Format::RG32Float, SizeClass::Custom, LoadOp::Clear_Float_Max);
           cascade1Task.addOutput("ShadowMapDepth1", AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_White);
           mRenderCascade1 = graph.addTask(cascade1Task);

           GraphicsPipelineDescription cascade2Desc(eng->getShader("./Shaders/ShadowMap.vert"),
              eng->getShader("./Shaders/VarianceShadowMap.frag"),
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
                    getDevice()->getSwapChain()->getSwapChainImageHeight() / 2},
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
              getDevice()->getSwapChain()->getSwapChainImageHeight() / 2},
            true, BlendMode::None, BlendMode::None, true, DepthTest::LessEqual, Primitive::TriangleList);

           GraphicsTask cascade2Task{"cascade2", cascade2Desc};
           cascade2Task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates);
           cascade2Task.addInput(kShadowingLights, AttachmentType::UniformBuffer);
           cascade2Task.addInput(kDefaultSampler, AttachmentType::Sampler);
           cascade2Task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
           cascade2Task.addInput("lightMatrix", AttachmentType::PushConstants);
           cascade2Task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
           cascade2Task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);
           cascade2Task.addOutput(kCascadeShadowMapRaw2, AttachmentType::RenderTarget2D, Format::RG32Float, SizeClass::Custom, LoadOp::Clear_Float_Max);
           cascade2Task.addOutput("ShadowMapDepth2", AttachmentType::Depth, Format::D32Float, SizeClass::HalfSwapchain, LoadOp::Clear_White);
           mRenderCascade2 = graph.addTask(cascade2Task);

    }

    // Blur + resolve.
    {
        ComputeTask blurXTask0{ "ShadowMapBlurX0", mBlurXDesc };
        blurXTask0.addInput(kCascadeShadowMapRaw0, AttachmentType::Texture2D);
        blurXTask0.addInput(kCascadeShadowMapBlurIntermediate0, AttachmentType::Image2D);
        blurXTask0.addInput(kDefaultSampler, AttachmentType::Sampler);
        blurXTask0.setRecordCommandsCallback(
                    [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                    {
                        const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
                        const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

                        exec->dispatch(std::ceil(threadGroupWidth / 128.0f), threadGroupHeight * 2, 1.0f);
                    }
        );
        graph.addTask(blurXTask0);

        ComputeTask blurYTask0{ "ShadowMapBlurY0", mBlurYDesc };
        blurYTask0.addInput(kCascadeShadowMapBlurIntermediate0, AttachmentType::Texture2D);
        blurYTask0.addInput(kCascadeShadowMapBlured0, AttachmentType::Image2D);
        blurYTask0.addInput(kDefaultSampler, AttachmentType::Sampler);
        blurYTask0.setRecordCommandsCallback(
                    [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                    {
                        const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
                        const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

                        exec->dispatch(threadGroupWidth * 2, std::ceil(threadGroupHeight / 128.0f), 1.0f);
                    }
        );
        graph.addTask(blurYTask0);

        ComputeTask blurXTask1{ "ShadowMapBlurX1", mBlurXDesc };
        blurXTask1.addInput(kCascadeShadowMapRaw1, AttachmentType::Texture2D);
        blurXTask1.addInput(kCascadeShadowMapBlurIntermediate1, AttachmentType::Image2D);
        blurXTask1.addInput(kDefaultSampler, AttachmentType::Sampler);
        blurXTask1.setRecordCommandsCallback(
                    [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                    {
                        const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
                        const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

                        exec->dispatch(std::ceil(threadGroupWidth / 128.0f), threadGroupHeight, 1.0f);
                    }
        );
        graph.addTask(blurXTask1);

        ComputeTask blurYTask1{ "ShadowMapBlurY1", mBlurYDesc };
        blurYTask1.addInput(kCascadeShadowMapBlurIntermediate1, AttachmentType::Texture2D);
        blurYTask1.addInput(kCascadeShadowMapBlured1, AttachmentType::Image2D);
        blurYTask1.addInput(kDefaultSampler, AttachmentType::Sampler);
        blurYTask1.setRecordCommandsCallback(
                    [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                    {
                        const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
                        const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

                        exec->dispatch(threadGroupWidth, std::ceil(threadGroupHeight / 128.0f), 1.0f);
                    }
        );
        graph.addTask(blurYTask1);

        ComputeTask blurXTask2{ "ShadowMapBlurX2", mBlurXDesc };
        blurXTask2.addInput(kCascadeShadowMapRaw2, AttachmentType::Texture2D);
        blurXTask2.addInput(kCascadeShadowMapBlurIntermediate2, AttachmentType::Image2D);
        blurXTask2.addInput(kDefaultSampler, AttachmentType::Sampler);
        blurXTask2.setRecordCommandsCallback(
                    [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                    {
                        const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
                        const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

                        exec->dispatch(std::ceil(threadGroupWidth / 128.0f), threadGroupHeight / 2.0f, 1.0f);
                    }
        );
        graph.addTask(blurXTask2);

        ComputeTask blurYTask2{ "ShadowMapBlurY2", mBlurYDesc };
        blurYTask2.addInput(kCascadeShadowMapBlurIntermediate2, AttachmentType::Texture2D);
        blurYTask2.addInput(kCascadeShadowMapBlured2, AttachmentType::Image2D);
        blurYTask2.addInput(kDefaultSampler, AttachmentType::Sampler);
        blurYTask2.setRecordCommandsCallback(
                    [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                    {
                        const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
                        const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

                        exec->dispatch(threadGroupWidth / 2.0f, std::ceil(threadGroupHeight / 128.0f), 1.0f);
                    }
        );
        graph.addTask(blurYTask2);

        ComputeTask resolveTask{ "Resolve shadow mapping", mResolveDesc };
        resolveTask.addInput(kGBufferDepth, AttachmentType::Texture2D);
        resolveTask.addInput(kCascadeShadowMapBlured0, AttachmentType::Texture2D);
        resolveTask.addInput(kCascadeShadowMapBlured1, AttachmentType::Texture2D);
        resolveTask.addInput(kCascadeShadowMapBlured2, AttachmentType::Texture2D);
        resolveTask.addInput(kShadowMap, AttachmentType::Image2D);
        resolveTask.addInput(kShadowingLights, AttachmentType::UniformBuffer);
        resolveTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
        resolveTask.addInput(kDefaultSampler, AttachmentType::Sampler);
        resolveTask.addInput(kCascadesInfo, AttachmentType::UniformBuffer);
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
        graph.addTask(resolveTask);
    }
}


void CascadeShadowMappingTechnique::render(RenderGraph& graph, Engine* eng)
{
    (*mCascadeShadowMaps)->updateLastAccessed();
    (*mCascadeShaowMapsViewMip0)->updateLastAccessed();
    (*mCascadeShaowMapsViewMip1)->updateLastAccessed();
    (*mCascadeShaowMapsViewMip2)->updateLastAccessed();
    (*mResolvedShadowMap)->updateLastAccessed();
    (*mResolvedShadowMapView)->updateLastAccessed();
    (*mIntermediateShadowMap)->updateLastAccessed();
    (*mIntermediateShadowMapViewMip0)->updateLastAccessed();
    (*mIntermediateShadowMapViewMip1)->updateLastAccessed();
    (*mIntermediateShadowMapViewMip2)->updateLastAccessed();
    (*mBluredShadowMap)->updateLastAccessed();
    (*mBluredShadowMapViewMip0)->updateLastAccessed();
    (*mBluredShadowMapViewMip1)->updateLastAccessed();
    (*mBluredShadowMapViewMip2)->updateLastAccessed();
    (*mCascadesBuffer)->updateLastAccessed();

    const Scene::ShadowCascades& cascades = eng->getScene()->getShadowCascades();

    (*mCascadesBuffer)->setContents(&cascades, sizeof(Scene::ShadowCascades));

    const Frustum lightFrustum = eng->getScene()->getShadowingLightFrustum();
    std::vector<const MeshInstance*> meshes = eng->getScene()->getViewableMeshes(lightFrustum);

    std::sort(meshes.begin(), meshes.end(), [lightPosition = eng->getScene()->getShadowingLight().mPosition](const MeshInstance* lhs, const MeshInstance* rhs)
    {
        const float3 centralLeft = lhs->getTransMatrix() * lhs->mMesh->getAABB().getCentralPoint();
        const float leftDistance = glm::length(centralLeft - float3(lightPosition));

        const float3 centralright = rhs->getTransMatrix() * rhs->mMesh->getAABB().getCentralPoint();
        const float rightDistance = glm::length(centralright - float3(lightPosition));

        return leftDistance < rightDistance;
    });

    std::vector<const MeshInstance*> nearCascadeMeshes;
    std::copy_if(meshes.begin(), meshes.end(), std::back_inserter(nearCascadeMeshes), [cascades, cameraPosition = eng->getScene()->getCamera().getPosition()](const MeshInstance* inst)
    {
         const float3 central = (inst->mMesh->getAABB() * inst->getTransMatrix()).getCentralPoint();
         const float distance = glm::length(central - cameraPosition);

         return distance <= cascades.mNearEnd;
    });

    std::vector<const MeshInstance*> midCascadeMeshes;
    std::copy_if(meshes.begin(), meshes.end(), std::back_inserter(midCascadeMeshes), [cascades, cameraPosition = eng->getScene()->getCamera().getPosition()](const MeshInstance* inst)
    {
         const float3 central = (inst->mMesh->getAABB() * inst->getTransMatrix()).getCentralPoint();
         const float distance = glm::length(central - cameraPosition);

         return distance <= cascades.mMidEnd && distance >= cascades.mNearEnd;
    });

    std::vector<const MeshInstance*> farCascadeMeshes;
    std::copy_if(meshes.begin(), meshes.end(), std::back_inserter(farCascadeMeshes), [cascades, cameraPosition = eng->getScene()->getCamera().getPosition()](const MeshInstance* inst)
    {
         const float3 central = (inst->mMesh->getAABB() * inst->getTransMatrix()).getCentralPoint();
         const float distance = glm::length(central - cameraPosition);

         return distance <= cascades.mFarEnd && distance >= cascades.mMidEnd;
    });

    GraphicsTask& cascade0Task = static_cast<GraphicsTask&>(graph.getTask(mRenderCascade0));
    cascade0Task.setRecordCommandsCallback(
                [nearCascadeMeshes](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                    exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                    for (const auto& mesh : nearCascadeMeshes)
                    {
                        // Don't render transparent geometry.
                        if ((mesh->mMesh->getAttributes() & MeshAttributes::Transparent) > 0 || !(mesh->getInstanceFlags() & InstanceFlags::Draw))
                            continue;

                        const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

                        const MeshEntry entry = mesh->getMeshShaderEntry();

                        exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
                        exec->indexedDraw(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
                    }
                }
    );

    GraphicsTask& cascade1Task = static_cast<GraphicsTask&>(graph.getTask(mRenderCascade1));
    cascade1Task.setRecordCommandsCallback(
                [midCascadeMeshes](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                    exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                    for (const auto& mesh : midCascadeMeshes)
                    {
                        // Don't render transparent geometry.
                        if ((mesh->mMesh->getAttributes() & MeshAttributes::Transparent) > 0 || !(mesh->getInstanceFlags() & InstanceFlags::Draw))
                            continue;

                        const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

                        const MeshEntry entry = mesh->getMeshShaderEntry();

                        exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
                        exec->indexedDraw(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
                    }
                }
    );

    GraphicsTask& cascade2Task = static_cast<GraphicsTask&>(graph.getTask(mRenderCascade2));
    cascade2Task.setRecordCommandsCallback(
                [farCascadeMeshes](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                    exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                    for (const auto& mesh : farCascadeMeshes)
                    {
                        // Don't render transparent geometry.
                        if ((mesh->mMesh->getAttributes() & MeshAttributes::Transparent) > 0 || !(mesh->getInstanceFlags() & InstanceFlags::Draw))
                            continue;

                        const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

                        const MeshEntry entry = mesh->getMeshShaderEntry();

                        exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
                        exec->indexedDraw(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
                    }
                }
    );
}
