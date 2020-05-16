#include "Engine/SkinningTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"


SkinningTechnique::SkinningTechnique(Engine* eng, RenderGraph& graph) :
    Technique("Skinning", eng->getDevice()),
    mPipelineDesc{eng->getShader("./Shaders/Skinning.comp")}
{
    ComputeTask task("Skinning", mPipelineDesc);
    task.addInput(kTPoseVertexBuffer, AttachmentType::DataBufferRO);
    task.addInput(kBoneWeighntsIndiciesBuffer, AttachmentType::DataBufferRO);
    task.addInput(kBonesWeights, AttachmentType::DataBufferRO);
    task.addInput(kBonesBuffer, AttachmentType::DataBufferRO);
    task.addInput(kSceneVertexBuffer, AttachmentType::DataBufferRW);
    task.addInput("meshParams", AttachmentType::PushConstants);
    task.setRecordCommandsCallback(
                [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    const auto& anims = eng->getActiveAnimations();
                    for(const auto& anim : anims)
                    {
                        MeshInstance* inst = eng->getScene()->getMeshInstance(anim.mMesh);
                        auto [vertexReadOffset, boneIndexOffset] = eng->addMeshToAnimationBuffer(inst->mMesh);
                        auto [vertexWriteOffset, indexOffset] = eng->addMeshToBuffer(inst->mMesh);

                        const uint32_t vertexCount = inst->mMesh->getVertexCount();
                        const uint32_t vertesStride = inst->mMesh->getVertexStride();
                        PushConstant pushConstants{};
                        pushConstants.mVertexCount = vertexCount;
                        pushConstants.mVertexReadIndex = vertexReadOffset / vertesStride;
                        pushConstants.mVertexWriteIndex = vertexWriteOffset / vertesStride;
                        pushConstants.mBoneIndex = anim.mBoneOffset;
                        pushConstants.mBoneIndiciesIndex = boneIndexOffset;
                        pushConstants.mVertexStride = vertesStride;

                        exec->insertPushConsatnt(&pushConstants, sizeof(PushConstant));
                        exec->dispatch(std::ceil(vertexCount / 32.0f), 1, 1);
                    }
                }
    );

    mTask = graph.addTask(task);
}
