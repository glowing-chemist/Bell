#ifndef STATIC_MESH_HPP
#define STATIC_MESH_HPP

#include "Engine/AABB.hpp"
#include "Engine/PassTypes.hpp"

#include "assimp/vector2.h"
#include "assimp/vector3.h"

#include <cstdint>
#include <string>
#include <vector>


class StaticMesh
{
public:
	
	StaticMesh(const std::string& filePath);

	const AABB& getAABB() const
	{
		return mAABB;
	}

    AABB& getAABB()
    {
        return mAABB;
    }

    const std::vector<float>& getVertexData() const
	{
        return mVertexData;
	}

    const std::vector<uint32_t>& getIndexData() const
	{
        return mIndexData;
	}

    void addPass(const PassType passType)
    {
        mPassTypes = static_cast<PassType>(static_cast<uint64_t>(mPassTypes) | static_cast<uint64_t>(passType));
    }

    PassType getPassTypes() const
    {
        return mPassTypes;
    }

private:

    void writeVertexVector4(const aiVector3D&, const uint32_t);
    void writeVertexVector2(const aiVector2D&, const uint32_t);
	void writeVertexFloat(const float, const uint32_t);


    std::vector<float> mVertexData;
    std::vector<uint32_t> mIndexData;
	AABB mAABB;

    PassType mPassTypes;
};

#endif
