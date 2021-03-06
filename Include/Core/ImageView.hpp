#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include <memory>

#include "Core/GPUResource.hpp"
#include "Core/DeviceChild.hpp"
#include "Core/Image.hpp"
#include "Engine/PassTypes.hpp"


struct SubResourceInfo;
class VulkanBarrierRecorder;


enum class ImageViewType
{
	Colour,
	Depth,
	CubeMap
};


class ImageViewBase : public GPUResource, public DeviceChild
{
    friend VulkanBarrierRecorder;
public:

	ImageViewBase(Image&,
			  const ImageViewType,
              const uint32_t level = 0,
			  const uint32_t levelCount = 1,
              const uint32_t lod = 0,
              const uint32_t lodCount = 1);

	virtual ~ImageViewBase() = default;

	Format getImageViewFormat() const
        { return mImageFormat; }

	ImageLayout getImageLayout(const uint32_t level = 0, const uint32_t LOD = 0) const;

	ImageExtent getImageExtent(const uint32_t level = 0, const uint32_t LOD = 0) const;
	ImageUsage getImageUsage() const
        { return mUsage; }

	uint32_t getBaseMip() const
	{ return mMipStart; }

	uint32_t getMipsCount() const
	{ return mMipEnd; }

	uint32_t getBaseLevel() const
	{ return mLayerStart; }

	uint32_t getLevelCount() const
	{ return mLayerEnd; }

	ImageViewType getType() const
		{ return mType; }

	uint32_t getTotalSubresourceCount() const
	{
		return mTotalLayers * mTotalMips;
	}

	bool isSwapChain() const
	{ return mIsSwapchain; }

    void updateLastAccessed();

protected:

    ImageViewType mType;

    Format mImageFormat;
    ImageUsage mUsage;

    SubResourceInfo* mSubResourceInfo;

    uint32_t mMipStart;
    uint32_t mMipEnd;
    uint32_t mTotalMips;
    uint32_t mLayerStart;
    uint32_t mLayerEnd;
    uint32_t mTotalLayers;

    bool mIsSwapchain;
};

// Alias array of images (will split this in to a separate class if we need any  more functionality).
using ImageViewArray = std::vector<ImageView>;
using BufferViewArray = std::vector<BufferView>;

class ImageView
{
public:

	ImageView(Image&,
		const ImageViewType,
		const uint32_t level = 0,
		const uint32_t levelCount = 1,
		const uint32_t lod = 0,
		const uint32_t lodCount = 1);
	~ImageView() = default;


	ImageViewBase* getBase()
	{
		return mBase.get();
	}

	const ImageViewBase* getBase() const
	{
		return mBase.get();
	}

	ImageViewBase* operator->()
	{
		return getBase();
	}

	const ImageViewBase* operator->() const
	{
		return getBase();
	}

private:

	std::shared_ptr<ImageViewBase> mBase;

};

#endif
