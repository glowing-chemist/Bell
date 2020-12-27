#include "Engine/VoxelTerrainTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

#include "Core/Executor.hpp"

#include <numeric>


VoxelTerrainTechnique::VoxelTerrainTechnique(Engine* eng, RenderGraph& graph) :
    Technique("Voxel terrain", eng->getDevice()),
    mGenerateTerrainMeshShader(eng->getShader("./Shaders/MarchingCubes.comp")),
    mInitialiseIndirectDrawShader(eng->getShader("./Shaders/InitialiseIndirectDraw.comp")),
    mTerrainVertexShader(eng->getShader("./Shaders/Terrain.vert")),
    mTerrainFragmentShaderDeferred(eng->getShader("./Shaders/TerrainDeferred.frag")),
    mVoxelGrid(getDevice(), Format::R8Norm, ImageUsage::Sampled | ImageUsage::Storage | ImageUsage::TransferDest,
               eng->getScene()->getVoxelTerrain()->getSize().x, eng->getScene()->getVoxelTerrain()->getSize().z, eng->getScene()->getVoxelTerrain()->getSize().y, 1, 1, 1, "Terrain Voxels"),
    mVoxelGridView(mVoxelGrid, ImageViewType::Colour),
    mVertexBuffer(getDevice(), BufferUsage::Vertex | BufferUsage::DataBuffer, 100 * 1024 * 1024, 100 * 1024 * 1024, "Terrain vertex buffer"),
    mVertexBufferView(mVertexBuffer),
    mIndirectArgsBuffer(getDevice(), BufferUsage::IndirectArgs | BufferUsage::DataBuffer, sizeof(uint32_t) * 4, sizeof(uint32_t) * 4, "Terrain indirect Args"),
    mIndirectArgView(mIndirectArgsBuffer),
    mTextureScale(5.0f, 5.0f),
    mMaterialIndex(0)
{
    // Upload terrain grid.
    const Scene* scene = eng->getScene();
    const std::unique_ptr<VoxelTerrain>& terrain = scene->getVoxelTerrain();
    const uint3 voxelGridSize = terrain->getSize();
    mVoxelGrid->setContents(terrain->getVoxelData().data(), voxelGridSize.x, voxelGridSize.y, voxelGridSize.z);

    {
        ComputeTask resetIndirectArgs{"Terrain reset args"};
        resetIndirectArgs.addInput(kTerrainIndirectBuffer, AttachmentType::DataBufferWO);
        resetIndirectArgs.setRecordCommandsCallback(
                [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
                {
                    const ComputeTask& task = static_cast<const ComputeTask&>(graph.getTask(taskIndex));
                    exec->setComputeShader(task, graph, mInitialiseIndirectDrawShader);

                    exec->dispatch(1, 1, 1);
                });
        graph.addTask(resetIndirectArgs);
    }

    {
        ComputeTask marchCube{"terrain march cubes"};
        marchCube.addInput(kTerrainVoxelGrid, AttachmentType::Texture3D);
        marchCube.addInput(kTerrainIndirectBuffer, AttachmentType::DataBufferRW);
        marchCube.addInput(kTerrainVertexBuffer, AttachmentType::DataBufferWO);
        marchCube.addInput(kDefaultSampler, AttachmentType::Sampler);
        marchCube.addInput(kTerrainUniformBuffer, AttachmentType::PushConstants);
        marchCube.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            const ComputeTask& task = static_cast<const ComputeTask&>(graph.getTask(taskIndex));
            exec->setComputeShader(task, graph, mGenerateTerrainMeshShader);

            const Scene* scene = eng->getScene();
            const Camera& cam = scene->getCamera();
            const std::unique_ptr<VoxelTerrain>& terrain = scene->getVoxelTerrain();
            const float voxelSize = terrain->getVoxelSize();
            const float3 voxelGridSize = float3(terrain->getSize()) * voxelSize;

            const float4x4 invView = glm::inverse(cam.getViewMatrix());
            const float farPlane = cam.getFarPlane();
            const float frustumHalfHeight = farPlane * tan(glm::radians(cam.getFOV()) * 0.5f);
            const float aspect = cam.getAspect();
            const float frustumHalfWidth = frustumHalfHeight * aspect;
            const float3 camUp = float3(0.f, frustumHalfHeight, 0.0f);
            const float3 camRight = float3(frustumHalfWidth, 0.0f, 0.0f);
            float3 viewSpaceFrustumPoints[8] = {camUp + camRight, -camUp + camRight
                                              , camUp - camRight, -camUp - camRight,
                                               camUp + camRight - float3(0.0f, 0.0f, farPlane), -camUp + camRight - float3(0.0f, 0.0f, farPlane)
                                              ,camUp - camRight - float3(0.0f, 0.0f, farPlane), -camUp - camRight - float3(0.0f, 0.0f, farPlane)};
            float3 terrainVolumeMin{INFINITY, INFINITY, INFINITY};
            float3 terrainVolumeMax{-INFINITY, -INFINITY, -INFINITY};
            for(uint32_t i = 0u; i < 8u; ++i)
            {
                terrainVolumeMin = componentWiseMin(terrainVolumeMin, invView * float4(viewSpaceFrustumPoints[i], 1.0f));
                terrainVolumeMax = componentWiseMax(terrainVolumeMax, invView * float4(viewSpaceFrustumPoints[i], 1.0f));
            }

            float3 gridMin = -(voxelGridSize / 2.0f);
            int3 offset = (terrainVolumeMin - gridMin) / voxelSize;
            offset = glm::max(offset, int3(0, 0, 0));
            float3 volumeMin = componentWiseMax(terrainVolumeMin, gridMin);

            TerrainVolume uniformBuffer{};
            uniformBuffer.minimum = float4(gridMin, 1.0f);
            uniformBuffer.offset = offset;
            uniformBuffer.voxelSize = voxelSize;
            exec->insertPushConsatnt(&uniformBuffer, sizeof(TerrainVolume));

            float3 volumeMax = terrainVolumeMax;
            volumeMax = componentWiseMin(volumeMax, -gridMin);

            const float3 volumeSize = volumeMax - volumeMin;
            exec->dispatch(std::ceil(volumeSize.x / (voxelSize * 8.0f)), std::ceil(volumeSize.y / (voxelSize * 8.0f)), std::ceil(volumeSize.z / (voxelSize * 8.0f)));
        });
        mSurfaceGenerationTask = graph.addTask(marchCube);
    }

    if(eng->isPassRegistered(PassType::GBufferPreDepth) || eng->isPassRegistered(PassType::GBuffer))
    {
        GraphicsPipelineDescription pipeline{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                                                  getDevice()->getSwapChain()->getSwapChainImageHeight()},
                                                  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                                                  getDevice()->getSwapChain()->getSwapChainImageHeight()},
                                                  FaceWindingOrder::CW, BlendMode::None, BlendMode::None, true, DepthTest::GreaterEqual, FillMode::Fill, Primitive::TriangleList};

        GraphicsTask renderTerrainTask{"Deferred Terrain", pipeline};
        renderTerrainTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals);
        renderTerrainTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
        renderTerrainTask.addInput(kDefaultSampler, AttachmentType::Sampler);
        renderTerrainTask.addInput(kMaterials, AttachmentType::ShaderResourceSet);
        renderTerrainTask.addInput(kTerrainVertexBuffer, AttachmentType::VertexBuffer);
        renderTerrainTask.addInput(kTerrainIndirectBuffer, AttachmentType::IndirectBuffer);
        renderTerrainTask.addInput("TextureInfo", AttachmentType::PushConstants);
        renderTerrainTask.addOutput(kGBufferDiffuse, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, LoadOp::Preserve);
        renderTerrainTask.addOutput(kGBufferNormals, AttachmentType::RenderTarget2D, Format::RG8UNorm, LoadOp::Preserve);
        renderTerrainTask.addOutput(kGBufferSpecularRoughness, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, LoadOp::Preserve);
        renderTerrainTask.addOutput(kGBufferVelocity, AttachmentType::RenderTarget2D, Format::RG16UNorm, LoadOp::Preserve);
        renderTerrainTask.addOutput(kGBufferEmissiveOcclusion, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, LoadOp::Preserve);
        renderTerrainTask.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, LoadOp::Preserve);
        renderTerrainTask.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
        {
            const GraphicsTask& task = static_cast<const GraphicsTask&>(graph.getTask(taskIndex));
            exec->setGraphicsShaders(task, graph, mTerrainVertexShader, nullptr, nullptr, nullptr, mTerrainFragmentShaderDeferred);
            exec->bindVertexBuffer(mVertexBufferView, 0);

            TerrainTexturing textureInfo{mTextureScale, mMaterialIndex};
            exec->insertPushConsatnt(&textureInfo, sizeof(TerrainTexturing));

            exec->indirectDraw(1, mIndirectArgView);
        });

        graph.addTask(renderTerrainTask);
    }
    else
    {
        // TODO add support for forward voxel terrain.
        BELL_TRAP;
    }
}


void VoxelTerrainTechnique::bindResources(RenderGraph& graph)
{
    if(!graph.isResourceSlotBound(kTerrainVertexBuffer))
    {
        graph.bindBuffer(kTerrainVertexBuffer, mVertexBufferView);
        graph.bindBuffer(kTerrainIndirectBuffer, mIndirectArgView);
        graph.bindImage(kTerrainVoxelGrid, mVoxelGridView);
    }
}
