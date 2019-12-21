#include "Core/ImageView.hpp"
#include "Core/Image.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/ConversionUtils.hpp"


ImageView::ImageView(Image& parentImage,
					 const ImageViewType viewType,
					 const uint32_t level,
					 const uint32_t levelCount,
					 const uint32_t mip,
					 const uint32_t mipCount) :
    GPUResource{parentImage.getDevice()->getCurrentSubmissionIndex()},
    DeviceChild{parentImage.getDevice()},
    mImageHandle{parentImage.getImage()},
    mImageMemory{parentImage.getMemory()},
	mType{viewType},
	mImageFormat{parentImage.getFormat()},
    mUsage{parentImage.getUsage()},
	mSubResourceInfo{nullptr},
	mMipStart{mip},
	mMipEnd{mipCount},
	mTotalMips{parentImage.numberOfMips()},
	mLayerStart{level},
	mLayerEnd{levelCount},
	mTotalLayers{parentImage.numberOfLevels()},
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

	mSubResourceInfo  = parentImage.mSubResourceInfo->data();

    vk::ImageSubresourceRange subresourceRange{};
	subresourceRange.setBaseMipLevel(mip);
	subresourceRange.setLevelCount(mipCount);
    subresourceRange.setBaseArrayLayer(level);
	subresourceRange.setLayerCount(mLayerEnd);
	subresourceRange.setAspectMask(adjustedViewType);

	const auto extent = parentImage.getExtent(mLayerStart, mMipStart);

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

	mMipStart = otherView.mMipStart;
	mMipEnd = otherView.mMipEnd;
	mTotalMips = otherView.mTotalMips;

	mLayerStart = otherView.mLayerStart;
	mLayerEnd = otherView.mLayerEnd;
	mTotalLayers = otherView.mTotalLayers;

	mIsSwapchain = otherView.isSwapChain();

    return *this;
}

ImageView::ImageView(ImageView&& otherView) :
	GPUResource{otherView},
	DeviceChild{otherView}
{
	mImageHandle = otherView.mImageHandle;
	mImageViewHandle = otherView.mImageViewHandle;
	mImageMemory = otherView.mImageMemory;
	mType = otherView.mType;

	mImageFormat = otherView.mImageFormat;
	mSubResourceInfo = otherView.mSubResourceInfo;
	mUsage = otherView.mUsage;

	mMipStart = otherView.mMipStart;
	mMipEnd = otherView.mMipEnd;
	mTotalMips = otherView.mTotalMips;

	mLayerStart = otherView.mLayerStart;
	mLayerEnd = otherView.mLayerEnd;
	mTotalLayers = otherView.mTotalLayers;

	mIsSwapchain = otherView.mIsSwapchain;

	otherView.mImageViewHandle = vk::ImageView{nullptr};
}


ImageLayout ImageView::getImageLayout(const uint32_t level, const uint32_t LOD) const
{
	return mSubResourceInfo[(level * mTotalMips) + LOD].mLayout;
}


vk::Extent3D ImageView::getImageExtent(const uint32_t level, const uint32_t LOD) const
{
	return mSubResourceInfo[(level * mTotalMips) + LOD].mExtent;
}


void ImageView::updateLastAccessed()
{
	GPUResource::updateLastAccessed(getDevice()->getCurrentSubmissionIndex());
}
