#include "Engine/Animation.hpp"
#include "Engine/StaticMesh.h"
#include "Core/ConversionUtils.hpp"

#include "Core/BellLogging.hpp"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "glm/gtx/handed_coordinate_space.hpp"

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
        mRootTransform(1.0f),
        mBones()
{
    const std::vector<Bone> &bones = mesh.getSkeleton();
    for (const auto &bone : bones)
    {
        readNodeHierarchy(anim, bone.mName, scene->mRootNode);
    }

    // get all transforms from scene root to root bone
    const Bone& rootBone = bones[0];
    BELL_ASSERT(rootBone.mParentIndex == 0XFFFF, "not root bone")
    const aiNode* rootBoneNode = scene->mRootNode->FindNode(rootBone.mName.c_str());
    mRootTransform = glm::inverse(aiMatrix4x4ToFloat4x4(rootBoneNode->mTransformation)) * rootBone.mLocalMatrix;
}


std::vector<float4x4> SkeletalAnimation::calculateBoneMatracies(const StaticMesh& mesh, const double tick)
{
    const std::vector<SubMesh>& subMeshes = mesh.getSubMeshes();
    std::vector<float4x4> boneTransforms{};
    boneTransforms.reserve(mesh.getBoneCount());

    const auto &bones = mesh.getSkeleton();
    for (const auto &bone : bones)
    {
        BELL_ASSERT(mBones.find(bone.mName) != mBones.end(), "Bone not found")
        BoneTransform &transforms = mBones[bone.mName];
        float4x4 transform{};
        float4x4 rootTransform(1.0f);
        if(transforms.mScales.empty() && transforms.mPositions.empty() && transforms.mRotations.empty())
            transform = bone.mLocalMatrix;
        else
        {
            transform = transforms.getBoneTransform(tick);
            rootTransform = mRootTransform;
        }

        uint16_t parent = bone.mParentIndex;
        while(parent != 0xFFFF)
        {
            const Bone& parentBone = bones[parent];
            BoneTransform &parentTransforms = mBones[parentBone.mName];
            float4x4 parentTransform = parentTransforms.getBoneTransform(tick);

            transform = parentTransform * transform;

            parent = parentBone.mParentIndex;
        }

        boneTransforms.emplace_back(rootTransform * transform * bone.mInverseBindPose);
    }

    return boneTransforms;
}


