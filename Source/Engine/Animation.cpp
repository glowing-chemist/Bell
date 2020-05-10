#include "Animation.hpp"

#include "Core/BellLogging.hpp"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>


Animation::Animation(const aiAnimation* anim) :
    mName(anim->mName.C_Str()),
    mNumTicks(anim->mDuration),
    mTicksPerSec(anim->mTicksPerSecond),
    mCurrentTick(0),
    mBones()
{

    for(uint32_t i = 0; i < anim->mNumChannels; ++i)
    {
        const aiNodeAnim* nodeAnim = anim->mChannels[i];

        BELL_ASSERT(nodeAnim->mNumPositionKeys == nodeAnim->mNumRotationKeys &&
                    nodeAnim->mNumScalingKeys == nodeAnim->mNumPositionKeys, "Diffrent number of scale/rotation/translation currently not supported in animations")

        BoneTransform& bone = mBones[nodeAnim->mNodeName.C_Str()];

        bone.mTick.resize(nodeAnim->mNumPositionKeys);
        for(uint32_t j = 0; j < nodeAnim->mNumPositionKeys; ++j)
        {
            Tick& tick = bone.mTick[j];
            tick.mTick = nodeAnim->mPositionKeys[j].mTime;

            glm::quat rot;
            rot.w = nodeAnim->mRotationKeys[j].mValue.w;
            rot.x = nodeAnim->mRotationKeys[j].mValue.x;
            rot.y = nodeAnim->mRotationKeys[j].mValue.y;
            rot.z = nodeAnim->mRotationKeys[j].mValue.z;
            float4x4 transformation = glm::toMat4(rot);

            transformation = glm::scale(transformation, float3(nodeAnim->mScalingKeys[j].mValue.x,
                                                        nodeAnim->mScalingKeys[j].mValue.y,
                                                        nodeAnim->mScalingKeys[j].mValue.z));

            transformation = glm::translate(transformation, float3(nodeAnim->mPositionKeys[j].mValue.x,
                                                                   nodeAnim->mPositionKeys[j].mValue.y,
                                                                   nodeAnim->mPositionKeys[j].mValue.z));

            tick.mBoneTransform = transformation;
        }
    }
}
