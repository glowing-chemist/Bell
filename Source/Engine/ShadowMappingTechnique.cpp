#include "Engine/ShadowMappingTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

static constexpr char kShadowMapRaw[] = "ShadowMapRaw";

ShadowMappingTechnique::ShadowMappingTechnique(Engine* eng) :
    Technique("ShadowMapping", eng->getDevice()),
    mDesc(eng->getShader("./Shaders/ShadowMap.vert"),
          eng->getShader("./Shaders/VarianceShadowMap.frag"),
          Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                getDevice()->getSwapChain()->getSwapChainImageHeight()},
          Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
          getDevice()->getSwapChain()->getSwapChainImageHeight()},
        true, BlendMode::None, BlendMode::None, true, DepthTest::LessEqual, Primitive::TriangleList),
    mTask("ShadowMapping", mDesc),
    mResolveDesc{eng->getShader("./Shaders/ResolveVarianceShadowMap.comp")},
    mResolveTask("Resolve shadow mapping", mResolveDesc)
{
    mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Material |
                              VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    mTask.addInput(kShadowingLights, AttachmentType::UniformBuffer);
    mTask.addInput("lightMatrix", AttachmentType::PushConstants);
    mTask.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    mTask.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    mTask.addOutput(kShadowMapRaw, AttachmentType::RenderTarget2D, Format::RG32Float, SizeClass::Swapchain, LoadOp::Clear_Black);
    mTask.addOutput("ShadowMapDepth", AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_White);

    mResolveTask.addInput(kGBufferDepth, AttachmentType::Texture2D);
    mResolveTask.addInput(kShadowMapRaw, AttachmentType::Texture2D);
    mResolveTask.addInput(kShadowMap, AttachmentType::Image2D);
    mResolveTask.addInput(kShadowingLights, AttachmentType::UniformBuffer);
    mResolveTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    mResolveTask.addInput(kDefaultSampler, AttachmentType::Sampler);
}


void ShadowMappingTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance*>&)
{
    mTask.clearCalls();
    mResolveTask.clearCalls();

    const Frustum lightFrustum = eng->getScene().getShadowingLightFrustum();
    const std::vector<const Scene::MeshInstance*> meshes = eng->getScene().getViewableMeshes(lightFrustum);

    for (const auto& mesh : meshes)
    {
        // Don't render transparent or alpha tested geometry.
        if ((mesh->mMesh->getAttributes() & (MeshAttributes::AlphaTested | MeshAttributes::Transparent)) > 0)
           continue;

        const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

        mTask.addPushConsatntValue(mesh->mTransformation);
        mTask.addIndexedDrawCall(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
    }

    const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
    const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;
    mResolveTask.addDispatch(static_cast<uint32_t>(std::ceil(threadGroupWidth / 32.0f)),
        static_cast<uint32_t>(std::ceil(threadGroupHeight / 32.0f)),
        1);

    graph.addTask(mTask);
    graph.addTask(mResolveTask);
}
