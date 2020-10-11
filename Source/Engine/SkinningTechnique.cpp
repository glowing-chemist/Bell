#include "Engine/SkinningTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"

constexpr const char kBlendShapesScratchBuffer[] = "BlendShapes";
constexpr uint32_t kScratchBufferSize = 5u * 1024u * 1024u;


SkinningTechnique::SkinningTechnique(Engine* eng, RenderGraph& graph) :
    Technique("Skinning", eng->getDevice()),
    mSkinningShader(eng->getShader("./Shaders/Skinning.comp")),
    mSkinningBlendShapesShader(eng->getShader("./Shaders/SkinnedBlendShapes.comp")),
    mBlendShapeScratchBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, kScratchBufferSize, kScratchBufferSize, "BlendShape Scratch buffer"),
    mBlendShapeScratchBufferView(mBlendShapeScratchBuffer)
{
    ComputeTask skinningTask("Skinning");
    skinningTask.addInput(kTPoseVertexBuffer, AttachmentType::DataBufferRO);
    skinningTask.addInput(kBoneWeighntsIndiciesBuffer, AttachmentType::DataBufferRO);
    skinningTask.addInput(kBonesWeights, AttachmentType::DataBufferRO);
    skinningTask.addInput(kBonesBuffer, AttachmentType::DataBufferRO);
    skinningTask.addInput(kSceneVertexBuffer, AttachmentType::DataBufferRW);
    skinningTask.addInput(kBlendShapesScratchBuffer, AttachmentType::DataBufferRO);
    skinningTask.addInput("meshParams", AttachmentType::PushConstants);
    skinningTask.setRecordCommandsCallback(
                [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {

                    std::unordered_map<std::string, uint32_t> boneOffsets{};

                    {
                        const RenderTask& task = graph.getTask(taskIndex);
                        exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mSkinningShader);

                        const auto& anims = eng->getActiveSkeletalAnimations();
                        for (const auto& anim : anims)
                        {
                            MeshInstance* inst = eng->getScene()->getMeshInstance(anim.mMesh);

                            // Will handle in combined shader.
                            if (inst->mMesh->isBlendMeshAnimation(anim.mName))
                            {
                                boneOffsets.insert({anim.mName, anim.mBoneOffset});
                                continue;
                            }
                            auto [vertexReadOffset, boneIndexOffset] = eng->addMeshToAnimationBuffer(inst->mMesh);
                            auto [vertexWriteOffset, indexOffset] = eng->addMeshToBuffer(inst->mMesh);

                            const uint32_t vertexCount = inst->mMesh->getVertexCount();
                            const uint32_t vertesStride = inst->mMesh->getVertexStride();
                            PushConstant pushConstants{};
                            pushConstants.mVertexCount = vertexCount;
                            pushConstants.mVertexReadIndex = vertexReadOffset / vertesStride;
                            pushConstants.mVertexWriteIndex = vertexWriteOffset / vertesStride;
                            pushConstants.mBoneIndex = anim.mBoneOffset;
                            pushConstants.mVertexStride = vertesStride;

                            exec->insertPushConsatnt(&pushConstants, sizeof(PushConstant));
                            exec->dispatch(std::ceil(vertexCount / 32.0f), 1, 1);
                        }
                    }

                    {
                        std::vector<PushConstant> constants{};
                        uint32_t scratchOffset = 0;
                        std::vector<unsigned char> vertexPatch{};
                        const auto& anims = eng->getActiveBlendShapesAnimations();
                        for (const auto& anim : anims)
                        {
                            MeshInstance* inst = eng->getScene()->getMeshInstance(anim.mMesh);
                            const BlendMeshAnimation& animation = inst->mMesh->getBlendMeshAnimation(anim.mName);
                            const std::vector<unsigned char> newVerticies = animation.getBlendedVerticies(*inst->mMesh, anim.mTick);
                            vertexPatch.insert(vertexPatch.end(), newVerticies.begin(), newVerticies.end());

                            auto [vertexReadOffset, boneIndexOffset] = eng->addMeshToAnimationBuffer(inst->mMesh);
                            auto [vertexWriteOffset, indexOffset] = eng->addMeshToBuffer(inst->mMesh);

                            const uint32_t vertexCount = inst->mMesh->getVertexCount();
                            const uint32_t vertesStride = inst->mMesh->getVertexStride();
                            PushConstant pushConstants{};
                            pushConstants.mVertexCount = vertexCount;
                            pushConstants.mVertexReadIndex = vertexReadOffset / vertesStride;
                            pushConstants.mVertexWriteIndex = vertexWriteOffset / vertesStride;
                            pushConstants.mBlendShapeReadIndex = scratchOffset / vertesStride;
                            BELL_ASSERT(boneOffsets.find(anim.mName) != boneOffsets.end(), "Blendshape animation doesn't have associated skeletal animation")
                            pushConstants.mBoneIndex = boneOffsets[anim.mName];
                            pushConstants.mVertexStride = vertesStride;

                            scratchOffset += newVerticies.size();

                            constants.push_back(pushConstants);
                        }

                        if (!vertexPatch.empty())
                        {
                            BELL_ASSERT(vertexPatch.size() < kScratchBufferSize, "Increase scratch buffer size")
                            exec->copyDataToBuffer(vertexPatch.data(), vertexPatch.size(), 0, mBlendShapeScratchBuffer);

                            const RenderTask& task = graph.getTask(taskIndex);
                            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mSkinningBlendShapesShader);

                            for (const auto& constants : constants)
                            {
                                exec->insertPushConsatnt(&constants, sizeof(PushConstant));
                                exec->dispatch(std::ceil(constants.mVertexCount / 32.0f), 1, 1);
                            }
                        }
                    }
                }
    );

    mTask = graph.addTask(skinningTask);
}


void SkinningTechnique::bindResources(RenderGraph& graph)
{
    if(!graph.isResourceSlotBound(kBlendShapesScratchBuffer))
        graph.bindBuffer(kBlendShapesScratchBuffer, mBlendShapeScratchBufferView);
}