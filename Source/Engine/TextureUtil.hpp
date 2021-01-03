#ifndef TEXTURE_UTIL_HPP
#define TEXTURE_UTIL_HPP

#include <algorithm>
#include <cstring>
#include <cstdint>
#include <vector>

#include "Core/BellLogging.hpp"
#include "stb_image.h"

namespace TextureUtil
{

struct TextureInfo
    {
	std::vector<unsigned char> mData;
	    int width;
	    int height;
    };

inline TextureInfo load32BitTexture(const char* filePath, int chanels)
    {
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filePath, &texWidth, &texHeight, &texChannels, chanels);
	BELL_ASSERT(pixels, "Failed to load image");

	BELL_LOG_ARGS("loading texture file: %s", filePath)
	    //BELL_ASSERT(texChannels == chanels, "Texture file has a different ammount of channels than requested")

	std::vector<unsigned char> imageData{};
	imageData.resize(texWidth * texHeight * chanels);

	std::memcpy(imageData.data(), pixels, texWidth * texHeight * chanels);

	stbi_image_free(pixels);

	return {imageData, texWidth, texHeight};
    }

inline TextureInfo load128BitTexture(const char* filePath, int chanels)
    {
    int texWidth, texHeight, texChannels;
    float* pixels = stbi_loadf(filePath, &texWidth, &texHeight, &texChannels, chanels);
    BELL_ASSERT(pixels, "Failed to load image");

    BELL_LOG_ARGS("loading texture file: %s", filePath)
        //BELL_ASSERT(texChannels == chanels, "Texture file has a different ammount of channels than requested")

    std::vector<unsigned char> imageData{};
    imageData.resize(texWidth * texHeight * chanels * sizeof(float));

    std::memcpy(imageData.data(), pixels, texWidth * texHeight * chanels * sizeof(float));

    stbi_image_free(pixels);

    return {imageData, texWidth, texHeight};
    }

template<typename T>
std::vector<T> generateMip(const std::vector<T>& tex, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t channels)
{
    BELL_ASSERT(tex.size() == width * height * depth * channels, "Incorrect texture dimensions")

    const uint32_t newWidth = std::max(1u, width / 2);
    const uint32_t newHeight = std::max(1u, height / 2);
    const uint32_t newDepth = std::max(1u, depth / 2);

    std::vector<T> mip{};
    mip.resize(newWidth * newHeight * newDepth * channels);

    for(uint32_t x = 0; x < newWidth; ++x)
    {
        for(uint32_t y = 0; y < newHeight; ++y)
        {
            for(uint32_t z = 0; z < newDepth; ++z)
            {
                for(uint32_t c = 0; c < channels; ++c)
                {
                    const int64_t v1 = tex[(y * width * 2 + z * width * height * 2 + x * 2) * channels + c];
                    const int64_t v2 = tex[((y * 2 + 1) * width + z * width * height * 2 + x * 2) * channels + c];
                    const int64_t v3 = tex[(y * width * 2 + z * width * height * 2 + (x * 2 + 1)) * channels + c];
                    const int64_t v4 = tex[((y * 2 + 1) * width + z * width * height * 2 + (x * 2 + 1)) * channels + c];

                    int64_t v;
                    if(depth > 1 && (z * 2 + 1) < depth)
                    {
                        const int64_t v5 = tex[(y * width * 2 + (z * 2 + 1) * width * height + x * 2) * channels + c];
                        const int64_t v6 = tex[((y * 2 + 1) * width + (z * 2 + 1) * width * height + x * 2) * channels + c];
                        const int64_t v7 = tex[(y * width * 2 + (z * 2 + 1) * width * height + (x * 2 + 1)) * channels + c];
                        const int64_t v8 = tex[((y * 2 + 1) * width + (z * 2 + 1) * width * height + (x * 2 + 1)) * channels + c];

                        v = (v1 + v2 + v3 + v4 + v5 + v6+ v7 + v8) / 8ll;
                    }
                    else
                        v = (v1 + v2 + v3 + v4) / 4ll;

                    mip[(y * newWidth + z * newWidth * newHeight + x) * channels + c] = v;
                }
            }
        }
    }

    return mip;
}

