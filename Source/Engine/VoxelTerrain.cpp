#include "Engine/VoxelTerrain.hpp"

#include "TextureUtil.hpp"


VoxelTerrain::VoxelTerrain(const uint3& size, const float voxelSize) :
    mSize{size},
    mVoxelSize{voxelSize}
{
    mVoxelData[0].resize(mSize.x * mSize.y * mSize.z);
    std::memset(mVoxelData[0].data(), -1, mVoxelData[0].size());
}


void VoxelTerrain::initialiseFromHeightMap(const std::string& path)
{
    using namespace TextureUtil;

    const TextureInfo heightMap = load32BitTexture(path.c_str(), STBI_grey);
    BELL_ASSERT(heightMap.height == mSize.z && heightMap.width == mSize.x, "Height map resampling needs to be implemented")

    mVoxelData[0] = TextureUtil::generateVoxelGridFromHeightMap(heightMap.mData, mSize.x, mSize.y, mSize.z, mVoxelSize);
    generateLODs();
}


void VoxelTerrain::initialiseFromData(const std::vector<int8_t>& data)
{
    BELL_ASSERT(data.size() == (mSize.x * mSize.y * mSize.z), "Data has incorrect dimensions")
    mVoxelData[0] = data;
    generateLODs();
}


void VoxelTerrain::generateLODs()
{
    mVoxelData[1] = TextureUtil::generateMipMax<int8_t>(mVoxelData[0], mSize.x, mSize.z, mSize.y, 1);
    mVoxelData[2] = TextureUtil::generateMipMax<int8_t>(mVoxelData[1], mSize.x / 2, mSize.z / 2, mSize.y / 2, 1);
}
