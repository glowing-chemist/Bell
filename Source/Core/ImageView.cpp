#include "Core/ImageView.hpp"
#include "Core/Image.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/ConversionUtils.hpp"

#ifdef VULKAN
#include "Core/Vulkan/VulkanImageView.hpp"
#endif

#ifdef DX_12
#include "Core/DX_12/DX_12ImageView.hpp"
#endif


ImageViewBase::ImageViewBase(Image& parentImage,
					 const ImageViewType viewType,
					 const uint32_t level,
					 const uint32_t levelCount,
					 const uint32_t mip,
					 const uint32_t mipCount) :
    GPUResource{parentImage->getDevice()->getCurrentSubmissionIndex()},
    DeviceChild{parentImage->getDevice()},
	mType{viewType},
	mImageFormat{parentImage->getFormat()},
    mUsage{parentImage->getUsage()},
	mSubResourceInfo{nullptr},
	mMipStart{mip},
	mMipEnd{mipCount},
	mTotalMips{parentImage->numberOfMips()},
	mLayerStart{level},
	mLayerEnd{levelCount},
	mTotalLayers{parentImage->numberOfLevels()},
	mIsSwapchain{parentImage->isSwapchainImage()}
{
	mSubResourceInfo  = parentImage->mSubResourceInfo->data();
}


ImageLayout ImageViewBase::getImageLayout(const uint32_t level, const uint32_t LOD) const
{
	return mSubResourceInfo[(level * mTotalMips) + LOD].mLayout;
}


ImageExtent ImageViewBase::getImageExtent(const uint32_t level, const uint32_t LOD) const
{
	return mSubResourceInfo[(level * mTotalMips) + LOD].mExtent;
}


void ImageViewBase::updateLastAccessed()
{
	GPUResource::updateLastAccessed(getDevice()->getCurrentSubmissionIndex());
}


ImageView::ImageView(Image& image,
	const ImageViewType type,
	const uint32_t level,
	const uint32_t levelCount,
	const uint32_t lod ,
	const uint32_t lodCount)
{
#ifdef VULKAN
	mBase = std::make_shared<VulkanImageView>(image, type, level, levelCount, lod, lodCount);
#endif

#ifdef DX_12
	mBase = std::make_shared<DX_12ImageView>(image, type, level, levelCount, lod, lodCount);
#endif
}