template<typename T>
std::vector<T> generateVoxelMip(const std::vector<T>& tex, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t channels)
{
    BELL_ASSERT(tex.size() == width * height * depth * channels, "Incorrect texture dimensions")

    const uint32_t newWidth = std::max(1u, width / 2);
    const uint32_t newHeight = std::max(1u, height / 2);
    const uint32_t newDepth = std::max(1u, depth / 2);

    std::vector<T> mip{};
    mip.resize(newWidth * newHeight * newDepth * channels);

    for(uint32_t x = 0; x < newWidth; ++x)
    {
        for(uint32_t y = 0; y < newHeight; ++y)
        {
            for(uint32_t z = 0; z < newDepth; ++z)
            {
                for(uint32_t c = 0; c < channels; ++c)
                {
                    const int64_t v1 = tex[(y * width * 2 + z * width * height * 2 + x * 2) * channels + c];
                    const int64_t v2 = tex[((y * 2 + 1) * width + z * width * height * 2 + x * 2) * channels + c];
                    const int64_t v3 = tex[(y * width * 2 + z * width * height * 2 + (x * 2 + 1)) * channels + c];
                    const int64_t v4 = tex[((y * 2 + 1) * width + z * width * height * 2 + (x * 2 + 1)) * channels + c];

                    int64_t v;
                    if(depth > 1 && (z * 2 + 1) < depth)
                    {
                        const int64_t v5 = tex[(y * width * 2 + (z * 2 + 1) * width * height + x * 2) * channels + c];
                        const int64_t v6 = tex[((y * 2 + 1) * width + (z * 2 + 1) * width * height + x * 2) * channels + c];
                        const int64_t v7 = tex[(y * width * 2 + (z * 2 + 1) * width * height + (x * 2 + 1)) * channels + c];
                        const int64_t v8 = tex[((y * 2 + 1) * width + (z * 2 + 1) * width * height + (x * 2 + 1)) * channels + c];

                        const int64_t minV = std::min(std::initializer_list<int64_t>{v1, v2, v3, v4, v5, v6, v7, v8});
                        const int64_t maxV = std::max(std::initializer_list<int64_t>{v1, v2, v3, v4, v5, v6, v7, v8});

                        if(std::abs(minV) > std::abs(maxV))
                            v = minV;
                        else
                            v = maxV;

                    }
                    else
                    {
                        const int64_t minV = std::min(std::initializer_list<int64_t>{v1, v2, v3, v4});
                        const int64_t maxV = std::max(std::initializer_list<int64_t>{v1, v2, v3, v4});
                        if(std::abs(minV) > std::abs(maxV))
                            v = minV;
                        else
                            v = maxV;
                    }

                    mip[(y * newWidth + z * newWidth * newHeight + x) * channels + c] = v;
                }
            }
        }
    }

    return mip;
}


template<typename T>
std::vector<T> generateMipMax(const std::vector<T>& tex, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t channels)
{
    BELL_ASSERT(tex.size() == width * height * depth * channels, "Incorrect texture dimensions")

    const uint32_t newWidth = std::max(1u, width / 2);
    const uint32_t newHeight = std::max(1u, height / 2);
    const uint32_t newDepth = std::max(1u, depth / 2);

    std::vector<T> mip{};
    mip.resize(newWidth * newHeight * newDepth * channels);

    for(uint32_t x = 0; x < newWidth; ++x)
    {
        for(uint32_t y = 0; y < newHeight; ++y)
        {
            for(uint32_t z = 0; z < newDepth; ++z)
            {
                for(uint32_t c = 0; c < channels; ++c)
                {
                    const int64_t v1 = tex[(y * width * 2 + z * width * height * 2 + x * 2) * channels + c];
                    const int64_t v2 = tex[((y * 2 + 1) * width + z * width * height * 2 + x * 2) * channels + c];
                    const int64_t v3 = tex[(y * width * 2 + z * width * height * 2 + (x * 2 + 1)) * channels + c];
                    const int64_t v4 = tex[((y * 2 + 1) * width + z * width * height * 2 + (x * 2 + 1)) * channels + c];

                    int64_t v;
                    if(depth > 1 && (z * 2 + 1) < depth)
                    {
                        const int64_t v5 = tex[(y * width * 2 + (z * 2 + 1) * width * height + x * 2) * channels + c];
                        const int64_t v6 = tex[((y * 2 + 1) * width + (z * 2 + 1) * width * height + x * 2) * channels + c];
                        const int64_t v7 = tex[(y * width * 2 + (z * 2 + 1) * width * height + (x * 2 + 1)) * channels + c];
                        const int64_t v8 = tex[((y * 2 + 1) * width + (z * 2 + 1) * width * height + (x * 2 + 1)) * channels + c];

                        v = std::max(std::initializer_list<int64_t>{v1, v2, v3, v4, v5, v6, v7, v8});
                    }
                    else
                        v = std::max(std::initializer_list<int64_t>{v1, v2, v3, v4});

                    mip[(y * newWidth + z * newWidth * newHeight + x) * channels + c] = v;
                }
            }
        }
    }

    return mip;
}

