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
	friend BarrierRecorder;
public:

    Image(RenderDevice*,
          const vk::Format,
          const vk::ImageUsageFlags,
          const uint32_t,
          const uint32_t,
          const uint32_t,
		  const std::string& = "");

    Image(RenderDevice*,
          vk::Image&,
          const vk::Format,
          const vk::ImageUsageFlags,
          const uint32_t,
          const uint32_t,
          const uint32_t,
		  const std::string& = ""
          );

    ~Image();

    Image& operator=(const Image&) = default;
    Image(const Image&) = default;

	Image& operator=(Image&&) = delete;
	Image(Image&&) = delete;

	void swap(Image&); // used instead of a move constructor.

    vk::Image       getImage() { return mImage; }
    vk::ImageView   createImageView(vk::Format,
                                    vk::ImageViewType,
                                    const uint32_t baseMipLevel,
                                    const uint32_t levelCount,
                                    const uint32_t baseArrayLayer,
                                    const uint32_t layerCount);

    void            setCurrentImageView(vk::ImageView& view)
                    { mImageView = view; }

    vk::ImageView&   getCurrentImageView()
                    { return mImageView; }

    const vk::ImageView&   getCurrentImageView() const
                    { return mImageView; }


    void setContents(const void* data,
                     const uint32_t xsize,
                    const uint32_t ysize,
                    const uint32_t zsize,
                    const int32_t offsetx = 0,
                    const int32_t offsety = 0,
                    const int32_t offsetz = 0);


    uint32_t        numberOfMips() const { return mNumberOfMips; }
    void            generateMips(const uint32_t);

	vk::Format		getFormat() const
						{ return mFormat; }

    vk::ImageUsageFlags getUsage() const
                           { return mUsage; }

	vk::ImageLayout getLayout() const
						{ return mLayout; }

    vk::Extent3D    getExtent() const
                        { return mExtent; }

    Allocation      getMemory() const
                        { return mImageMemory; }

    void            clear();

private:
    Allocation mImageMemory;
    vk::Image mImage;
    vk::ImageView mImageView;

    bool mIsOwned;
    vk::Format mFormat;
	vk::ImageLayout mLayout;
    vk::ImageUsageFlags mUsage;
    uint32_t mNumberOfMips;
    vk::Extent3D mExtent;
    vk::ImageType mType;

    std::string mDebugName;
};

#endif
