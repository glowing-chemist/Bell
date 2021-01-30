#include "Engine/VoxelTerrainTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

#include "Engine/TextureUtil.hpp"
#include "Engine/GeomUtils.h"

#include "Core/Executor.hpp"

#include <numeric>

#include <glm/gtx/transform.hpp>


VoxelTerrainTechnique::VoxelTerrainTechnique(Engine* eng, RenderGraph& graph) :
    Technique("Voxel terrain", eng->getDevice()),
    mGenerateTerrainMeshShader(eng->getShader("./Shaders/MarchingCubes.comp")),
    mInitialiseIndirectDrawShader(eng->getShader("./Shaders/InitialiseIndirectDraw.comp")),
    mTerrainVertexShader(eng->getShader("./Shaders/Terrain.vert")),
    mTerrainFragmentShaderDeferred(eng->getShader("./Shaders/TerrainDeferred.frag")),
    mModifyTerrainShader(eng->getShader("./Shaders/ModifyTerrain.comp")),
    mVoxelGrid(getDevice(), Format::R8Norm, ImageUsage::Sampled | ImageUsage::Storage | ImageUsage::TransferDest,
               eng->getScene()->getVoxelTerrain()->getSize().x, eng->getScene()->getVoxelTerrain()->getSize().z, eng->getScene()->getVoxelTerrain()->getSize().y, 1, 1, 1, "Terrain Voxels"),
    mVoxelGridView(mVoxelGrid, ImageViewType::Colour),
    mVertexBuffer(getDevice(), BufferUsage::Vertex | BufferUsage::DataBuffer, 10 * 1024 * 1024, 10 * 1024 * 1024, "Terrain vertex buffer"),
    mVertexBufferView(mVertexBuffer),
    mIndirectArgsBuffer(getDevice(), BufferUsage::IndirectArgs | BufferUsage::DataBuffer, sizeof(uint32_t) * 4, sizeof(uint32_t) * 4, "Terrain indirect Args"),
    mIndirectArgView(mIndirectArgsBuffer),
    mModifySize(5.0f),
    mTextureScale(5.0f, 5.0f),
    mMaterialIndexXZ(0),
    mMaterialIndexY(0)
{
    // Upload terrain grid.
    {
        const Scene* scene = eng->getScene();
        const std::unique_ptr<VoxelTerrain>& terrain = scene->getVoxelTerrain();
        uint3 voxelGridSize = terrain->getSize();
        const std::vector<int8_t>& voxelData = terrain->getVoxelData();

        mVoxelGrid->setContents(voxelData.data(), voxelGridSize.x, voxelGridSize.y, voxelGridSize.z);
    }

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
        ComputeTask modifyTerrain("ModifyTerrain");
        modifyTerrain.addInput(kTerrainVoxelGrid, AttachmentType::Image3D);
        modifyTerrain.addInput(kPreviousLinearDepth, AttachmentType::Texture2D);
        modifyTerrain.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
        modifyTerrain.addInput(kDefaultSampler, AttachmentType::Sampler);
        modifyTerrain.addInput("ModifyConstant", AttachmentType::PushConstants);
        modifyTerrain.setRecordCommandsCallback(
                    [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    PROFILER_EVENT("Voxel terrain");
                    PROFILER_GPU_TASK(exec);
                    PROFILER_GPU_EVENT("voxel terrain");

                    if(eng->editTerrain())
                    {
                        const ComputeTask& task = static_cast<const ComputeTask&>(graph.getTask(taskIndex));
                        exec->setComputeShader(task, graph, mModifyTerrainShader);

                        ImGuiIO& io = ImGui::GetIO();

                        const Scene* scene = eng->getScene();
                        const std::unique_ptr<VoxelTerrain>& terrain = scene->getVoxelTerrain();
                        const float voxelSize = terrain->getVoxelSize();
                        const float3 voxelGridWorldSize = float3(terrain->getSize()) * voxelSize;

                        float mode = 0.0f;
                        if(io.MouseDown[0])
                            mode = 0.01f;
                        else if(io.MouseDown[1])
                            mode = -0.01f;

                        TerrainModifying constants{uint2{io.MousePos.x, io.MousePos.y}, mode, voxelSize, voxelGridWorldSize};
                        exec->insertPushConsatnt(&constants, sizeof(TerrainModifying));

                        const uint32_t modifySize = std::ceil(mModifySize / voxelSize);
                        exec->dispatch(modifySize, modifySize, modifySize);
                    }
                });

        graph.addTask(modifyTerrain);
    }

    {
        ComputeTask marchCube{"terrain march cubes"};
        marchCube.addInput(kTerrainVoxelGrid, AttachmentType::Texture3D);
        marchCube.addInput(kTerrainIndirectBuffer, AttachmentType::DataBufferRW);
        marchCube.addInput(kTerrainVertexBuffer, AttachmentType::DataBufferWO);
        marchCube.addInput(kDefaultSampler, AttachmentType::Sampler);
        marchCube.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
        marchCube.addInput(kTerrainUniformBuffer, AttachmentType::PushConstants);
        marchCube.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            const ComputeTask& task = static_cast<const ComputeTask&>(graph.getTask(taskIndex));
            exec->setComputeShader(task, graph, mGenerateTerrainMeshShader);

            const Scene* scene = eng->getScene();
            const Scene::ShadowCascades cascades = scene->getShadowCascades();
            const Camera& cam = scene->getCamera();
            const float4x4 view = cam.getViewMatrix();
            const float cameraFarPlane = cam.getFarPlane();
            const float2 cascadeRanges[3] = {{cam.getNearPlane(), cascades.mNearEnd * cameraFarPlane},
                                             {cascades.mNearEnd * cameraFarPlane, cascades.mMidEnd * cameraFarPlane},
                                             {cascades.mMidEnd * cameraFarPlane, cascades.mFarEnd * cameraFarPlane} };
            const std::unique_ptr<VoxelTerrain>& terrain = scene->getVoxelTerrain();

            // generate terrain at 3 LODs.
            for(uint32_t lod = 0u; lod < 3u; ++lod)
            {
                const float baseVoxelSize = terrain->getVoxelSize();
                const float lodFactor = std::pow(2.0f, float(lod));
                const float lodVoxelSize = baseVoxelSize * lodFactor;
                const float3 voxelGridWorldSize = float3(terrain->getSize()) * baseVoxelSize;

                const float4x4 invView = glm::inverse(cam.getViewMatrix());
                const float nearPlane = cascadeRanges[lod].x;
                const float farPlane = cascadeRanges[lod].y;
                const float frustumHalfHeightFar = farPlane * tanf(glm::radians(cam.getFOV()) * 0.5f);
                const float frustumHalfHeightNear = nearPlane * tanf(glm::radians(cam.getFOV()) * 0.5f);
                const float aspect = cam.getAspect();
                const float frustumHalfWidthFar = frustumHalfHeightFar * aspect;
                const float frustumHalfWidthNear = frustumHalfHeightNear * aspect;
                const float3 camUpFar = float3(0.f, frustumHalfHeightFar, 0.0f);
                const float3 camRightFar = float3(frustumHalfWidthFar, 0.0f, 0.0f);
                const float3 camUpNear = float3(0.f, frustumHalfHeightNear, 0.0f);
                const float3 camRightNear = float3(frustumHalfWidthNear, 0.0f, 0.0f);
                float3 viewSpaceFrustumPoints[8] = {camUpNear + camRightNear - float3(0.0f, 0.0f, nearPlane), -camUpNear + camRightNear - float3(0.0f, 0.0f, nearPlane),
                                                    camUpNear - camRightNear - float3(0.0f, 0.0f, nearPlane), -camUpNear - camRightNear - float3(0.0f, 0.0f, nearPlane),
                                                    camUpFar + camRightFar - float3(0.0f, 0.0f, farPlane), -camUpFar + camRightFar - float3(0.0f, 0.0f, farPlane),
                                                    camUpFar - camRightFar - float3(0.0f, 0.0f, farPlane), -camUpFar - camRightFar - float3(0.0f, 0.0f, farPlane)};
                float3 terrainVolumeMin{INFINITY, INFINITY, INFINITY};
                float3 terrainVolumeMax{-INFINITY, -INFINITY, -INFINITY};
                for(uint32_t i = 0u; i < 8u; ++i)
                {
                    const float4 transformedPoint = invView * float4(viewSpaceFrustumPoints[i], 1.0f);
                    terrainVolumeMin = componentWiseMin(terrainVolumeMin, float3(transformedPoint.x, transformedPoint.y, transformedPoint.z));
                    terrainVolumeMax = componentWiseMax(terrainVolumeMax, float3(transformedPoint.x, transformedPoint.y, transformedPoint.z));
                }
                float3 gridMin = -(voxelGridWorldSize / 2.0f);
                float3 gridMax = -gridMin;
                float3 volumeMin = componentWiseMax(terrainVolumeMin, gridMin);
                volumeMin = componentWiseMin(terrainVolumeMin, gridMax);
                int3 offset = (terrainVolumeMin - gridMin) / lodVoxelSize;

                const float4x4 proj = glm::perspective(glm::radians(cam.getFOV()), cam.getAspect(), farPlane, nearPlane);
                const std::array<float4, 6> planes = frustumPlanes(proj * view);

                TerrainVolume uniformBuffer{};
                uniformBuffer.minimum = gridMin;
                uniformBuffer.offset = offset;
                uniformBuffer.voxelSize = baseVoxelSize;
                uniformBuffer.lod = lodFactor;
                memcpy(&uniformBuffer.frustumPlanes, planes.data(), sizeof(float4) * 6);
                exec->insertPushConsatnt(&uniformBuffer, sizeof(TerrainVolume));

                float3 volumeMax = terrainVolumeMax;
                volumeMax = componentWiseMin(volumeMax, gridMax);
                volumeMax = componentWiseMax(volumeMax, gridMin);

                const float3 volumeSize = volumeMax - volumeMin;
                exec->dispatch(std::ceil(volumeSize.x / (lodVoxelSize * 4.0f)), std::ceil(volumeSize.y / (lodVoxelSize * 4.0f)), std::ceil(volumeSize.z / (lodVoxelSize * 4.0f)));
            }

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

            TerrainTexturing textureInfo{mTextureScale, mMaterialIndexXZ, mMaterialIndexY};
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
