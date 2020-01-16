#include "Engine/ShadowMappingTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"


ShadowMappingTechnique::ShadowMappingTechnique(Engine* eng) :
    Technique("ShadowMapping", eng->getDevice()),
    mDesc(eng->getShader("./Shaders/ShadowMap.vert"),
          eng->getShader("./Shaders/VarianceShadowMap.frag"),
          Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                getDevice()->getSwapChain()->getSwapChainImageHeight()},
          Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
          getDevice()->getSwapChain()->getSwapChainImageHeight()},
        true, BlendMode::None, BlendMode::None, true, DepthTest::LessEqual, Primitive::TriangleList),
    mTask("ShadowMapping", mDesc)
{
    mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Material |
                              VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    mTask.addInput(kShadowingLights, AttachmentType::UniformBuffer);
    mTask.addInput("lightMatrix", AttachmentType::PushConstants);
    mTask.addOutput(kShadowMap, AttachmentType::RenderTarget2D, Format::RG32Float, SizeClass::Swapchain, LoadOp::Clear_Black);
    mTask.addOutput("ShadowMapDepth", AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_White);
}


void ShadowMappingTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance*>&)
{
    mTask.clearCalls();

    uint64_t minVertexOffset = ~0ul;

    const Frustum lightFrustum = eng->getScene().getShadowingLightFrustum();
    const std::vector<const Scene::MeshInstance*> meshes = eng->getScene().getViewableMeshes(lightFrustum);

    uint32_t vertexBufferOffset = 0;
    for (const auto& mesh : meshes)
    {
        const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);
        minVertexOffset = std::min(minVertexOffset, vertexOffset);

        mTask.addPushConsatntValue(mesh->mTransformation);
        mTask.addIndexedDrawCall(vertexBufferOffset, indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());

        vertexBufferOffset += mesh->mMesh->getVertexCount();
    }

    mTask.setVertexBufferOffset(minVertexOffset);

    graph.addTask(mTask);
}
