#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <vulkan/vulkan.hpp>
#include <string>
#include <optional>

#include "Core/GPUResource.hpp"
#include "Core/DeviceChild.hpp"
#include "Core/MemoryManager.hpp"
#include "BarrierManager.hpp"

class RenderDevice;


class Image final : public GPUResource, public DeviceChild
{
	friend BarrierManager;
public:

    Image(RenderDevice*,
          const vk::Format,
          const vk::ImageUsageFlags,
          const uint32_t,
          const uint32_t,
          const uint32_t,
          std::string = "");

    ~Image();

    vk::Image       getImage() { return mImage; }
    vk::ImageView   createImageView(vk::Format,
                                    vk::ImageViewType,
                                    const uint32_t baseMipLevel,
                                    const uint32_t levelCount,
                                    const uint32_t baseArrayLayer,
                                    const uint32_t layerCount);

    uint32_t        numberOfMips() const { return mNumberOfMips; }
    void            generateMips(const uint32_t);

	vk::Format		getFormat() const
						{ return mFormat; }

	vk::ImageLayout getLayout() const
						{ return mLayout; }

    void            clear();

private:
    Allocation mImageMemory;
    vk::Image mImage;
    std::optional<vk::Sampler> mSampler; // only needed when the image is used as a texture

    vk::Format mFormat;
	vk::ImageLayout mLayout;
    vk::ImageUsageFlags mUsage;
    uint32_t mNumberOfMips;
    vk::Extent3D mExtent;
    vk::ImageType mType;

    std::string mDebugName;
};

#endif
