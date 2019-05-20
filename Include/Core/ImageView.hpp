#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "Core/MemoryManager.hpp"
#include "Core/GPUResource.hpp"


class Image;


class ImageView : public GPUResource, public DeviceChild
{
public:

    ImageView(Image&,
              const uint32_t offsetx = 0,
              const uint32_t offsety = 0,
              const uint32_t offsetz = 0,
              const uint32_t lod = 0);

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

    vk::Format getImageViewFormat() const
        { return mImageFormat; }

    Allocation getImageMemory() const
        { return mImageMemory; }


private:

    vk::Image mImageHandle;
    vk::ImageView mImageViewHandle;
    Allocation mImageMemory;

    vk::Format mImageFormat;

    uint32_t mOffsetx;
    uint32_t mOffsety;
    uint32_t mOffsetz;
    uint32_t mOffsetLOD;
};

#endif
