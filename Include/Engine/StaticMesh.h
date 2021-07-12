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

class Buffer;
class BufferView;

struct MeshEntry
{
    float3x4 mTransformation;
    float3x4 mPreviousTransformation;
    uint32_t mMaterialIndex;
    uint32_t mMaterialFlags;
    uint32_t mGlobalBoneBufferOffset;
    uint32_t mBoneCountBufferIndex;
    uint32_t mBoneWeightBufferIndex;
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
    float4x4 mLocalMatrix;
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
    uint32_t mVertexCount;
    uint32_t mIndexOffset;
    uint32_t mIndexCount;

    float4x4 mTransform;
};

class StaticMesh
{
public:
	
    StaticMesh(const std::string& filePath, const int vertexAttributes, const bool globalScaling = false);
    StaticMesh(const aiScene* scene, const aiMesh* mesh, const int vertexAttributes);
    StaticMesh(const aiScene* scene, const int vertexAttributes);
    ~StaticMesh();


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

    Buffer* getVertexBuffer()
    {
        return mVertexBuffer;
    }

    const Buffer* getVertexBuffer() const
    {
        return mVertexBuffer;
    }

    const BufferView* getVertexBufferView() const
    {
        return mVertexBufferView;
    }

    Buffer* getIndexBuffer()
    {
        return mIndexBuffer;
    }

    const Buffer* getIndexBuffer() const
    {
        return mIndexBuffer;
    }

    const BufferView* getIndexBufferView() const
    {
        return mIndexBufferView;
    }

    const BufferView* getBonesPerVertexBufferView() const
    {
        return mBonePerVertexBufferView;
    }

    const BufferView* getBoneIndexWeightBufferView() const
    {
        return mBoneIndexWeightBufferView;
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
        return mSkeleton.size();
    }

    uint32_t getSubMeshCount() const
    {
	return mSubMeshes.size();
    }

    const std::vector<Bone>& getSkeleton() const
    {
        return mSkeleton;
    }

    const std::vector<BoneIndex>& getBoneWeights() const
    {
        return mBoneWeights;
    }

    const std::vector<uint2>& getBoneIndicies() const
    {
        return mBonesPerVertex;
    }

    void initializeDeviceBuffers(RenderEngine*);

private:

    void configure(const aiScene *scene, const aiMesh* mesh, const float4x4 transform, const int vertexAttributes);

    uint16_t findBoneParent(const aiNode*, float4x4&);
    void loadSkeleton(const aiScene* scene, const aiMesh* mesh);
    void loadBlendMeshed(const aiMesh* mesh);

    void loadAnimations(const aiScene*);

    uint32_t getPrimitiveSize(const aiPrimitiveType) const;

    void parseNode(const aiScene* scene,
                   const aiNode* node,
                   const aiMatrix4x4& parentTransofrmation,
                   const int vertAttributes);

    std::vector<SubMesh> mSubMeshes;

    std::unordered_map<std::string, uint32_t> mBoneIndexMap;
    std::vector<Bone> mSkeleton;
    std::vector<BoneIndex> mBoneWeights;
    std::vector<uint2> mBonesPerVertex;

    VertexBuffer         mVertexData;
    std::vector<uint32_t> mIndexData;

    Buffer* mVertexBuffer;
    BufferView* mVertexBufferView;
    Buffer* mIndexBuffer;
    BufferView* mIndexBufferView;

    // Animation buffers.
    Buffer* mBonePerVertexBuffer;
    BufferView* mBonePerVertexBufferView;
    Buffer* mBoneIndexWeightBuffer;
    BufferView* mBoneIndexWeightBufferView;
    // Bone transforms buffer will be per instance.

    std::vector<MeshBlend> mBlendMeshes;

    std::map<std::string, SkeletalAnimation> mSkeletalAnimations;
    std::map<std::string, BlendMeshAnimation> mBlendAnimations;

    AABB mAABB;

	uint64_t mVertexCount;
    int mVertexAttributes;
	uint32_t mVertexStride;
};

#endif
