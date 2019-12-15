#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "Core/MemoryManager.hpp"
#include "Core/GPUResource.hpp"
#include "Engine/PassTypes.hpp"


class Image;
class SubResourceInfo;
class BarrierRecorder;


enum class ImageViewType
{
	Colour,
	Depth
};


class ImageView : public GPUResource, public DeviceChild
{
    friend BarrierRecorder;
public:

    ImageView(Image&,
			  const ImageViewType,
              const uint32_t level = 0,
			  const uint32_t levelCount = 1,
              const uint32_t lod = 0,
              const uint32_t lodCount = 1);

    ~ImageView();

    // ImageViews are copy only.
    ImageView& operator=(const ImageView&);
    ImageView(const ImageView&) = default;

    ImageView& operator=(ImageView&&) = delete;
    ImageView(ImageView&&);

    vk::Image getImage() const
        { return mImageHandle; }

    vk::ImageView getImageView() const
        { return mImageViewHandle; }

	Format getImageViewFormat() const
        { return mImageFormat; }

	vk::ImageLayout getImageLayout(const uint32_t level = 0, const uint32_t LOD = 0) const;

	vk::Extent3D getImageExtent(const uint32_t level = 0, const uint32_t LOD = 0) const;
	ImageUsage getImageUsage() const
        { return mUsage; }

    Allocation getImageMemory() const
        { return mImageMemory; }

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

	bool isSwapChain() const
	{ return mIsSwapchain; }

    void updateLastAccessed();

private:

    vk::Image mImageHandle;
    vk::ImageView mImageViewHandle;
    Allocation mImageMemory;
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

#endif
