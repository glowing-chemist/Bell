#ifndef STATIC_MESH_HPP
#define STATIC_MESH_HPP

#include "Engine/AABB.hpp"

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

	const std::vector<float>& getVertexBuffer() const
	{
		return mVertexBuffer;
	}

	const std::vector<uint32_t>& getIndexBuffer() const
	{
		return mIndexBuffer;
	}

private:

	std::vector<float> mVertexBuffer;
	std::vector<uint32_t> mIndexBuffer;
	AABB mAABB;
};

#endif
