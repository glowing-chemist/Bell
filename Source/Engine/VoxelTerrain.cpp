#include "Engine/VoxelTerrain.hpp"

#include "TextureUtil.hpp"


VoxelTerrain::VoxelTerrain(const uint3& size, const float voxelSize) :
    mSize{size},
    mVoxelSize{voxelSize}
{
    mVoxelData.resize(mSize.x * mSize.y * mSize.z);
    std::memset(mVoxelData.data(), -1, mVoxelData.size());
}


void VoxelTerrain::initialiseFromHeightMap(const std::string& path)
{
    using namespace TextureUtil;

    const TextureInfo heightMap = load32BitTexture(path.c_str(), STBI_grey);
    BELL_ASSERT(heightMap.height == mSize.z && heightMap.width == mSize.x, "Height map resampling needs to be implemented")

    mVoxelData = TextureUtil::generateVoxelGridFromHeightMap(heightMap.mData, mSize.x, mSize.y, mSize.z, mVoxelSize);
}


void VoxelTerrain::initialiseFromData(const std::vector<int8_t>& data)
{
    BELL_ASSERT(data.size() == (mSize.x * mSize.y * mSize.z), "Data has incorrect dimensions")
    mVoxelData = data;
}

