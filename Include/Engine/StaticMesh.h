#ifndef STATIC_MESH_HPP
#define STATIC_MESH_HPP

#include "Core/BellLogging.hpp"
#include "Engine/AABB.hpp"
#include "Engine/PassTypes.hpp"
#include "Engine/Animation.hpp"
#include "RenderGraph/GraphicsTask.hpp"

#include "assimp/vector2.h"
#include "assimp/vector3.h"
#include "assimp/scene.h"

#include <cstdint>
#include <string>
#include <vector>

struct MeshEntry
{
    float3x4 mTransformation;
    float3x4 mPreviousTransformation;
    uint32_t mMaterialIndex;
    uint32_t mMaterialFlags;
};
static_assert (sizeof(MeshEntry) <= 128, "Mesh Entry will no longer fit inside push constants");

class VertexBuffer
{
public:

    VertexBuffer() :
        mCurrentOffset{0},
        mBuffer{} {}

    void setSize(const size_t size)
    {
        mBuffer.resize(size);
    }

    void writeVertexVector4(const aiVector3D&);
    void writeVertexVector2(const aiVector2D&);
    void writeVertexFloat(const float);
    void WriteVertexInt(const uint32_t);
    void WriteVertexChar4(const char4&);

    const std::vector<unsigned char>& getVertexBuffer() const
    {
        return mBuffer;
    }

private:

    uint32_t mCurrentOffset;
    std::vector<unsigned char> mBuffer;

};

struct Bone
{
    std::string mName;
    uint16_t mParentIndex;
    float4x4 mInverseBindPose;
    OBB mOBB;
};

struct BoneIndex
{
    BoneIndex() :
            mBone(0),
            mWeight(0.0f) {}

    uint32_t mBone;
    float mWeight;
};

struct BoneIndicies
{
    BoneIndicies() :
            mBoneIndices{} {}

    std::vector<BoneIndex> mBoneIndices;
};

struct SubMesh
{
    uint32_t mVertexOffset;
    uint32_t mIndexOffset;
    uint32_t mIndexCount;

    float4x4 mTransform;

    std::vector<Bone> mSkeleton;
    std::vector<BoneIndex> mBoneWeights;
    std::vector<uint2> mBoneWeightsIndicies;
};

class StaticMesh
{
public:
	
    StaticMesh(const std::string& filePath, const int vertexAttributes, const bool globalScaling = false);
    StaticMesh(const aiScene* scene, const aiMesh* mesh, const int vertexAttributes);
    StaticMesh(const aiScene* scene, const int vertexAttributes);


    const AABB& getAABB() const
    {
        return mAABB;
    }

    AABB& getAABB()
    {
        return mAABB;
    }

    const std::vector<unsigned char>& getVertexData() const
    {
        return mVertexData.getVertexBuffer();
    }

    const std::vector<uint32_t>& getIndexData() const
    {
        return mIndexData;
    }

    int getVertexAttributes() const
    {
        return mVertexAttributes;
    }

    uint32_t getVertexStride() const
    {
	return mVertexStride;
    }

    uint64_t getVertexCount() const
    {
	return mVertexCount;
    }

    bool hasAnimations() const
    {
        return !mSkeletalAnimations.empty() || !mBlendAnimations.empty();
    }

    bool isSkeletalAnimation(const std::string& name) const
    {
        return mSkeletalAnimations.find(name) != mSkeletalAnimations.end();
    }

    bool isBlendMeshAnimation(const std::string& name) const
    {
        return mBlendAnimations.find(name) != mBlendAnimations.end();
    }

    SkeletalAnimation& getSkeletalAnimation(const std::string& name)
    {
        BELL_ASSERT(mSkeletalAnimations.find(name) != mSkeletalAnimations.end(), "Unable to find animation");
        auto it = mSkeletalAnimations.find(name);
        return it->second;
    }

    const std::map<std::string, SkeletalAnimation>& getAllSkeletalAnimations() const
    {
        return mSkeletalAnimations;
    }

    BlendMeshAnimation& getBlendMeshAnimation(const std::string& name)
    {
        BELL_ASSERT(mBlendAnimations.find(name) != mBlendAnimations.end(), "Unable to find animation");
        auto it = mBlendAnimations.find(name);
        return it->second;
    }

    const std::map<std::string, BlendMeshAnimation>& getAllBlendMeshAnimations() const
    {
        return mBlendAnimations;
    }

    const std::vector<MeshBlend>& getBlendMeshes() const
    {
        return mBlendMeshes;
    }

    const std::vector<SubMesh>& getSubMeshes() const
    {
        return mSubMeshes;
    }

    uint32_t getBoneCount() const
    {
        return mBoneCount;
    }

private:

    void configure(const aiScene *scene, const aiMesh* mesh, const int vertexAttributes);

    uint16_t findBoneParent(const aiNode*, aiBone** const, const uint32_t);
    void loadSkeleton(const aiScene* scene, const aiMesh* mesh, SubMesh& submesh);
    void loadBlendMeshed(const aiMesh* mesh);

    void loadAnimations(const aiScene*);

    uint32_t getPrimitiveSize(const aiPrimitiveType) const;

    void parseNode(const aiScene* scene,
                   const aiNode* node,
                   const aiMatrix4x4& parentTransofrmation,
                   const int vertAttributes);

    std::vector<SubMesh> mSubMeshes;

    VertexBuffer         mVertexData;
    std::vector<uint32_t> mIndexData;

    std::vector<MeshBlend> mBlendMeshes;

    std::map<std::string, SkeletalAnimation> mSkeletalAnimations;
    std::map<std::string, BlendMeshAnimation> mBlendAnimations;

    AABB mAABB;

    uint32_t mBoneCount;
	uint64_t mVertexCount;
    int mVertexAttributes;
	uint32_t mVertexStride;
};

#endif
