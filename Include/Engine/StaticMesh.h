#ifndef STATIC_MESH_HPP
#define STATIC_MESH_HPP

#include "Engine/AABB.hpp"
#include "Engine/PassTypes.hpp"
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


class StaticMesh
{
public:
	
    StaticMesh(const std::string& filePath, const int vertexAttributes);
    StaticMesh(const aiMesh* mesh, const int vertexAttributes);


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
        return mVertexData;
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

    struct Bone
    {
        uint32_t mParentIndex;
        std::string mName;
        float4x4 mInverseBindPose;
        float4x4 mBindPose;
    };

    struct BoneIndex
    {
        uint32_t mBone;
        float mWeight;
    };

    struct BoneIndicies
    {
        BoneIndex mBoneIndices[4];
        uint8_t mUsedBones = 0;
    };

private:

    void configure(const aiMesh* mesh, const int vertexAttributes);
    void loadSkeleton(const aiMesh* mesh);

    void writeVertexVector4(const aiVector3D&, const uint32_t);
    void writeVertexVector2(const aiVector2D&, const uint32_t);
    void writeVertexFloat(const float, const uint32_t);
    void WriteVertexInt(const uint32_t, const uint32_t);

    uint32_t getPrimitiveSize(const aiPrimitiveType) const;

    std::vector<Bone> mSkeleton;
    std::vector<BoneIndicies> mBonesPerVertex;
    std::vector<unsigned char> mVertexData;
    std::vector<uint32_t> mIndexData;
    AABB mAABB;

    uint64_t mPassTypes;
	uint64_t mVertexCount;
    int mVertexAttributes;
	uint32_t mVertexStride;
};

#endif
