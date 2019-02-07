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

    vk::Sampler    createImageSampler(  vk::Filter magFilter,
                                        vk::Filter minFilter,
                                        vk::SamplerAddressMode u,
                                        vk::SamplerAddressMode v,
                                        vk::SamplerAddressMode w,
                                        bool anisotropyEnable,
                                        uint32_t maxAnisotropy,
                                        vk::BorderColor borderColour,
                                        vk::CompareOp compareOp,
                                        vk::SamplerMipmapMode mipMapMode);

    void           setCurrentSampler(vk::Sampler& sampler)
                    { mCurrentSampler = sampler;}
    const vk::Sampler&    getCurrentSampler() const
                    { return mCurrentSampler; }
    void            setCurrentImageView(vk::ImageView& view)
                    { mImageView = view; }
    const vk::ImageView&   getCurrentImageView() const
                    { return mImageView; }

    uint32_t        numberOfMips() const { return mNumberOfMips; }
    void            generateMips(const uint32_t);

	vk::Format		getFormat() const
						{ return mFormat; }

	vk::ImageLayout getLayout() const
						{ return mLayout; }

    vk::Extent3D    getExtent() const
                        { return mExtent; }

    void            clear();

private:
    Allocation mImageMemory;
    vk::Image mImage;
    vk::ImageView mImageView;
    vk::Sampler mCurrentSampler;

    vk::Format mFormat;
	vk::ImageLayout mLayout;
    vk::ImageUsageFlags mUsage;
    uint32_t mNumberOfMips;
    vk::Extent3D mExtent;
    vk::ImageType mType;

    std::string mDebugName;
};

#endif
