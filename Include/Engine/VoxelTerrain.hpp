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

    const std::vector<int8_t>& getVoxelData(const uint32_t lod) const
    {
        return mVoxelData[lod];
    }

    std::vector<int8_t>& getVoxelData(const uint32_t lod)
    {
        return mVoxelData[lod];
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

    void generateLODs();

    uint3 mSize;
    float mVoxelSize;

    // for 3 LODS.
    std::vector<int8_t> mVoxelData[3];
};

#endif
