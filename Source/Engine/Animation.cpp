#include "Engine/Animation.hpp"
#include "Engine/StaticMesh.h"

#include "Core/BellLogging.hpp"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <set>

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
        readNodeHierarchy(anim, aiString(bone.mName.c_str()), scene->mRootNode);
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
        const std::vector<Tick>& ticks = transform.getTicks();

        uint32_t i = 1;
        while(tick > ticks[i].mTick)
        {
            ++i;
        }

        BELL_ASSERT(i < ticks.size(), "tick out of bounds")
        const float4x4 boneTransform = interpolateTick(ticks[i - 1], ticks[i], tick);
        boneTransforms.push_back(boneTransform * bone.mInverseBindPose);
    }

    return boneTransforms;
}


float4x4 Animation::interpolateTick(const Tick& lhs, const Tick& rhs, const double tick) const
{
    const float lerpFactor = float(tick - lhs.mTick) / float(rhs.mTick - lhs.mTick);
    BELL_ASSERT(lerpFactor >= 0.0 && lerpFactor <= 1.0f, "Lerp factor out of range")

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


void Animation::readNodeHierarchy(const aiAnimation* anim, const aiString& name, const aiNode* rootNode)
{
    std::string nodeName(name.data);

    const aiNode* node = rootNode->FindNode(name.C_Str());
    BELL_ASSERT(node, "Unable to find node matching anim node")
    const aiNodeAnim* animNode = findNodeAnim(anim, nodeName);

    getParentTransform(anim, node, 0.0);
    getParentTransform(anim, node, mNumTicks);

    if(animNode)
    {
        std::set<double> uniqueTicks;
        for(uint32_t i = 0; i < animNode->mNumPositionKeys; ++i)
        {
            uniqueTicks.insert(animNode->mPositionKeys[i].mTime);
        }
        for(uint32_t i = 0; i < animNode->mNumScalingKeys; ++i)
        {
            uniqueTicks.insert(animNode->mScalingKeys[i].mTime);
        }
        for(uint32_t i = 0; i < animNode->mNumRotationKeys; ++i)
        {
            uniqueTicks.insert(animNode->mRotationKeys[i].mTime);
        }

        for(double tick : uniqueTicks)
        {
            getParentTransform(anim, node, tick);
        }
    }
}


float4x4 Animation::getParentTransform(const aiAnimation* anim, const aiNode* parent, const double tick)
{
    float4x4 nodeTransformation(aiMatrix4x4ToFloat4x4(parent->mTransformation));
    std::string nodeName(parent->mName.data);
    const aiNodeAnim* nodeAnim = findNodeAnim(anim, nodeName);

    if (nodeAnim)
    {
        const float4x4 scale = interpolateScale(tick, nodeAnim);
        const float4x4 rotation = interpolateRotation(tick, nodeAnim);
        const float4x4 translation = interpolateTranslation(tick, nodeAnim);

        nodeTransformation = translation * rotation * scale;
    }

    float4x4 finalTransforms;
    if(parent->mParent)
        finalTransforms = getParentTransform(anim, parent->mParent, tick) * nodeTransformation;
    else
        finalTransforms = nodeTransformation;

    if(mBones.find(nodeName) != mBones.end())
    {
        BoneTransform& boneTrans = mBones[nodeName];
        boneTrans.insert(tick, mInverseGlobalTransform * finalTransforms);
    }

    return finalTransforms;
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


float4x4 Animation::interpolateScale(double time, const aiNodeAnim* pNodeAnim)
{
    aiVector3D scale;

    if (pNodeAnim->mNumScalingKeys == 1)
    {
        scale = pNodeAnim->mScalingKeys[0].mValue;
    }
    else
    {
        uint32_t frameIndex = 0;
        for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
        {
            if (time <= (float)pNodeAnim->mScalingKeys[i + 1].mTime)
            {
                frameIndex = i;
                break;
            }
        }

        aiVectorKey currentFrame = pNodeAnim->mScalingKeys[frameIndex];
        aiVectorKey nextFrame = pNodeAnim->mScalingKeys[(frameIndex + 1) % pNodeAnim->mNumScalingKeys];

        float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

        const aiVector3D& start = currentFrame.mValue;
        const aiVector3D& end = nextFrame.mValue;

        scale = (start + delta * (end - start));
    }

    return glm::scale(float4x4(1.0f), float3(scale.x, scale.y, scale.z));
}


float4x4 Animation::interpolateTranslation(double time, const aiNodeAnim* pNodeAnim)
{
    aiVector3D translation;

    if (pNodeAnim->mNumPositionKeys == 1)
    {
        translation = pNodeAnim->mPositionKeys[0].mValue;
    }
    else
    {
        uint32_t frameIndex = 0;
        for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
        {
            if (time <= (float)pNodeAnim->mPositionKeys[i + 1].mTime)
            {
                frameIndex = i;
                break;
            }
        }

        aiVectorKey currentFrame = pNodeAnim->mPositionKeys[frameIndex];
        aiVectorKey nextFrame = pNodeAnim->mPositionKeys[(frameIndex + 1) % pNodeAnim->mNumPositionKeys];

        float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

        const aiVector3D& start = currentFrame.mValue;
        const aiVector3D& end = nextFrame.mValue;

        translation = (start + delta * (end - start));
    }

    return glm::translate(float4x4(1.0f), float3(translation.x, translation.y, translation.z));
}


float4x4 Animation::interpolateRotation(double time, const aiNodeAnim* pNodeAnim)
{
    aiQuaternion rotation;

    if (pNodeAnim->mNumRotationKeys == 1)
    {
        rotation = pNodeAnim->mRotationKeys[0].mValue;
    }
    else
    {
        uint32_t frameIndex = 0;
        for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
        {
            if (time <= (float)pNodeAnim->mRotationKeys[i + 1].mTime)
            {
                frameIndex = i;
                break;
            }
        }

        aiQuatKey currentFrame = pNodeAnim->mRotationKeys[frameIndex];
        aiQuatKey nextFrame = pNodeAnim->mRotationKeys[(frameIndex + 1) % pNodeAnim->mNumRotationKeys];

        float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

        const aiQuaternion& start = currentFrame.mValue;
        const aiQuaternion& end = nextFrame.mValue;

        aiQuaternion::Interpolate(rotation, start, end, delta);
        rotation.Normalize();
    }

    glm::quat quat;
    quat.x = rotation.x;
    quat.y = rotation.y;
    quat.z = rotation.z;
    quat.w = rotation.w;

    return glm::toMat4(quat);
}
