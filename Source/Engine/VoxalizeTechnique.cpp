#include "Engine/VoxalizeTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/UberShaderStateCache.hpp"
#include "Core/Executor.hpp"


#define VOXEL_SIZE_X 25
#define VOXEL_SIZE_Y 18
#define VOXEL_SIZE_Z 50


VoxalizeTechnique::VoxalizeTechnique(RenderEngine* eng, RenderGraph& graph) :
    Technique("Voxalize", eng->getDevice()),
    mVoxelMap(eng->getDevice(),
              Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled | ImageUsage::TransferDest,
              VOXEL_SIZE_X, VOXEL_SIZE_Y, VOXEL_SIZE_Z,
              1, 1, 1, "Diffuse voxel map"),
    mVoxelMapView(mVoxelMap, ImageViewType::Colour),
    mVoxelDimmensions(eng->getDevice(), BufferUsage::Uniform, sizeof(ConstantBuffer), sizeof(ConstantBuffer), "Voxel Dimmensions"),
    mVoxelDimmensionsView(mVoxelDimmensions),
    mPipelineDesc{
        Rect{VOXEL_SIZE_X, VOXEL_SIZE_Y},
        Rect{VOXEL_SIZE_X, VOXEL_SIZE_Y},
        FaceWindingOrder::CW,
        BlendMode::None,
        BlendMode::None,
        false,
        DepthTest::None,
        FillMode::Fill,
        Primitive::TriangleList
        },
    mVolxalizeVertexShader(eng->getShader("./Shaders/Voxalize.vert")),
    mVolxalizeGeometryShader(eng->getShader("./Shaders/Voxalize.geom")),
    mVolxalizeFragmentShader(eng->getShader("./Shaders/Voxalize.frag"))
#if DEBUG_VOXEL_GENERATION
    ,mVoxelDebug(eng->getDevice(),
                 Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled,
                 eng->getSwapChainImage()->getExtent(0, 0).width,
                 eng->getSwapChainImage()->getExtent(0, 0).height, 1, 1, 1, 1, "Debug Voxels"),
    mVoxelDebugView(mVoxelDebug, ImageViewType::Colour),
    mDebugVoxelPipeline{eng->getShader("./Shaders/DebugVoxels.comp")}
#endif
{
    GraphicsTask task{"voxalize", mPipelineDesc};
    task.setVertexAttributes((VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::Tangents | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo));
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kVoxelDimmensions, AttachmentType::UniformBuffer);
    task.addInput(kDiffuseVoxelMap, AttachmentType::Image3D);
    task.addInput(kDFGLUT, AttachmentType::Texture2D);
    task.addInput(kLTCMat, AttachmentType::Texture2D);
    task.addInput(kLTCAmp, AttachmentType::Texture2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kConvolvedDiffuseSkyBox, AttachmentType::CubeMap);

    if(eng->isPassRegistered(PassType::LightFroxelation))
    {
        task.addInput(kSparseFroxels, AttachmentType::DataBufferRO);
        task.addInput(kLightIndicies, AttachmentType::DataBufferRO);
        task.addInput(kActiveFroxels, AttachmentType::Texture2D);
    }

    if(eng->isPassRegistered(PassType::Shadow) || eng->isPassRegistered(PassType::CascadingShadow))
        task.addInput(kShadowMap, AttachmentType::Texture2D);

    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput(kLightBuffer, AttachmentType::ShaderResourceSet);
    task.addInput("ModelInfo", AttachmentType::PushConstants);
    task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    mTaskID = graph.addTask(task);

#if DEBUG_VOXEL_GENERATION
    ComputeTask debugTask("Voxel debug", mDebugVoxelPipeline);
    debugTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    debugTask.addInput(kVoxelDimmensions, AttachmentType::UniformBuffer);
    debugTask.addInput(kGBufferDepth, AttachmentType::Texture2D);
    debugTask.addInput(kDiffuseVoxelMap, AttachmentType::Texture2D);
    debugTask.addInput(kDebugVoxels, AttachmentType::Image2D);
    debugTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    debugTask.setRecordCommandsCallback(
                [](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
                    const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

                    exec->dispatch(	static_cast<uint32_t>(std::ceil(threadGroupWidth / 32.0f)),
                                    static_cast<uint32_t>(std::ceil(threadGroupHeight / 32.0f)),
                                    1);
                }
    );

    graph.addTask(debugTask);
#endif
}


void VoxalizeTechnique::render(RenderGraph& graph, RenderEngine* eng)
{
    // Need to manually clear this as load/store ops don't work on uavs :(.
    mVoxelMap->clear(float4(0.0f, 0.0f, 0.0f, 0.0f));

    const Camera& camera = eng->getCurrentSceneCamera();
    const ImageExtent extent = eng->getSwapChainImage()->getExtent(0, 0);
    const float aspect = float(extent.width) / float(extent.height);

    const float worldSpaceHeight = 2.0f * camera.getFarPlane() * tanf(glm::radians(camera.getFOV()) * 0.5f);
    const float worldSpaceWidth = worldSpaceHeight * aspect;

    ConstantBuffer constantBuffer{};
    //constantBuffer.voxelCentreWS =  float4(camera.getPosition() - (camera.getUp() * 0.5f * worldSpaceHeight) - (camera.getRight() * 0.5f * worldSpaceWidth), 1.0f);
    constantBuffer.voxelCentreWS =  float4(camera.getPosition() + (camera.getDirection() * 0.5f * camera.getFarPlane()), 1.0f);
    constantBuffer.recipVoxelSize = float4(1.0f / (worldSpaceWidth / float(VOXEL_SIZE_X)), 1.0f / (worldSpaceHeight / float(VOXEL_SIZE_Y)), 1.0f / (camera.getFarPlane() / float(VOXEL_SIZE_Z)), 1.0f);
    constantBuffer.recipVolumeSize = float4(1.0f / float(VOXEL_SIZE_X), 1.0f / float(VOXEL_SIZE_Y), 1.0f / float(VOXEL_SIZE_Z), 1.0f);
    constantBuffer.voxelVolumeSize = float4(VOXEL_SIZE_X, VOXEL_SIZE_Y, VOXEL_SIZE_Z, 1.0f);

    (*mVoxelDimmensions)->setContents(&constantBuffer, sizeof(ConstantBuffer));

    GraphicsTask& task = static_cast<GraphicsTask&>(graph.getTask(mTaskID));

    task.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
        {
            exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
            exec->bindVertexBuffer(eng->getVertexBuffer(), 0);
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mVolxalizeVertexShader, &mVolxalizeGeometryShader, nullptr, nullptr, mVolxalizeFragmentShader);

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
}
