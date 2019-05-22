#include "Core/ImageView.hpp"
#include "Core/Image.hpp"
#include "Core/RenderDevice.hpp"


ImageView::ImageView(Image& parentImage,
                     const uint32_t offsetx,
                     const uint32_t offsety,
                     const uint32_t offsetz,
                     const uint32_t level,
                     const uint32_t lod) :
    GPUResource{parentImage.getDevice()->getCurrentSubmissionIndex()},
    DeviceChild{parentImage.getDevice()},
    mImageHandle{parentImage.getImage()},
    mImageMemory{parentImage.getMemory()},
    mImageFormat{parentImage.getFormat()},
    mLayout{parentImage.getLayout(level, lod)},
    mExtent{parentImage.getExtent(level, lod)},
    mUsage{parentImage.getUsage()},
    mOffsetx{offsetx},
    mOffsety{offsety},
    mOffsetz{offsetz},
    mLOD{lod},
    mLevel{level}
{
    vk::ImageSubresourceRange subresourceRange{};
    subresourceRange.setBaseMipLevel(lod);
    subresourceRange.setLevelCount(1);
    subresourceRange.setBaseArrayLayer(level);
    subresourceRange.setLayerCount(1);
    subresourceRange.setAspectMask(parentImage.getLayout(mLevel, mLOD) == vk::ImageLayout::eDepthStencilAttachmentOptimal ?
                                       vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor);

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
    // This is the last reference on the image so destroy ourselves and the image.
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

    mOffsetx = otherView.mOffsetx;
    mOffsety = otherView.mOffsety;
    mOffsetz = otherView.mOffsetz;
    mLOD = otherView.mLOD;

    return *this;
}

