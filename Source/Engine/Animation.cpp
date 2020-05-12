#include "Engine/Animation.hpp"
#include "Engine/StaticMesh.h"

#include "Core/BellLogging.hpp"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace
{
    float4x4 aiMatrix4x4ToFloat4x4(const aiMatrix4x4& transformation)
    {
        float4x4 transformationMatrix{};
        transformationMatrix[0][0] = transformation.a1; transformationMatrix[0][1] = transformation.b1;  transformationMatrix[0][2] = transformation.c1; transformationMatrix[0][3] = transformation.d1;
        transformationMatrix[1][0] = transformation.a2; transformationMatrix[1][1] = transformation.b2;  transformationMatrix[1][2] = transformation.c2; transformationMatrix[1][3] = transformation.d2;
        transformationMatrix[2][0] = transformation.a3; transformationMatrix[2][1] = transformation.b3;  transformationMatrix[2][2] = transformation.c3; transformationMatrix[2][3] = transformation.d3;
        transformationMatrix[3][0] = transformation.a4; transformationMatrix[3][1] = transformation.b4;  transformationMatrix[3][2] = transformation.c4; transformationMatrix[3][3] = transformation.d4;

        return transformationMatrix;
    }
}


Animation::Animation(const StaticMesh& mesh, const aiAnimation* anim, const aiScene* scene) :
    mName(anim->mName.C_Str()),
    mNumTicks(anim->mDuration),
    mTicksPerSec(anim->mTicksPerSecond),
    mInverseGlobalTransform(glm::inverse(aiMatrix4x4ToFloat4x4(scene->mRootNode->mTransformation))),
    mBones()
{
    const std::vector<StaticMesh::Bone>& bones = mesh.getSkeleton();
    for(const auto& bone : bones)
    {
        mBones[bone.mName]; // Just create an entry for every bone.
    }

    for(uint32_t i = 0; i < anim->mChannels[0]->mNumPositionKeys; ++i)
    {
        float4x4 identity(1.0f);
        readNodeHierarchy(anim, i, scene->mRootNode, identity);
    }
}


std::vector<float4x4> Animation::calculateBoneMatracies(const StaticMesh& mesh, const double tick)
{
    const auto& bones = mesh.getSkeleton();
    std::vector<float4x4> boneTransforms{};
    boneTransforms.reserve(bones.size());
    for(const auto& bone : bones)
    {
        BELL_ASSERT(mBones.find(bone.mName) != mBones.end(), "Bone not found")
        BoneTransform& transform = mBones[bone.mName];

        uint32_t i = 1;
        while(tick >= transform.mTick[i].mTick)
        {
            ++i;
        }

        const float4x4 boneTransform = interpolateTick(transform.mTick[i - 1], transform.mTick[i], tick);
        boneTransforms.push_back(boneTransform * bone.mInverseBindPose);
    }

    return boneTransforms;
}


float4x4 Animation::interpolateTick(const Tick& lhs, const Tick& rhs, const double tick) const
{
    const float lerpFactor = float(tick - lhs.mTick) / float(rhs.mTick - lhs.mTick);

    // Decompose matracties.
    float3 skew;
    float4 perspective; // These 2 shouldn't be needed.

    float3 lhsScale;
    glm::quat lhsRotation;
    float3 lhsTranslation;
    glm::decompose(lhs.mBoneTransform, lhsScale, lhsRotation, lhsTranslation, skew, perspective);

    float3 rhsScale;
    glm::quat rhsRotation;
    float3 rhsTranslation;
    glm::decompose(rhs.mBoneTransform, rhsScale, rhsRotation, rhsTranslation, skew, perspective);


    const float3 scale = glm::lerp(lhsScale, rhsScale, lerpFactor);
    const glm::quat rotation = glm::slerp(lhsRotation, rhsRotation, lerpFactor);
    const float3 translation = glm::lerp(lhsTranslation, rhsTranslation, lerpFactor);

    const float4x4 scaleMat = glm::scale(float4x4(1.0f), scale);
    const float4x4 rotMat = glm::toMat4(rotation);
    const float4x4 transMat = glm::translate(float4x4(1.0f), translation);

    return transMat * rotMat * scaleMat;
}


void Animation::readNodeHierarchy(const aiAnimation* anim, const uint32_t keyFrameIndex, const aiNode* pNode, const float4x4& ParentTransform)
{
    std::string NodeName(pNode->mName.data);

    float4x4 NodeTransformation(aiMatrix4x4ToFloat4x4(pNode->mTransformation));

    const aiNodeAnim* nodeAnim = findNodeAnim(anim, NodeName);

    Tick boneTransform{};

    if (nodeAnim)
    {
        boneTransform.mTick = nodeAnim->mPositionKeys[keyFrameIndex].mTime;

        glm::quat rot;
        rot.w = nodeAnim->mRotationKeys[keyFrameIndex].mValue.w;
        rot.x = nodeAnim->mRotationKeys[keyFrameIndex].mValue.x;
        rot.y = nodeAnim->mRotationKeys[keyFrameIndex].mValue.y;
        rot.z = nodeAnim->mRotationKeys[keyFrameIndex].mValue.z;
        float4x4 rotation = glm::toMat4(rot);

        float4x4 scale = glm::scale(float4x4(1.0f), float3(nodeAnim->mScalingKeys[keyFrameIndex].mValue.x,
                                                    nodeAnim->mScalingKeys[keyFrameIndex].mValue.y,
                                                    nodeAnim->mScalingKeys[keyFrameIndex].mValue.z));

        float4x4 translation = glm::translate(float4x4(1.0f), float3(nodeAnim->mPositionKeys[keyFrameIndex].mValue.x,
                                                               nodeAnim->mPositionKeys[keyFrameIndex].mValue.y,
                                                               nodeAnim->mPositionKeys[keyFrameIndex].mValue.z));

        NodeTransformation = translation * rotation * scale;
    }

    float4x4 GlobalTransformation = ParentTransform * NodeTransformation;

    if (mBones.find(NodeName) != mBones.end())
    {
       boneTransform.mBoneTransform = mInverseGlobalTransform * GlobalTransformation;
       mBones[NodeName].mTick.push_back(boneTransform);
    }

    for (uint32_t i = 0; i < pNode->mNumChildren; i++)
    {
       readNodeHierarchy(anim, keyFrameIndex, pNode->mChildren[i], GlobalTransformation);
    }
}


const aiNodeAnim* Animation::findNodeAnim(const aiAnimation* animation, const std::string& nodeName)
{
    for (uint32_t i = 0; i < animation->mNumChannels; i++)
    {
        const aiNodeAnim* nodeAnim = animation->mChannels[i];
        if (std::string(nodeAnim->mNodeName.data) == nodeName)
        {
            return nodeAnim;
        }
    }
    return nullptr;
}
