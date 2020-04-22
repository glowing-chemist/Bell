#include "Engine/VoxalizeTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"


VoxalizeTechnique::VoxalizeTechnique(Engine* eng, RenderGraph& graph) :
    Technique("Voxalize", eng->getDevice()),
    mVoxelMap(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled, 128, 128, 256, 5, 1, 1, "Diffuse voxel map"),
    mVoxelMapView(mVoxelMap, ImageViewType::Colour),
    mVoxelDimmensions(eng->getDevice(), BufferUsage::Uniform, sizeof(ConstantBuffer), sizeof(ConstantBuffer), "Voxel Dimmensions"),
    mVoxelDimmensionsView(mVoxelDimmensions),
    mPipelineDesc{
        eng->getShader("./Shaders/Voxalize.vert"),
        eng->getShader("./Shaders/Voxalize.frag"),
        {},
        eng->getShader("./Shaders/Voxalize.geom"),
        {},
        {},
        Rect{128, 128},
        Rect{128, 128},
        true,
        BlendMode::None,
        BlendMode::None,
        false,
        DepthTest::None,
        Primitive::TriangleList
        }
{
    GraphicsTask task{"voxalize", mPipelineDesc};
    task.setVertexAttributes((VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates));
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput("VoxelDimm", AttachmentType::UniformBuffer);
    task.addInput(kDiffuseVoxelMap, AttachmentType::Image3D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kDiffuseVoxelMap, AttachmentType::Texture2D);

    if(eng->isPassRegistered(PassType::LightFroxelation))
    {
        task.addInput(kSparseFroxels, AttachmentType::DataBufferRO);
        task.addInput(kLightIndicies, AttachmentType::DataBufferRO);
    }

    if(eng->isPassRegistered(PassType::Shadow))
        task.addInput(kShadowMap, AttachmentType::Texture2D);

    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput(kLightBuffer, AttachmentType::ShaderResourceSet);
    task.addInput("ModelInfo", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    mTaskID = graph.addTask(task);
}


void VoxalizeTechnique::render(RenderGraph& graph, Engine*)
{
    GraphicsTask& task = static_cast<GraphicsTask&>(graph.getTask(mTaskID));

    task.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
        {
            exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
            exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

            for (const auto& mesh : meshes)
            {
                if (mesh->mMesh->getAttributes() & MeshAttributes::Transparent)
                    continue;

                const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

                const MeshEntry entry = mesh->getMeshShaderEntry();

                exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
                exec->indexedDraw(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
            }
        }
    );
}
