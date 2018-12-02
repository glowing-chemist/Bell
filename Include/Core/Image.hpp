#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <vulkan/vulkan.hpp>
#include <string>
#include <optional>

#include "Core/GPUResource.hpp"
#include "Core/DeviceChild.hpp"
#include "Core/MemoryManager.hpp"
#include "Core/BarrierManager.hpp"


class RenderDevice;


struct ImageCreationInfo {
    vk::DeviceSize xSize;
    vk::DeviceSize ySize;
    uint32_t mips;
    bool autoGenMips;
    vk::Format format;
    ImageType resourceType;
};


class Image : public GPUResource, public DeviceChild
{
    friend BarrierManager;
public:
    Image(RenderDevice*, const ImageCreationInfo, std::string = "");

    vk::Image       getImage() { return mImage; }
    vk::ImageView   getImageView() { return mImageView; }

    uint32_t        numberOfMips() const { return mNumberOfMips; }

    void            clear();

private:
    Allocation mImageMemory;
    vk::Image mImage;
    vk::ImageView mImageView;
    std::optional<vk::Sampler> mSampler; // only needed when the image is used as a texture

    vk::Format mFormat;
    vk::ImageUsageFlags mUsage;
    uint32_t mNumberOfMips;

    std::string mDebugName;
};

#endif