void SkeletalAnimation::readNodeHierarchy(const aiAnimation* anim, const std::string& name, const aiNode* rootNode)
{
    const aiNode* node = rootNode->FindNode(name.c_str());
    BELL_ASSERT(node, "Unable to find node matching anim node")
    const aiNodeAnim* animNode = findNodeAnim(anim, name);

    BoneTransform transforms{};
    if(animNode)
    {
        for(uint32_t i = 0; i < animNode->mNumPositionKeys; ++i)
        {
            transforms.mPositions.push_back({animNode->mPositionKeys[i].mTime, float3{animNode->mPositionKeys[i].mValue.x,
                    animNode->mPositionKeys[i].mValue.y,
                    animNode->mPositionKeys[i].mValue.z}});
        }
        for(uint32_t i = 0; i < animNode->mNumScalingKeys; ++i)
        {
            transforms.mScales.push_back({animNode->mScalingKeys[i].mTime, float3{animNode->mScalingKeys[i].mValue.x,
                                                                                        animNode->mScalingKeys[i].mValue.y,
                                                                                        animNode->mScalingKeys[i].mValue.z}});
        }
        for(uint32_t i = 0; i < animNode->mNumRotationKeys; ++i)
        {
            quat rotation;
            rotation.x = animNode->mRotationKeys[i].mValue.x;
            rotation.y = animNode->mRotationKeys[i].mValue.y;
            rotation.z = animNode->mRotationKeys[i].mValue.z;
            rotation.w = animNode->mRotationKeys[i].mValue.w;
            transforms.mRotations.push_back({animNode->mRotationKeys[i].mTime, rotation});
        }
    }

    mBones[name] = transforms;
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


float4x4 SkeletalAnimation::BoneTransform::getBoneTransform(const double tick)
{
    const float3 scale = interpolateScale(tick);
    const float3 position = interpolateTranslation(tick);
    const quat rotation = interpolateRotation(tick);

    return glm::translate(position) * glm::mat4_cast(rotation) * glm::scale(scale);
}


float3 SkeletalAnimation::BoneTransform::interpolateScale(double time)
{
    float3 scale;

    if(mScales.empty())
    {
        return float3{1.0f, 1.0f, 1.0f};
    }
    else if (mScales.size() == 1)
    {
        scale = mScales[0].mValue;
    }
    else
    {
        uint32_t frameIndex = 0;
        for (uint32_t i = 0; i < mScales.size() - 1; i++)
        {
            if (time <= mScales[i + 1].mTime)
            {
                frameIndex = i;
                break;
            }
        }

        BELL_ASSERT((frameIndex + 1) < mScales.size(), "frame index out of bounds")

        const ScaleKey& currentFrame = mScales[frameIndex];
        const ScaleKey& nextFrame = mScales[(frameIndex + 1) % mScales.size()];

        double delta = (time - currentFrame.mTime) / (nextFrame.mTime - currentFrame.mTime);
        BELL_ASSERT(delta >= 0.0 && delta <= 1.0, "Delta out of range")

        const float3& start = currentFrame.mValue;
        const float3& end = nextFrame.mValue;

        scale = (start + static_cast<float>(delta) * (end - start));
    }

    return scale;
}


float3 SkeletalAnimation::BoneTransform::interpolateTranslation(double time)
{
    float3 translation;

    if(mPositions.empty())
    {
        return float3{0.0f, 0.0f, 0.0f};
    }
    else if (mPositions.size() == 1)
    {
        translation = mPositions[0].mValue;
    }
    else
    {
        uint32_t frameIndex = 0;
        for (uint32_t i = 0; i < mPositions.size() - 1; i++)
        {
            if (time <= mPositions[i + 1].mTime)
            {
                frameIndex = i;
                break;
            }
        }

        BELL_ASSERT((frameIndex + 1) < mPositions.size(), "frame index out of bounds")

        const PositionKey& currentFrame = mPositions[frameIndex];
        const PositionKey& nextFrame = mPositions[(frameIndex + 1) % mPositions.size()];

        double delta = (time - currentFrame.mTime) / (nextFrame.mTime - currentFrame.mTime);
        BELL_ASSERT(delta >= 0.0 && delta <= 1.0, "Delta out of range")

        const float3& start = currentFrame.mValue;
        const float3& end = nextFrame.mValue;

        translation = (start + static_cast<float>(delta) * (end - start));
    }

    return translation;
}


quat SkeletalAnimation::BoneTransform::interpolateRotation(double time)
{
    quat rotation;

    if(mRotations.empty())
    {
        return quat{1.0f, 0.0f, 0.0f, 0.0f};
    }
    else if (mRotations.size() == 1)
    {
        rotation = mRotations[0].mValue;
    }
    else
    {
        uint32_t frameIndex = 0;
        for (uint32_t i = 0; i < mRotations.size() - 1; i++)
        {
            if (time <= mRotations[i + 1].mTime)
            {
                frameIndex = i;
                break;
            }
        }

        BELL_ASSERT((frameIndex + 1) < mRotations.size(), "frame index out of bounds")

        const RotationKey& currentFrame = mRotations[frameIndex];
        const RotationKey& nextFrame = mRotations[(frameIndex + 1) % mRotations.size()];

        double delta = (time - currentFrame.mTime) / (nextFrame.mTime - currentFrame.mTime);
        BELL_ASSERT(delta >= 0.0 && delta <= 1.0, "Delta out of range")

        const quat& start = currentFrame.mValue;
        const quat& end = nextFrame.mValue;

        rotation = glm::slerp(start, end, static_cast<float>(delta));
    }

    rotation = glm::normalize(rotation);
    return rotation;
}


MeshBlend::MeshBlend(const aiAnimMesh* mesh)
{
    mName = mesh->mName.C_Str();
    mWeight = mesh->mWeight;

    for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
    {
        if (mesh->HasPositions())
        {
            mPosition.emplace_back(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        }
        else
        {
            mPosition.emplace_back(0.0f, 0.0f, 0.0f );
        }

        if (mesh->HasNormals())
        {
            mNormals.emplace_back(mesh->mNormals[i].x, mesh->mNormals[i].y , mesh->mNormals[i].z , 1.0f);
        }
        else
        {
            mNormals.emplace_back(0.0f, 0.0f, 0.0f , 1.0f);
        }

        if (mesh->HasTangentsAndBitangents())
        {
            float3 normal = float3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            float3 tangent = float3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            float3 biTangent = float3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);

            float3 calculatedBitangent = glm::normalize(glm::cross(normal, tangent));
            float bitangentSign;
            if (glm::leftHanded(tangent, biTangent, normal) == glm::leftHanded(tangent, calculatedBitangent, normal))
                bitangentSign = 1.0f;
            else
                bitangentSign = -1.0f;

            mTangents.emplace_back(mesh->mTangents[i].x, mesh->mTangents[i].y , mesh->mTangents[i].z , bitangentSign);
        }
        else
        {
            mTangents.emplace_back(0.0f, 0.0f, 0.0f , 1.0f );
        }

        if (mesh->HasTextureCoords(0))
        {
            mUV.emplace_back(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
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
    const double currentBlendWeight = 1.0 - ((tick - currentTick.mTime) / (nextTick.mTime - currentTick.mTime));
    const double nextBlendWeight = 1.0 - currentBlendWeight;

    const std::vector<MeshBlend>& shapes = mesh.getBlendMeshes();
    std::vector<double> blendShapeWeights(shapes.size(), 0.0);
    
    for (uint32_t i = 0; i < currentTick.mVertexIndex.size(); ++i)
    {
        const uint32_t shapeIndex = currentTick.mVertexIndex[i];
        const double shapeWeight = currentTick.mWeight[i];

        if (shapeWeight > 0.0)
        {
            BELL_ASSERT(shapeIndex < blendShapeWeights.size(), "Invalid index")
            blendShapeWeights[shapeIndex] += shapeWeight * currentBlendWeight;
        }
    }

    for (uint32_t i = 0; i < nextTick.mVertexIndex.size(); ++i)
    {
        const uint32_t shapeIndex = nextTick.mVertexIndex[i];
        const double shapeWeight = nextTick.mWeight[i];

        if (shapeWeight > 0.0)
        {
            BELL_ASSERT(shapeIndex < blendShapeWeights.size(), "Invalid index")
            blendShapeWeights[shapeIndex] += shapeWeight * nextBlendWeight;
        }
    }

    VertexBuffer newVertexBuffer{};
    newVertexBuffer.setSize(mesh.getVertexStride() * shapes[0].mPosition.size());
    for (uint32_t i = 0; i < shapes[0].mPosition.size(); ++i)
    {
        float3 position{0.0f, 0.0f, 0.0f};
        float4 normal{0.0f, 0.0f, 0.0f, 0.0f};
        float4 tangent{ 0.0f, 0.0f, 0.0f, 0.0f };
        float2 uv{0.0f, 0.0f};
        uint32_t colour{ 0 };

        float weight = 0.0f;
        for (uint32_t j = 0; j < blendShapeWeights.size(); ++j)
        {
            BELL_ASSERT(shapes[j].mPosition.size() == mesh.getVertexCount(), "Incorrect blend shape")

            const double shapeWeight = blendShapeWeights[j];
            BELL_ASSERT(shapeWeight >= 0.0, "Invalid weight")
            if (shapeWeight > 0.0)
            {
                weight += shapeWeight * shapes[j].mWeight;
                position += static_cast<float>(shapeWeight) * shapes[j].mPosition[i] * shapes[j].mWeight;
                normal += static_cast<float>(shapeWeight) * shapes[j].mNormals[i] * shapes[j].mWeight;
                tangent += static_cast<float>(shapeWeight) * shapes[j].mTangents[i] * shapes[j].mWeight;
                uv += static_cast<float>(shapeWeight) * shapes[j].mUV[i] * shapes[j].mWeight;
                colour += packColour(static_cast<float>(shapeWeight) * unpackColour(shapes[j].mColours[i]));
            }
        }

        newVertexBuffer.writeVertexVector4(aiVector3D{ position.x, position.y, position.z } / weight);
        newVertexBuffer.writeVertexVector2(aiVector2D{ uv.x, uv.y } / weight);
        const char4 packednormals = packNormal(normal / weight);
        newVertexBuffer.WriteVertexChar4(packednormals);
        const char4 packedtangent = packNormal(tangent / weight);
        newVertexBuffer.WriteVertexChar4(packedtangent);
        newVertexBuffer.WriteVertexInt(colour / weight);
    }

    return newVertexBuffer.getVertexBuffer();
}