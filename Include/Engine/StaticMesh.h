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


class StaticMesh
{
public:
	
    StaticMesh(const std::string& filePath, const int vertexAttributes);
    StaticMesh(const std::string& filePath, const int vertexAttributes, const uint32_t materialID);
    StaticMesh(const aiMesh* mesh, const int vertexAttributes, const uint32_t materialID);
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

private:

    void configure(const aiMesh* mesh, const int vertexAttributes, const uint32_t materialID);
    void configure(const aiMesh* mesh, const int vertexAttributes);

    void writeVertexVector4(const aiVector3D&, const uint32_t);
    void writeVertexVector2(const aiVector2D&, const uint32_t);
    void writeVertexFloat(const float, const uint32_t);
    void WriteVertexInt(const uint32_t, const uint32_t);

    uint32_t getPrimitiveSize(const aiPrimitiveType) const;


    std::vector<unsigned char> mVertexData;
    std::vector<uint32_t> mIndexData;
    AABB mAABB;

    uint64_t mPassTypes;
};

#endif