template<typename T>
std::vector<T> generateMipMin(const std::vector<T>& tex, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t channels)
{
    BELL_ASSERT(tex.size() == width * height * depth * channels, "Incorrect texture dimensions")

    const uint32_t newWidth = std::max(1u, width / 2);
    const uint32_t newHeight = std::max(1u, height / 2);
    const uint32_t newDepth = std::max(1u, depth / 2);

    std::vector<T> mip{};
    mip.resize(newWidth * newHeight * newDepth * channels);

    for(uint32_t x = 0; x < newWidth; ++x)
    {
        for(uint32_t y = 0; y < newHeight; ++y)
        {
            for(uint32_t z = 0; z < newDepth; ++z)
            {
                for(uint32_t c = 0; c < channels; ++c)
                {
                    const int64_t v1 = tex[(y * width * 2 + z * width * height * 2 + x * 2) * channels + c];
                    const int64_t v2 = tex[((y * 2 + 1) * width + z * width * height * 2 + x * 2) * channels + c];
                    const int64_t v3 = tex[(y * width * 2 + z * width * height * 2 + (x * 2 + 1)) * channels + c];
                    const int64_t v4 = tex[((y * 2 + 1) * width + z * width * height * 2 + (x * 2 + 1)) * channels + c];

                    int64_t v;
                    if(depth > 1 && (z * 2 + 1) < depth)
                    {
                        const int64_t v5 = tex[(y * width * 2 + (z * 2 + 1) * width * height + x * 2) * channels + c];
                        const int64_t v6 = tex[((y * 2 + 1) * width + (z * 2 + 1) * width * height + x * 2) * channels + c];
                        const int64_t v7 = tex[(y * width * 2 + (z * 2 + 1) * width * height + (x * 2 + 1)) * channels + c];
                        const int64_t v8 = tex[((y * 2 + 1) * width + (z * 2 + 1) * width * height + (x * 2 + 1)) * channels + c];

                        v = std::min(std::initializer_list<int64_t>{v1, v2, v3, v4, v5, v6, v7, v8});
                    }
                    else
                        v = std::min(std::initializer_list<int64_t>{v1, v2, v3, v4});

                    mip[(y * newWidth + z * newWidth * newHeight + x) * channels + c] = v;
                }
            }
        }
    }

    return mip;
}

inline std::vector<int8_t> generateVoxelGridFromHeightMap(const std::vector<unsigned char>& heightMap, const uint32_t width, const uint32_t height, const uint32_t depth, const float voxelSize)
{
    std::vector<int8_t> voxelData{};
    voxelData.resize(width * height * depth);

    for(uint32_t x = 0; x < width; ++x)
    {
        for(uint32_t y = 0; y < height; ++y)
        {
            for(uint32_t z = 0; z < depth; ++z)
            {
                const uint32_t voxelIndex = (z * width) + (y * width * depth) + x;
                BELL_ASSERT(voxelIndex < voxelData.size(), "OOB index")
                const uint32_t heightIndex = (z * width) + x;
                BELL_ASSERT(heightIndex < heightMap.size(), "OOB index")

                const float voxelHeight = y * voxelSize;
                const float heightSample = (float(heightMap[heightIndex]) / 255.0f) * height * voxelSize;
                const float f = heightSample > voxelHeight ? -127.0f * (1.0f - (voxelHeight / heightSample)) : 127.0f * ((voxelHeight - heightSample) / ((height * voxelSize) - heightSample));

                voxelData[voxelIndex] = f;
            }
        }
    }

    return voxelData;
}

}

#endif
