#ifndef GL_IMAGE_VIEW_HPP
#define GL_IMAGE_VIEW_HPP

#include "Core/Image.hpp"
#include "Core/ImageView.hpp"


class OpenGLImageView : public ImageViewBase
{
public:

	OpenGLImageView(Image&,
		const ImageViewType,
		const uint32_t level = 0,
		const uint32_t levelCount = 1,
		const uint32_t lod = 0,
		const uint32_t lodCount = 1);

	~OpenGLImageView() {}

	uint32_t getImage() const
	{
		return mImageHandle;
	}

private:

	uint32_t mImageHandle;

};

#endif