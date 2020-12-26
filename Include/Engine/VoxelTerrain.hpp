#ifndef VOXEL_TERRAIN_HPP
#define VOXEL_TERRAIN_HPP

#include "GeomUtils.h"
#include <string>
#include <vector>


class VoxelTerrain
{
public:
    VoxelTerrain(const uint3& size, const float voxelSize);
    ~VoxelTerrain() = default;

    void initialiseFromHeightMap(const std::string& path);
    void initialiseFromData(const std::vector<int8_t>&);

    const std::vector<int8_t>& getVoxelData() const
    {
	return mVoxelData;
    }

    std::vector<int8_t>& getVoxelData()
    {
	return mVoxelData;
    }

    uint3 getSize() const
    {
	return mSize;
    }

    float getVoxelSize() const
    {
	return mVoxelSize;
    }

private:

    uint3 mSize;
    float mVoxelSize;

    std::vector<int8_t> mVoxelData;
};

#endif
