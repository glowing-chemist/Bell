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

    for(uint32_t x = 0; x < mSize.x; ++x)
    {
        for(uint32_t y = 0; y < mSize.y; ++y)
        {
            for(uint32_t z = 0; z < mSize.z; ++z)
            {
                const uint32_t voxelIndex = (z * mSize.x) + (y * mSize.x * mSize.z) + x;
                BELL_ASSERT(voxelIndex < mVoxelData.size(), "OOB index")
                const uint32_t heightIndex = (z * mSize.x) + x;
                BELL_ASSERT(heightIndex < heightMap.mData.size(), "OOB index")

                const float voxelHeight = y * mVoxelSize;

                mVoxelData[voxelIndex] = ((float(heightMap.mData[heightIndex]) / 255.0f) * mSize.y * mVoxelSize * 0.9f) > voxelHeight ? 127 : -127;
            }
        }
    }
}


void VoxelTerrain::initialiseFromData(const std::vector<int8_t>& data)
{
    BELL_ASSERT(data.size() == (mSize.x * mSize.y * mSize.z), "Data has incorrect dimensions")
    mVoxelData = data;
}
