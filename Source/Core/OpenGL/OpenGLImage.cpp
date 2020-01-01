#include "OpenGLImage.hpp"
#include "OpenGLRenderDevice.hpp"
#include "Core/ConversionUtils.hpp"

#include "glad/glad.h"


OpenGLImage::OpenGLImage(RenderDevice* dev,
	const Format format,
	const ImageUsage usage,
	const uint32_t x,
	const uint32_t y,
	const uint32_t z,
	const uint32_t mips,
	const uint32_t levels,
	const uint32_t samples,
	const std::string& name) :
	ImageBase(dev, format, usage, x, y, z, mips, levels, samples, name)
{
	mSlot = GL_TEXTURE_1D;
	if (usage & ImageUsage::CubeMap)
	{
		BELL_ASSERT(levels % 6 == 0, "Cubemaps must have a multiple of 6 layers.")
		mSlot = GL_TEXTURE_CUBE_MAP;
	}
	else if (usage & ImageUsage::DepthStencil)
	{
		mSlot = GL_TEXTURE_DEPTH;
	}
	else
	{
		if (y > 1)
		{
			if (levels > 1)
				mSlot = GL_TEXTURE_2D_ARRAY;
			else
				mSlot = GL_TEXTURE_2D;
		}
		if (z > 1)
		{
			mSlot = GL_TEXTURE_3D;
		}
	}

	glGenTextures(1, &mImage);
	glBindTexture(mSlot, mImage);

	for (uint32_t arrayLevel = 0; arrayLevel < mNumberOfLevels; ++arrayLevel)
	{
		for (uint32_t mipsLevel = 0; mipsLevel < mNumberOfMips; ++mipsLevel)
		{
			texImage(arrayLevel, mipsLevel, nullptr);
		}
	}

}


OpenGLImage::~OpenGLImage()
{
	getDevice()->destroyImage(*this);
}


void OpenGLImage::swap(ImageBase& other)
{
	uint32_t tempImage = mImage;

	OpenGLImage& GLImage = static_cast<OpenGLImage&>(other);

	GLImage.mImage = tempImage;
}


void OpenGLImage::setContents(const void* data,
	const uint32_t xsize,
	const uint32_t ysize,
	const uint32_t zsize,
	const uint32_t arrayLevel,
	const uint32_t mipsLevel,
	const int32_t offsetx,
	const int32_t offsety,
	const int32_t offsetz)
{
	if (mSlot == GL_TEXTURE_2D)
	{
		glTexSubImage2D(mSlot, arrayLevel, offsetx, offsety, (*mSubResourceInfo)[(arrayLevel * mNumberOfLevels) + mipsLevel].mExtent.width,
			(*mSubResourceInfo)[(arrayLevel * mNumberOfLevels) + mipsLevel].mExtent.height,
			getOpenGLImageFormat(mFormat), GL_UNSIGNED_BYTE, data);
	}
	else if (mSlot == GL_TEXTURE_1D)
	{
		glTexSubImage1D(mSlot, arrayLevel, offsetx, (*mSubResourceInfo)[(arrayLevel * mNumberOfLevels) + mipsLevel].mExtent.width,
			getOpenGLImageFormat(mFormat), GL_UNSIGNED_BYTE, data);
	}
	else
	{
		glTexSubImage3D(mSlot, arrayLevel, offsetx, offsety, offsetz, (*mSubResourceInfo)[(arrayLevel * mNumberOfLevels) + mipsLevel].mExtent.width,
			(*mSubResourceInfo)[(arrayLevel * mNumberOfLevels) + mipsLevel].mExtent.height,
			(*mSubResourceInfo)[(arrayLevel * mNumberOfLevels) + mipsLevel].mExtent.depth,
			getOpenGLImageFormat(mFormat), GL_UNSIGNED_BYTE, data);
	}
}


void OpenGLImage::texImage(const uint32_t arrayLevel, const uint32_t mipsLevel, const void* data)
{
	if (mSlot == GL_TEXTURE_2D)
	{
		glTexImage2D(mSlot, arrayLevel, getOpenGLImageFormat(mFormat), (*mSubResourceInfo)[(arrayLevel * mNumberOfLevels) + mipsLevel].mExtent.width,
			(*mSubResourceInfo)[(arrayLevel * mNumberOfLevels) + mipsLevel].mExtent.height,
			0, getOpenGLImageFormat(mFormat), GL_UNSIGNED_BYTE, data);
	}
	else if (mSlot == GL_TEXTURE_1D)
	{
		glTexImage1D(mSlot, arrayLevel, getOpenGLImageFormat(mFormat), (*mSubResourceInfo)[(arrayLevel * mNumberOfLevels) + mipsLevel].mExtent.width, 0,
			getOpenGLImageFormat(mFormat), GL_UNSIGNED_BYTE, data);
	}
	else
	{
		glTexImage3D(mSlot, arrayLevel, getOpenGLImageFormat(mFormat), (*mSubResourceInfo)[(arrayLevel * mNumberOfLevels) + mipsLevel].mExtent.width,
			(*mSubResourceInfo)[(arrayLevel * mNumberOfLevels) + mipsLevel].mExtent.height,
			(*mSubResourceInfo)[(arrayLevel * mNumberOfLevels) + mipsLevel].mExtent.depth,
			0, getOpenGLImageFormat(mFormat), GL_UNSIGNED_BYTE, data);
	}
}
