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

class StaticMesh
{
public:
	
    StaticMesh(const std::string& filePath, const int vertexAttributes, const bool globalScaling = false);
    StaticMesh(const aiScene* scene, const aiMesh* mesh, const int vertexAttributes);


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

    void addPass(const PassType passType)
    {
        mPassTypes |= static_cast<uint64_t>(passType);
    }

    PassType getPassTypes() const
    {
        return static_cast<PassType>(mPassTypes);
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
        return !mSkeleton.empty() || !mBlendMeshes.empty();
    }

    struct Bone
    {
        std::string mName;
        float4x4 mInverseBindPose;
	OBB mOBB;
    };

    const std::vector<Bone>& getSkeleton() const
    {
        return mSkeleton;
    }

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

    const std::vector<uint2>& getBoneIndicies() const
    {
        return mBoneWeightsIndicies;
    }

    const std::vector<BoneIndex>& getBoneWeights() const
    {
        return mBoneWeights;
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

private:

    void configure(const aiScene *scene, const aiMesh* mesh, const int vertexAttributes);
    void loadSkeleton(const aiMesh* mesh);
    void loadBlendMeshed(const aiMesh* mesh);

    uint32_t getPrimitiveSize(const aiPrimitiveType) const;

    std::vector<Bone> mSkeleton;
    std::vector<BoneIndex> mBoneWeights;
    std::vector<uint2> mBoneWeightsIndicies;
    VertexBuffer         mVertexData;
    std::vector<uint32_t> mIndexData;

    std::vector<MeshBlend> mBlendMeshes;

    std::map<std::string, SkeletalAnimation> mSkeletalAnimations;
    std::map<std::string, BlendMeshAnimation> mBlendAnimations;

    AABB mAABB;

    uint64_t mPassTypes;
	uint64_t mVertexCount;
    int mVertexAttributes;
	uint32_t mVertexStride;
};

#endif
