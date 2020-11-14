#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <memory>
#include <string>
#include <optional>

#include "Core/GPUResource.hpp"
#include "Core/DeviceChild.hpp"
#include "Core/BarrierManager.hpp"

#include "Engine/PassTypes.hpp"
#include "Engine/GeomUtils.h"


class RenderDevice;
class ImageViewBase;
class VulkanBarrierRecorder;

struct ImageExtent
{
	uint32_t width;
	uint32_t height;
	uint32_t depth;
};

struct SubResourceInfo
{
	ImageLayout mLayout;
	ImageExtent mExtent;
};


class ImageBase : public GPUResource, public DeviceChild 
{
	friend VulkanBarrierRecorder;
	friend ImageViewBase;
public:

	ImageBase(RenderDevice*,
		  const Format,
		  const ImageUsage,
          const uint32_t x,
          const uint32_t y,
          const uint32_t z,
          const uint32_t mips = 1,
          const uint32_t levels = 1,
          const uint32_t samples = 1,
		  const std::string& = "");

    virtual ~ImageBase();

	virtual void swap(ImageBase&); // used instead of a move constructor.

    virtual void setContents(const void* data,
                     const uint32_t xsize,
                    const uint32_t ysize,
                    const uint32_t zsize,
                    const uint32_t level = 0,
                    const uint32_t lod = 0,
                    const int32_t offsetx = 0,
                    const int32_t offsety = 0,
                    const int32_t offsetz = 0) = 0;

	// Must be called outside of a renderpass (will execuet at the beginning of the frame).
    virtual void clear(const float4&) = 0;

    virtual void generateMips() = 0;

	virtual void updateLastAccessed();

    uint32_t        numberOfMips() const { return mNumberOfMips; }
    uint32_t        numberOfLevels() const { return mNumberOfLevels; }

	Format		getFormat() const
						{ return mFormat; }

	ImageUsage	getUsage() const
						{ return mUsage; }

    ImageLayout getLayout(const uint32_t level, const uint32_t LOD) const
						{ return (*mSubResourceInfo)[(level * mNumberOfMips) + LOD].mLayout; }

	ImageExtent    getExtent(const uint32_t level, const uint32_t LOD) const
						{ return (*mSubResourceInfo)[(level * mNumberOfMips) + LOD].mExtent; }


	bool			isSwapchainImage() const
	{ return !mIsOwned; }

protected:

	std::vector<SubResourceInfo>* mSubResourceInfo;
    uint32_t mNumberOfMips;
    uint32_t mNumberOfLevels;


    bool mIsOwned;
	Format mFormat;
	ImageUsage mUsage;
    uint32_t mSamples;

    std::string mDebugName;
};

class Image
{
public:

	Image(RenderDevice*,
		const Format,
		const ImageUsage,
		const uint32_t x,
		const uint32_t y,
		const uint32_t z,
		const uint32_t mips = 1,
		const uint32_t levels = 1,
		const uint32_t samples = 1,
		const std::string & = "");

	Image(ImageBase* base) :
		mBase{ base } {}

	~Image() = default;


	ImageBase* getBase()
	{
		return mBase.get();
	}

	const ImageBase* getBase() const
	{
		return mBase.get();
	}

	ImageBase* operator->()
	{
		return getBase();
	}

	const ImageBase* operator->() const
	{
		return getBase();
	}

private:

	std::shared_ptr<ImageBase> mBase;
};

#endif
