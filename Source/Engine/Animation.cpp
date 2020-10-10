#include "Engine/Animation.hpp"
#include "Engine/StaticMesh.h"
#include "Core/ConversionUtils.hpp"

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


SkeletalAnimation::SkeletalAnimation(const StaticMesh& mesh, const aiAnimation* anim, const aiScene* scene) :
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


std::vector<float4x4> SkeletalAnimation::calculateBoneMatracies(const StaticMesh& mesh, const double tick)
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


float4x4 SkeletalAnimation::interpolateTick(const Tick& lhs, const Tick& rhs, const double tick) const
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


void SkeletalAnimation::readNodeHierarchy(const aiAnimation* anim, const aiString& name, const aiNode* rootNode)
{
    std::string nodeName(name.data);

    const aiNode* node = rootNode->FindNode(name.C_Str());
    BELL_ASSERT(node, "Unable to find node matching anim node")
    const aiNodeAnim* animNode = findNodeAnim(anim, nodeName);

    getParentTransform(anim, node, 0.0);
    getParentTransform(anim, node, mNumTicks);

    struct DoubleComparator
    {
        bool operator()(const double lhs, const double rhs) const
        {
            const double diff = rhs - lhs;
            return diff > 0.001;
        }
    };

    if(animNode)
    {
        std::set<double, DoubleComparator> uniqueTicks;
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


float4x4 SkeletalAnimation::getParentTransform(const aiAnimation* anim, const aiNode* parent, const double tick)
{
    float4x4 nodeTransformation(aiMatrix4x4ToFloat4x4(parent->mTransformation));
    std::string nodeName(parent->mName.data);
    const aiNodeAnim* nodeAnim = findNodeAnim(anim, nodeName);

    BELL_ASSERT(tick <= mNumTicks, "Tick out of bounds")

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


const aiNodeAnim* SkeletalAnimation::findNodeAnim(const aiAnimation* animation, const std::string& nodeName)
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


float4x4 SkeletalAnimation::interpolateScale(double time, const aiNodeAnim* pNodeAnim)
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
            if (time <= pNodeAnim->mScalingKeys[i + 1].mTime)
            {
                frameIndex = i;
                break;
            }
        }

        BELL_ASSERT((frameIndex + 1) < pNodeAnim->mNumScalingKeys, "frame index out of bounds")

        aiVectorKey currentFrame = pNodeAnim->mScalingKeys[frameIndex];
        aiVectorKey nextFrame = pNodeAnim->mScalingKeys[(frameIndex + 1) % pNodeAnim->mNumScalingKeys];

        float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);
        BELL_ASSERT(delta >= 0.0 && delta <= 1.0, "Delta out of range")

        const aiVector3D& start = currentFrame.mValue;
        const aiVector3D& end = nextFrame.mValue;

        scale = (start + delta * (end - start));
    }

    return glm::scale(float4x4(1.0f), float3(scale.x, scale.y, scale.z));
}


float4x4 SkeletalAnimation::interpolateTranslation(double time, const aiNodeAnim* pNodeAnim)
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
            if (time <= pNodeAnim->mPositionKeys[i + 1].mTime)
            {
                frameIndex = i;
                break;
            }
        }

        BELL_ASSERT((frameIndex + 1) < pNodeAnim->mNumPositionKeys, "frame index out of bounds")

        aiVectorKey currentFrame = pNodeAnim->mPositionKeys[frameIndex];
        aiVectorKey nextFrame = pNodeAnim->mPositionKeys[(frameIndex + 1) % pNodeAnim->mNumPositionKeys];

        float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);
        BELL_ASSERT(delta >= 0.0 && delta <= 1.0, "Delta out of range")

        const aiVector3D& start = currentFrame.mValue;
        const aiVector3D& end = nextFrame.mValue;

        translation = (start + delta * (end - start));
    }

    return glm::translate(float4x4(1.0f), float3(translation.x, translation.y, translation.z));
}


float4x4 SkeletalAnimation::interpolateRotation(double time, const aiNodeAnim* pNodeAnim)
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
            if (time <= pNodeAnim->mRotationKeys[i + 1].mTime)
            {
                frameIndex = i;
                break;
            }
        }

        BELL_ASSERT((frameIndex + 1) < pNodeAnim->mNumRotationKeys, "frame index out of bounds")

        aiQuatKey currentFrame = pNodeAnim->mRotationKeys[frameIndex];
        aiQuatKey nextFrame = pNodeAnim->mRotationKeys[(frameIndex + 1) % pNodeAnim->mNumRotationKeys];

        float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);
        BELL_ASSERT(delta >= 0.0 && delta <= 1.0, "Delta out of range")

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


