#ifndef TEXTURE_UTIL_HPP
#define TEXTURE_UTIL_HPP

#include <cstring>
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

}

#endif
