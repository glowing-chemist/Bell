#include "Core/ImageView.hpp"
#include "Core/Image.hpp"
#include "Core/RenderDevice.hpp"


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
    mLayout{parentImage.getLayout(level, lod)},
    mExtent{parentImage.getExtent(level, lod)},
    mUsage{parentImage.getUsage()},
    mLOD{lod},
    mLODCount{lodCount},
	mLevel{level},
	mLevelCount{levelCount}
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
    if(extent.depth > 1)
        type = vk::ImageViewType::e3D;

    vk::ImageViewCreateInfo createInfo{};
    createInfo.setImage(mImageHandle);
    createInfo.setFormat(mImageFormat);
    createInfo.setSubresourceRange(subresourceRange);
    createInfo.setViewType(type);

    mImageViewHandle = getDevice()->createImageView(createInfo);
}


ImageView::~ImageView()
{
    const bool shouldDestroy = release();
    if(shouldDestroy)
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

