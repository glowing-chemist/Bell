#ifndef GL_IMAGE_HPP
#define GL_IMAGE_HPP

#include "Core/Image.hpp"


class OpenGLImage : public ImageBase
{
public:

	OpenGLImage(RenderDevice*,
		const Format,
		const ImageUsage,
		const uint32_t x,
		const uint32_t y,
		const uint32_t z,
		const uint32_t mips = 1,
		const uint32_t levels = 1,
		const uint32_t samples = 1,
		const std::string & = "");

	~OpenGLImage();

	virtual void swap(ImageBase&) override;

	virtual void setContents(const void* data,
		const uint32_t xsize,
		const uint32_t ysize,
		const uint32_t zsize,
		const uint32_t level = 0,
		const uint32_t lod = 0,
		const int32_t offsetx = 0,
		const int32_t offsety = 0,
		const int32_t offsetz = 0) override;

	virtual void clear() override;

	uint32_t getImage() const
	{
		return mImage;
	}

private:

	void texImage(const uint32_t level, const uint32_t mip, const void* data);

	uint32_t mImage;
	uint32_t mSlot;

};

#endif