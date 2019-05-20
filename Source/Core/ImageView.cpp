#include "Core/ImageView.hpp"
#include "Core/Image.hpp"
#include "Core/RenderDevice.hpp"


ImageView::ImageView(Image& parentImage,
                     const uint32_t offsetx,
                     const uint32_t offsety,
                     const uint32_t offsetz,
                     const uint32_t lod) :
    // Copy construct the GPUresource/ref count from the parent Image
    GPUResource{parentImage},
    DeviceChild{parentImage.getDevice()},
    mImageHandle{parentImage.getImage()},
    mImageMemory{parentImage.getMemory()},
    mImageFormat{parentImage.getFormat()},
    mOffsetx{offsetx},
    mOffsety{offsety},
    mOffsetz{offsetz},
    mOffsetLOD{lod}
{
    vk::ImageSubresourceRange subresourceRange{};
    subresourceRange.setBaseMipLevel(lod);
    subresourceRange.setLevelCount(1);
    subresourceRange.setBaseArrayLayer(0);
    subresourceRange.setLayerCount(1);
    subresourceRange.setAspectMask(parentImage.getLayout() == vk::ImageLayout::eDepthStencilAttachmentOptimal ?
                                       vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor);

    const auto extent = parentImage.getExtent();
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
    if(!shouldDestroy)
        mImageHandle = nullptr;

    getDevice()->destroyImageView(*this);
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
    mOffsetLOD = otherView.mOffsetLOD;

    return *this;
}

