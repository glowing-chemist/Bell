#include "Core/ImageView.hpp"
#include "Core/Image.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/ConversionUtils.hpp"


ImageView::ImageView(Image& parentImage,
					 const ImageViewType viewType,
					 const uint32_t level,
					 const uint32_t levelCount,
					 const uint32_t lod,
					 const uint32_t lodCount) :
    GPUResource{parentImage.getDevice()->getCurrentSubmissionIndex()},
    DeviceChild{parentImage.getDevice()},
    mImageHandle{parentImage.getImage()},
    mImageMemory{parentImage.getMemory()},
	mType{viewType},
	mImageFormat{parentImage.getFormat()},
    mUsage{parentImage.getUsage()},
	mSubResourceInfo{nullptr},
    mLOD{lod},
    mLODCount{lodCount},
	mLevel{level},
	mLevelCount{levelCount},
	mIsSwapchain{parentImage.isSwapchainImage()}
{
	const vk::ImageAspectFlags adjustedViewType = [viewType]()
	{
		switch(viewType)
		{
			case ImageViewType::Colour:
				return vk::ImageAspectFlagBits::eColor;

			case ImageViewType::Depth:
				return vk::ImageAspectFlagBits::eDepth;
		}
	}();

	mSubResourceInfo  = &(*parentImage.mSubResourceInfo)[(level * mLODCount) + mLOD];

    vk::ImageSubresourceRange subresourceRange{};
    subresourceRange.setBaseMipLevel(lod);
    subresourceRange.setLevelCount(mLODCount);
    subresourceRange.setBaseArrayLayer(level);
	subresourceRange.setLayerCount(mLevelCount);
	subresourceRange.setAspectMask(adjustedViewType);

    const auto extent = parentImage.getExtent(mLevel, mLOD);

	vk::ImageViewType type = vk::ImageViewType::e1D;

	if(extent.height > 0)
        type = vk::ImageViewType::e2D;

	if(extent.height > 0 && levelCount > 1)
		type = vk::ImageViewType::e2DArray;

    if(extent.depth > 1)
        type = vk::ImageViewType::e3D;

	if(parentImage.getUsage() & ImageUsage::CubeMap)
		type = vk::ImageViewType::eCube;


    vk::ImageViewCreateInfo createInfo{};
    createInfo.setImage(mImageHandle);
	createInfo.setFormat(getVulkanImageFormat(mImageFormat));
    createInfo.setSubresourceRange(subresourceRange);
    createInfo.setViewType(type);

    mImageViewHandle = getDevice()->createImageView(createInfo);
}


ImageView::~ImageView()
{
    const bool shouldDestroy = release();
	if (shouldDestroy && mImageViewHandle != vk::ImageView{nullptr})
    {
        mImageHandle = nullptr;
        getDevice()->destroyImageView(*this);
    }
}


ImageView& ImageView::operator=(const ImageView& otherView)
{
    if(mImageHandle != vk::Image{nullptr})
    {
        const bool shouldDestroy = release();
        if(!shouldDestroy)
            mImageHandle = nullptr;

        getDevice()->destroyImageView(*this);
    }

    GPUResource::operator=(otherView);
    DeviceChild::operator=(otherView);

    mImageHandle = otherView.mImageHandle;
    mImageViewHandle = otherView.mImageViewHandle;
    mImageMemory = otherView.mImageMemory;
    mImageFormat = otherView.mImageFormat;

    mLOD = otherView.mLOD;
    mLODCount = otherView.mLODCount;

    return *this;
}

ImageView::ImageView(ImageView&& rhs) :
	GPUResource{rhs},
	DeviceChild{rhs}
{
	mImageHandle = rhs.mImageHandle;
	mImageViewHandle = rhs.mImageViewHandle;
	mImageMemory = rhs.mImageMemory;
	mType = rhs.mType;

	mImageFormat = rhs.mImageFormat;
	mSubResourceInfo = rhs.mSubResourceInfo;
	mUsage = rhs.mUsage;

	mLOD = rhs.mLOD;
	mLODCount = rhs.mLODCount;
	mLevel = rhs.mLevel;
	mLevelCount = rhs.mLevelCount;

	rhs.mImageViewHandle = vk::ImageView{nullptr};
}


vk::ImageLayout ImageView::getImageLayout(const uint32_t level, const uint32_t LOD) const
{
	return mSubResourceInfo[(level * mLODCount) + LOD].mLayout;
}


vk::Extent3D ImageView::getImageExtent(const uint32_t level, const uint32_t LOD) const
{
	return mSubResourceInfo[(level * mLODCount) + LOD].mExtent;
}
