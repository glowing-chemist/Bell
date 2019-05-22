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
          const uint32_t x,
          const uint32_t y,
          const uint32_t z,
          const uint32_t mips = 1,
          const uint32_t levels = 1,
          const uint32_t samples = 1,
		  const std::string& = "");

    Image(RenderDevice*,
          vk::Image&,
          const vk::Format,
          const vk::ImageUsageFlags,
          const uint32_t x,
          const uint32_t y,
          const uint32_t z,
          const uint32_t mips = 1,
          const uint32_t levels = 1,
          const uint32_t samples = 1,
		  const std::string& = ""
          );

    ~Image();

	Image& operator=(const Image&) = delete;
	Image(const Image&) = default;

	Image& operator=(Image&&) = delete;
	Image(Image&&) = delete;

	void swap(Image&); // used instead of a move constructor.

    vk::Image       getImage() { return mImage; }

    void setContents(const void* data,
                     const uint32_t xsize,
                    const uint32_t ysize,
                    const uint32_t zsize,
                    const uint32_t level = 0,
                    const uint32_t lod = 0,
                    const int32_t offsetx = 0,
                    const int32_t offsety = 0,
                    const int32_t offsetz = 0);


    uint32_t        numberOfMips() const { return mNumberOfMips; }
    uint32_t        numberOfLevels() const { return mNumberOfLevels; }
    void            generateMips(const uint32_t);

	vk::Format		getFormat() const
						{ return mFormat; }

    vk::ImageUsageFlags getUsage() const
                           { return mUsage; }

    vk::ImageLayout getLayout(const uint32_t level, const uint32_t LOD) const
                        { return mSubResourceInfo[(level * mNumberOfMips) + LOD].mLayout; }

    vk::Extent3D    getExtent(const uint32_t level, const uint32_t LOD) const
                        { return mSubResourceInfo[(level * mNumberOfMips) + LOD].mExtent; }

    Allocation      getMemory() const
                        { return mImageMemory; }

    void            clear();

private:
    Allocation mImageMemory;
    vk::Image mImage;

    struct SubResourceInfo
    {
        vk::ImageLayout mLayout;
        vk::Extent3D mExtent;
    };
    std::vector<SubResourceInfo> mSubResourceInfo;
    uint32_t mNumberOfMips;
    uint32_t mNumberOfLevels;


    bool mIsOwned;
    vk::Format mFormat;
    vk::ImageUsageFlags mUsage;
    vk::ImageType mType;
    uint32_t mSamples;

    std::string mDebugName;
};

#endif
