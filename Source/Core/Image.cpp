#include "Core/Image.hpp"
#include "RenderDevice.hpp"

Image::Image(RenderDevice* dev,
             const vk::Format format,
             const vk::ImageUsageFlags usage,
             const uint32_t x,
             const uint32_t y,
             const uint32_t z,
             std::string debugName) :
    GPUResource{dev->getCurrentFrameIndex()},
    DeviceChild{dev},
    mFormat{format},
	mLayout{vk::ImageLayout::eUndefined},
    mUsage{usage},
    mNumberOfMips{0},
    mExtent{x, y, z},
    mDebugName{debugName}
{
    if(x != 0 && y == 0 && z == 0) mType = vk::ImageType::e1D;
    if(x != 0 && y != 0 && z == 0) mType = vk::ImageType::e2D;
    if(x != 0 && y != 0 && z != 0) mType = vk::ImageType::e3D;

    mImage = getDevice()->createImage(format, usage, mType, x, y, z);
    vk::MemoryRequirements imageRequirements = getDevice()->getMemoryRequirements(mImage);

    mImageMemory = getDevice()->getMemoryManager()->Allocate(imageRequirements.size, imageRequirements.alignment, false);
    getDevice()->getMemoryManager()->BindImage(mImage, mImageMemory);
}


Image::~Image()
{
    const bool shouldDestroy = release();
    if(shouldDestroy)
        getDevice()->destroyImage(*this);
}


vk::ImageView   Image::createImageView( vk::Format format,
                                        vk::ImageViewType type,
                                        const uint32_t baseMipLevel,
                                        const uint32_t levelCount,
                                        const uint32_t baseArrayLayer,
                                        const uint32_t layerCount)
{
    vk::ImageSubresourceRange subresourceRange{};
    subresourceRange.setBaseMipLevel(baseMipLevel);
    subresourceRange.setLevelCount(levelCount);
    subresourceRange.setBaseArrayLayer(baseArrayLayer);
    subresourceRange.setLayerCount(layerCount);

    vk::ImageViewCreateInfo createInfo{};
    createInfo.setImage(mImage);
    createInfo.setFormat(format);
    createInfo.setSubresourceRange(subresourceRange);
    createInfo.setViewType(type);

    return getDevice()->createImageView(createInfo);
}


vk::Sampler Image::createImageSampler(  vk::Filter magFilter,
                                    vk::Filter minFilter,
                                    vk::SamplerAddressMode u,
                                    vk::SamplerAddressMode v,
                                    vk::SamplerAddressMode w,
                                    bool anisotropyEnable,
                                    uint32_t maxAnisotropy,
                                    vk::BorderColor borderColour,
                                    vk::CompareOp compareOp,
                                    vk::SamplerMipmapMode mipMapMode)
{
    vk::SamplerCreateInfo info{};
    info.setMagFilter(magFilter);
    info.setMinFilter(minFilter);
    info.setAddressModeU(u);
    info.setAddressModeV(v);
    info.setAddressModeW (w);
    info.setAnisotropyEnable(anisotropyEnable);
    info.setMaxAnisotropy(maxAnisotropy);
    info.setBorderColor(borderColour);
    info.setCompareEnable(compareOp != vk::CompareOp::eAlways);
    info.setCompareOp(compareOp);
    info.setMipmapMode(mipMapMode);
    info.setMipLodBias(0.0f);
    info.setMinLod(0.0f);
    info.setMaxLod(0.0f);

    return getDevice()->createSampler(info);
}


void    Image::generateMips(const uint32_t)
{
    // Do with a compute dispatch?
    // possibly allow a custom kernel to be passed in?
}


void    Image::clear()
{
    // TODO wait until the barrier manager is implemented to to this
}
