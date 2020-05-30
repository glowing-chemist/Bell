#include "Engine/DebugVisualizationTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"

#include "glm/gtx/transform.hpp"


DebugAABBTechnique::DebugAABBTechnique(Engine* eng, RenderGraph& graph) :
    Technique("DebugAABB", eng->getDevice()),
    mTaskID{0},
    mPipelineDesc{eng->getShader("./Shaders/DebugAABB.vert"),
                  eng->getShader("./Shaders/DebugAABB.frag"),
                  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                        getDevice()->getSwapChain()->getSwapChainImageHeight()},
                  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                  getDevice()->getSwapChain()->getSwapChainImageHeight()},
                  false, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, Primitive::LineList},
    mVertexBuffer(getDevice(), BufferUsage::TransferDest | BufferUsage::Vertex, sizeof(float4) * 8, sizeof(float4) * 8),
    mVertexBufferView(mVertexBuffer),
    mIndexBuffer(getDevice(), BufferUsage::TransferDest | BufferUsage::Index, sizeof(uint32_t) * 24, sizeof(uint32_t) * 24),
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

    uint32_t indicies[24] = {0, 1,
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
                             7, 3};
    mIndexBuffer->setContents(&indicies, sizeof(uint32_t) * 24);

    GraphicsTask debugAABBTask("Debug AABB", mPipelineDesc);
    debugAABBTask.setVertexAttributes(VertexAttributes::Position4);
    debugAABBTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    debugAABBTask.addInput("ABBBMatrix", AttachmentType::PushConstants);
    debugAABBTask.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm);
    debugAABBTask.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float);
    debugAABBTask.setRecordCommandsCallback(
                [this](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
                {
                    exec->bindVertexBuffer(this->mVertexBufferView, 0);
                    exec->bindIndexBuffer(this->mIndexBufferView, 0);

                    for(const auto* mesh : meshes)
                    {
                        if(mesh->getInstanceFlags() & InstanceFlags::DrawAABB)
                        {
                            const AABB& meshAABB = mesh->mMesh->getAABB();
                            const float4x4 meshTransformation = mesh->getTransMatrix();

                            AABB transformedAABB = meshAABB * meshTransformation;
                            float4x4 AABBtransformation = glm::translate(float3(transformedAABB.getCentralPoint())) * glm::scale(transformedAABB.getSideLengths());

                            exec->insertPushConsatnt(&AABBtransformation, sizeof(float4x4));
                            exec->indexedDraw(0, 0, 24);
                        }
                    }

                    // Check all active animations and draw bone OBBs
                    const std::vector<Engine::AnimationEntry>& activeAnims = eng->getActiveAnimations();
                    for(const Engine::AnimationEntry& entry : activeAnims)
                    {
                        MeshInstance* inst = eng->getScene()->getMeshInstance(entry.mMesh);
                        if(inst->getInstanceFlags() & InstanceFlags::DrawAABB)
                        {
                            const std::vector<StaticMesh::Bone>& skeleton = inst->mMesh->getSkeleton();
                            Animation& activeAnim = inst->mMesh->getAnimation(entry.mName);
                            std::vector<float4x4> boneTransforms = activeAnim.calculateBoneMatracies(*inst->mMesh, entry.mTick);
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
}
