#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "Core/MemoryManager.hpp"
#include "Core/GPUResource.hpp"
#include "Engine/PassTypes.hpp"


class Image;
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
    ImageView(ImageView&&) = delete;

    vk::Image getImage() const
        { return mImageHandle; }

    vk::ImageView getImageView() const
        { return mImageViewHandle; }

	Format getImageViewFormat() const
        { return mImageFormat; }

    vk::ImageLayout getImageLayout() const
        { return mLayout; }

    vk::Extent3D getImageExtent() const
        { return mExtent; }

	ImageUsage getImageUsage() const
        { return mUsage; }

    Allocation getImageMemory() const
        { return mImageMemory; }

	uint32_t getBaseMip() const
		{ return mLOD; }

	uint32_t getMipsCount() const
		{ return mLODCount; }

	uint32_t getBaseLevel() const
		{ return mLevel; }

	uint32_t getLevelCount() const
		{ return mLevelCount; }

	ImageViewType getType() const
		{ return mType; }


private:

    vk::Image mImageHandle;
    vk::ImageView mImageViewHandle;
    Allocation mImageMemory;
	ImageViewType mType;

	Format mImageFormat;
    vk::ImageLayout mLayout;
    vk::Extent3D mExtent;
	ImageUsage mUsage;

    uint32_t mLOD;
    uint32_t mLODCount;
    uint32_t mLevel;
	uint32_t mLevelCount;
};

#endif
