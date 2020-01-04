#include "OpenGLImageView.hpp"
#include "OpenGLImage.hpp"


OpenGLImageView::OpenGLImageView(Image& img,
	const ImageViewType type,
	const uint32_t level,
	const uint32_t levelCount,
	const uint32_t lod,
	const uint32_t lodCount) :
	ImageViewBase(img, type, level, levelCount, lod, lodCount),
	mImageHandle(-1)
{
	OpenGLImage* GLImage = static_cast<OpenGLImage*>(img.getBase());
	mImageHandle = GLImage->getImage();
}