MeshBlend::MeshBlend(const aiAnimMesh* mesh)
{
    mName = mesh->mName.C_Str();
    mWeight = mesh->mWeight;

    for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
    {
        if (mesh->HasPositions())
        {
            mPosition.push_back(float3{mesh->mVertices[i].x, mesh->mVertices[i].y , mesh->mVertices[i].z});
        }
        else
        {
            mPosition.push_back(float3{ 0.0f, 0.0f, 0.0f });
        }

        if (mesh->HasNormals())
        {
            mNormals.push_back(float4{mesh->mNormals[i].x, mesh->mNormals[i].y , mesh->mNormals[i].z , 1.0f});
        }
        else
        {
            mNormals.push_back(float4{ 0.0f, 0.0f, 0.0f , 1.0f });
        }

        if (mesh->HasTextureCoords(0))
        {
            mUV.push_back(float2{mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y});
        }
        else
        {
            mUV.push_back(float2{ 0.0f, 0.0f });
        }

        if (mesh->HasVertexColors(0))
        {
            mColours.push_back(packColour(float4(mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b, mesh->mColors[0][i].a)));
        }
        else
        {
            mColours.push_back(0u);
        }
    }
}


BlendMeshAnimation::BlendMeshAnimation(const aiAnimation* anim, const aiScene* sceneRoot) :
    mName{anim->mName.C_Str()},
    mTicksPerSecond{ anim->mTicksPerSecond },
    mNumTicks{anim->mDuration},
    mTicks{}
{
    mName = anim->mName.C_Str();
    BELL_ASSERT(anim->mNumMorphMeshChannels == 1, "Only support single blend mesh animations atm")

    const aiMeshMorphAnim* meshAnim = anim->mMorphMeshChannels[0];
    for (uint32_t i = 0; i < meshAnim->mNumKeys; ++i)
    {
        Tick tick{};
        tick.mTime = meshAnim->mKeys[i].mTime;

        for (uint32_t j = 0; j < meshAnim->mKeys[i].mNumValuesAndWeights; ++j)
        {
            tick.mVertexIndex.push_back(meshAnim->mKeys[i].mValues[j]);
            tick.mWeight.push_back(meshAnim->mKeys[i].mWeights[j]);
        }

        mTicks.push_back(std::move(tick));
    }
}


std::vector<unsigned char> BlendMeshAnimation::getBlendedVerticies(const StaticMesh& mesh, const double tick) const
{
    uint32_t frameIndex = 0;
    if (mTicks.size() > 1)
    {
        for (uint32_t i = 0; i < mTicks.size() - 1; i++)
        {
            if (tick <= mTicks[i + 1].mTime)
            {
                frameIndex = i;
                break;
            }
        }
    }
    BELL_ASSERT((frameIndex + 1) < mTicks.size(), "frame index out of bounds")

    const Tick& currentTick = mTicks[frameIndex];
    const Tick& nextTick = mTicks[(frameIndex + 1) % mTicks.size()];
    const double currentBlendWeight = (tick - currentTick.mTime) / (nextTick.mTime - currentTick.mTime);
    const double nextBlendWeight = 1.0 - currentBlendWeight;

    const std::vector<MeshBlend>& shapes = mesh.getBlendMeshes();
    std::vector<double> blendShapeWeights(shapes.size(), 0.0);
    
    for (uint32_t i = 0; i < currentTick.mVertexIndex.size(); ++i)
    {
        const uint32_t shapeIndex = currentTick.mVertexIndex[i];
        const double shapeWeight = currentTick.mWeight[i];

        if (shapeWeight > 0.0)
        {
            blendShapeWeights[shapeIndex] += shapeWeight * currentBlendWeight;
        }
    }

    for (uint32_t i = 0; i < nextTick.mVertexIndex.size(); ++i)
    {
        const uint32_t shapeIndex = nextTick.mVertexIndex[i];
        const double shapeWeight = nextTick.mWeight[i];

        if (shapeWeight > 0.0)
        {
            blendShapeWeights[shapeIndex] += shapeWeight * nextBlendWeight;
        }
    }

    VertexBuffer newVertexBuffer{};
    newVertexBuffer.setSize(mesh.getVertexStride() * shapes[0].mPosition.size());
    for (uint32_t i = 0; i < shapes[0].mPosition.size(); ++i)
    {
        float3 position{0.0f, 0.0f, 0.0f};
        float4 normal{0.0f, 0.0f, 0.0f, 0.0f};
        float2 uv{0.0f, 0.0f};
        uint32_t colour{ 0 };

        for (uint32_t j = 0; j < blendShapeWeights.size(); ++j)
        {
            BELL_ASSERT(shapes[j].mPosition.size() == mesh.getVertexCount(), "Incorrect blend shape")

            const double shapeWeight = blendShapeWeights[j];
            if (shapeWeight > 0.0)
            {
                position += static_cast<float>(shapeWeight) * shapes[j].mPosition[i] * shapes[j].mWeight;
                normal += static_cast<float>(shapeWeight) * shapes[j].mNormals[i] * shapes[j].mWeight;
                uv += static_cast<float>(shapeWeight) * shapes[j].mUV[i] * shapes[j].mWeight;
                colour += packColour(static_cast<float>(shapeWeight) * unpackColour(shapes[j].mColours[i]));
            }
        }

        newVertexBuffer.writeVertexVector4(aiVector3D{ position.x, position.y, position.z });
        newVertexBuffer.writeVertexVector2(aiVector2D{ uv.x, uv.y });
        const char4 packednormals = packNormal(normal);
        newVertexBuffer.WriteVertexChar4(packednormals);
        newVertexBuffer.WriteVertexInt(colour);
    }

    return newVertexBuffer.getVertexBuffer();
}