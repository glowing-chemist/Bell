#include "Engine/DebugVisualizationTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"

#include "glm/gtx/transform.hpp"


DebugAABBTechnique::DebugAABBTechnique(Engine* eng, RenderGraph& graph) :
    Technique("DebugAABB", eng->getDevice()),
    mAABBPipelineDesc{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                        getDevice()->getSwapChain()->getSwapChainImageHeight()},
                  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                  getDevice()->getSwapChain()->getSwapChainImageHeight()},
                  FaceWindingOrder::None, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, FillMode::Fill, Primitive::LineList},
    mAABBDebugVisVertexShader(eng->getShader("./Shaders/DebugAABB.vert")),
    mAABBDebugVisFragmentShader(eng->getShader("./Shaders/DebugAABB.frag")),
    mWireFramePipelineDesc{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                        getDevice()->getSwapChain()->getSwapChainImageHeight()},
                  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                  getDevice()->getSwapChain()->getSwapChainImageHeight()},
                  FaceWindingOrder::None, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, FillMode::Line, Primitive::TriangleList},
    mSimpleTransformShader(eng->getShader("./Shaders/BasicVertexTransform.vert")),
    mDebugLightsPipeline{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                        getDevice()->getSwapChain()->getSwapChainImageHeight()},
                  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                  getDevice()->getSwapChain()->getSwapChainImageHeight()},
                  FaceWindingOrder::None, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, FillMode::Fill, Primitive::TriangleList},
    mLightDebugFragmentShader(eng->getShader("./Shaders/LightDebug.frag")),
    mVertexBuffer(getDevice(), BufferUsage::TransferDest | BufferUsage::Vertex, sizeof(float4) * 8, sizeof(float4) * 8),
    mVertexBufferView(mVertexBuffer),
    mIndexBuffer(getDevice(), BufferUsage::TransferDest | BufferUsage::Index, sizeof(uint32_t) * 60, sizeof(uint32_t) * 60),
    mIndexBufferView(mIndexBuffer)
{
    float4 verticies[8] = {float4(-0.5f, 0.5f, -0.5f, 1.0f),
                           float4(-0.5f, 0.5f, 0.5f, 1.0f),
                           float4(0.5f, 0.5f, 0.5f, 1.0f),
                           float4(0.5f, 0.5f, -0.5f, 1.0f),
                           float4(-0.5f, -0.5f, -0.5f, 1.0f),
                           float4(-0.5f, -0.5f, 0.5f, 1.0f),
                           float4(0.5f, -0.5f, 0.5f, 1.0f),
                           float4(0.5f, -0.5f, -0.5f, 1.0f)};
    mVertexBuffer->setContents(&verticies, sizeof(float4) * 8);

    uint32_t indicies[60] = {0, 1, // Cube as lines.
                             1, 2,
                             2, 3,
                             3, 0,
                             4, 5,
                             5, 6,
                             6, 7,
                             7, 4,
                             4, 0,
                             5, 1,
                             6, 2,
                             7, 3,

                             0,2,1, // Cube as tris.
                             0,3,2,

                             1,2,6,
                             6,5,1,

                             4,5,6,
                             6,7,4,

                             2,3,6,
                             6,3,7,

                             0,7,3,
                             0,4,7,

                             0,1,5,
                             0,5,4
                            };
    mIndexBuffer->setContents(&indicies, sizeof(uint32_t) * 60);

    GraphicsTask debugAABBTask("Debug AABB", mAABBPipelineDesc);
    debugAABBTask.setVertexAttributes(VertexAttributes::Position4);
    debugAABBTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    debugAABBTask.addInput("ABBBMatrix", AttachmentType::PushConstants);
    debugAABBTask.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm);
    debugAABBTask.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float);
    debugAABBTask.setRecordCommandsCallback(
                [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
                {
                    exec->bindVertexBuffer(this->mVertexBufferView, 0);
                    exec->bindIndexBuffer(this->mIndexBufferView, 0);
                    const RenderTask& task = graph.getTask(taskIndex);
                    exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mAABBDebugVisVertexShader, nullptr, nullptr, nullptr, mAABBDebugVisFragmentShader);

                    for(const auto* mesh : meshes)
                    {
                        if(mesh->getInstanceFlags() & InstanceFlags::DrawAABB)
                        {
                            const AABB& meshAABB = mesh->getMesh()->getAABB();
                            const float4x4 meshTransformation = mesh->getTransMatrix();

                            AABB transformedAABB = meshAABB * meshTransformation;
                            float4x4 AABBtransformation = glm::translate(float3(transformedAABB.getCentralPoint())) * glm::scale(transformedAABB.getSideLengths());

                            exec->insertPushConsatnt(&AABBtransformation, sizeof(float4x4));
                            exec->indexedDraw(0, 0, 24);
                        }
                    }

                    // Check all active animations and draw bone OBBs
                    const std::vector<Engine::SkeletalAnimationEntry>& activeAnims = eng->getActiveSkeletalAnimations();
                    for(const Engine::SkeletalAnimationEntry& entry : activeAnims)
                    {
                        MeshInstance* inst = eng->getScene()->getMeshInstance(entry.mMesh);
                        if(inst->getInstanceFlags() & InstanceFlags::DrawAABB)
                        {
                            const std::vector<StaticMesh::Bone>& skeleton = inst->getMesh()->getSkeleton();
                            SkeletalAnimation& activeAnim = inst->getMesh()->getSkeletalAnimation(entry.mName);
                            std::vector<float4x4> boneTransforms = activeAnim.calculateBoneMatracies(*inst->getMesh(), entry.mTick);
                            BELL_ASSERT(boneTransforms.size() == skeleton.size(), "Bone size mismatch")

                            for(uint32_t i = 0; i < skeleton.size(); ++i)
                            {
                                const float4x4& boneTransform = boneTransforms[i];
                                const StaticMesh::Bone& bone = skeleton[i];

                                const float3 sideLenghts = bone.mOBB.getSideLengths();
                                const float3 centralPoint = bone.mOBB.getCentralPoint();

                                float4x4 OBBTransformation = inst->getTransMatrix() * boneTransform * glm::translate(centralPoint) * glm::scale(float4x4(1.0f), sideLenghts);
                                exec->insertPushConsatnt(&OBBTransformation, sizeof(float4x4));
                                exec->indexedDraw(0, 0, 24);
                            }
                        }
                    }
                }

    );

    mTaskID = graph.addTask(debugAABBTask);

    GraphicsTask wireFrameTask("WireFrame", mWireFramePipelineDesc);
    wireFrameTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::Tangents | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);
    wireFrameTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    wireFrameTask.addInput("Wireframe transforms", AttachmentType::PushConstants);
    wireFrameTask.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm);
    wireFrameTask.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float);
    wireFrameTask.setRecordCommandsCallback([this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
    {
        exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
        exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

        const RenderTask& task = graph.getTask(taskIndex);
        exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mAABBDebugVisVertexShader, nullptr, nullptr, nullptr, mAABBDebugVisFragmentShader);

        for(const auto* mesh : meshes)
        {
            if(mesh->getInstanceFlags() & InstanceFlags::DrawWireFrame && mesh->getInstanceFlags() & InstanceFlags::Draw)
            {
                const float4x4& transform = mesh->getTransMatrix();
                const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->getMesh());

                exec->insertPushConsatnt(&transform, sizeof(float4x4));
                exec->indexedDraw(vertexOffset / mesh->getMesh()->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->getMesh()->getIndexData().size());
            }
        }
    });

    graph.addTask(wireFrameTask);

    GraphicsTask lightDebug{"Light debug", mDebugLightsPipeline};
    lightDebug.setVertexAttributes(VertexAttributes::Position4);
    lightDebug.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    lightDebug.addInput(kLightBuffer, AttachmentType::ShaderResourceSet);
    lightDebug.addInput("Light transforms", AttachmentType::PushConstants);
    lightDebug.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm);
    lightDebug.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float);
    lightDebug.setRecordCommandsCallback([this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
    {
        struct LightTransform
        {
            float4x4 trans;
            uint32_t index;
        };

        exec->bindVertexBuffer(this->mVertexBufferView, 0);
        exec->bindIndexBuffer(this->mIndexBufferView, 0);
        const RenderTask& task = graph.getTask(taskIndex);
        exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mAABBDebugVisVertexShader, nullptr, nullptr, nullptr, mLightDebugFragmentShader);

        const std::vector<Scene::Light>& lights = eng->getScene()->getLights();
        for(uint32_t i = 0; i < lights.size(); ++i)
        {
            const Scene::Light& light = lights[i];

            if(light.mType == LightType::Area)
            {
                float4x4 lightTransform = float4x4(1.0f);
                lightTransform[0] = float4(glm::cross(light.mUp, light.mDirection), 0.0f);
                lightTransform[1] = float4{light.mUp, 0.0f};
                lightTransform[2] = float4{light.mDirection, 0.0f};
                lightTransform[3] = float4{light.mPosition, 1.0f};
                lightTransform = glm::scale(lightTransform, float3(light.mAngleSize.x, light.mAngleSize.y, 0.1f));

                LightTransform pushPonstants{};
                pushPonstants.trans = lightTransform;
                pushPonstants.index = i;

                exec->insertPushConsatnt(&pushPonstants, sizeof(LightTransform));
                exec->indexedDraw(0, 24, 36);
            }
            else if(light.mType == LightType::Strip)
            {
                float4x4 lightTransform = float4x4(1.0f);
                lightTransform[0] = float4(glm::cross(light.mUp, light.mDirection), 0.0f);
                lightTransform[1] = float4{light.mUp, 0.0f};
                lightTransform[2] = float4{light.mDirection, 0.0f};
                lightTransform[3] = float4{light.mPosition, 1.0f};
                lightTransform = glm::scale(lightTransform, float3(light.mAngleSize.y, light.mAngleSize.x, light.mAngleSize.x));

                LightTransform pushPonstants{};
                pushPonstants.trans = lightTransform;
                pushPonstants.index = i;

                exec->insertPushConsatnt(&pushPonstants, sizeof(LightTransform));
                exec->indexedDraw(0, 24, 36);
            }
        }
    });

    graph.addTask(lightDebug);
}
