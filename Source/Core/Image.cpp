#include "Core/Image.hpp"
#include "RenderDevice.hpp"

namespace
{

    uint32_t getPixelSize(const vk::Format format)
    {
		uint32_t result = 4;

        switch(format)
        {
        case vk::Format::eR8G8B8A8Unorm:
			result = 4;
			break;

		case vk::Format::eR32G32B32A32Sfloat:
			result = 16;
			break;

        default:
			result =  4;
        }

		return result;
    }

}

Image::Image(RenderDevice* dev,
             const vk::Format format,
             const vk::ImageUsageFlags usage,
             const uint32_t x,
             const uint32_t y,
             const uint32_t z,
			 const std::string& debugName) :
    GPUResource{dev->getCurrentSubmissionIndex()},
    DeviceChild{dev},
    mIsOwned{true},
    mFormat{format},
	mLayout{vk::ImageLayout::eUndefined},
    mUsage{usage},
    mNumberOfMips{1},
    mExtent{x, y, z},
    mDebugName{debugName}
{
    if(x != 0 && y == 0 && z == 0) mType = vk::ImageType::e1D;
    if(x != 0 && y != 0 && z == 1) mType = vk::ImageType::e2D;
    if(x != 0 && y != 0 && z >  1) mType = vk::ImageType::e3D;

    mImage = getDevice()->createImage(format, usage, mType, x, y, z);
    vk::MemoryRequirements imageRequirements = getDevice()->getMemoryRequirements(mImage);

    mImageMemory = getDevice()->getMemoryManager()->Allocate(imageRequirements.size, imageRequirements.alignment, false);
    getDevice()->getMemoryManager()->BindImage(mImage, mImageMemory);

	if(mDebugName != "")
	{
		getDevice()->setDebugName(mDebugName, reinterpret_cast<uint64_t>(VkImage(mImage)), vk::DebugReportObjectTypeEXT::eImage);
	}
}


Image::Image(RenderDevice* dev,
      vk::Image& image,
      vk::Format format,
      const vk::ImageUsageFlags usage,
      const uint32_t x,
      const uint32_t y,
      const uint32_t z,
	  const std::string& debugName
      )
    :
        GPUResource{dev->getCurrentSubmissionIndex()},
        DeviceChild{dev},
        mImage{image},
        mIsOwned{false},
        mFormat{format},
        mLayout{vk::ImageLayout::eUndefined},
        mUsage{usage},
        mNumberOfMips{1},
        mExtent{x, y, z},
        mDebugName{debugName}
{
    if(x != 0 && y == 0 && z == 0) mType = vk::ImageType::e1D;
    if(x != 0 && y != 0 && z == 1) mType = vk::ImageType::e2D;
    if(x != 0 && y != 0 && z >  1) mType = vk::ImageType::e3D;

	if(mDebugName != "")
	{
		getDevice()->setDebugName(mDebugName, reinterpret_cast<uint64_t>(VkImage(mImage)), vk::DebugReportObjectTypeEXT::eImage);
	}
}


Image::~Image()
{
    const bool shouldDestroy = release() && mIsOwned;

    // Don't add a moved from image to the defered queue.
    if(shouldDestroy && mImage != vk::Image(nullptr))
        getDevice()->destroyImage(*this);
}


Image& Image::operator=(Image&& other)
{
    mImageMemory = other.mImageMemory;
    mImage = other.mImage;
    other.mImage = nullptr;
    mIsOwned = other.mIsOwned;
    mImageView = other.mImageView;
    other.mImageView = nullptr;
    mFormat = other.mFormat;
    mLayout = other.mLayout;
    mUsage = other.mUsage;
    mNumberOfMips = other.mNumberOfMips;
    mExtent = other.mExtent;
    mType = other.mType;

    mDebugName = other.mDebugName;

    return *this;
}


Image::Image(Image&& other) :   GPUResource(other.getDevice()->getCurrentSubmissionIndex()),
                                DeviceChild (other.getDevice())
{
    mImageMemory = other.mImageMemory;
    mImage = other.mImage;
    other.mImage = nullptr;
    mImageView = other.mImageView;
    other.mImageView = nullptr;
    mIsOwned = other.mIsOwned;
    mFormat = other.mFormat;
    mLayout = other.mLayout;
    mUsage = other.mUsage;
    mNumberOfMips = other.mNumberOfMips;
    mExtent = other.mExtent;
    mType = other.mType;

    mDebugName = other.mDebugName;
}


vk::ImageView   Image::createImageView( vk::Format format,
                                        vk::ImageViewType type,
                                        const uint32_t baseMipLevel,
                                        const uint32_t levelCount,
                                        const uint32_t baseArrayLayer,
                                        const uint32_t layerCount)
{
    vk::ImageSubresourceRange subresourceRange{};
    subresourceRange.setBaseMipLevel(baseMipLevel);
    subresourceRange.setLevelCount(levelCount);
    subresourceRange.setBaseArrayLayer(baseArrayLayer);
    subresourceRange.setLayerCount(layerCount);
    subresourceRange.setAspectMask(mLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal ?
                                       vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor);

    vk::ImageViewCreateInfo createInfo{};
    createInfo.setImage(mImage);
    createInfo.setFormat(format);
    createInfo.setSubresourceRange(subresourceRange);
    createInfo.setViewType(type);

    return getDevice()->createImageView(createInfo);
}


void Image::setContents(const void* data,
                        const uint32_t xsize,
                        const uint32_t ysize,
                        const uint32_t zsize,
                        const int32_t offsetx,
                        const int32_t offsety,
                        const int32_t offsetz)
{
    const uint32_t size = xsize * ysize * zsize * getPixelSize(mFormat);
    Buffer stagingBuffer = Buffer(getDevice(), vk::BufferUsageFlagBits::eTransferSrc, size, 1, "Staging Buffer");

	MapInfo mapInfo{};
	mapInfo.mOffset = 0;
	mapInfo.mSize = stagingBuffer.getSize();
	void* mappedBuffer = stagingBuffer.map(mapInfo);
    std::memcpy(mappedBuffer, data, size);
    stagingBuffer.unmap();

    vk::BufferImageCopy copyInfo{};
    copyInfo.setBufferOffset(0);
    copyInfo.setBufferImageHeight(0);
    copyInfo.setBufferRowLength(0);

    copyInfo.setImageSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});

    copyInfo.setImageOffset({offsetx, offsety, offsetz}); // copy to the image starting at the start (0, 0, 0)
    copyInfo.setImageExtent(mExtent);

	{
		// This needs to get recorded before the upload is recorded in to the primrary
		BarrierRecorder preBarrier{ getDevice() };
		preBarrier.transitionImageLayout(*this, vk::ImageLayout::eTransferDstOptimal);
		getDevice()->execute(preBarrier);
	}

    getDevice()->getCurrentCommandPool()
            ->getBufferForQueue(QueueType::Graphics).copyBufferToImage(stagingBuffer.getBuffer(),
                                                                       getImage(),
                                                                       vk::ImageLayout::eTransferDstOptimal,
                                                                       copyInfo);

	{
		// Image will probably be sampled from next, will need to handle seperatly if it will be used to write to
		// with non discard (blending).
		BarrierRecorder postRecorder{ getDevice() };
		postRecorder.transitionImageLayout(*this, vk::ImageLayout::eShaderReadOnlyOptimal);
		getDevice()->execute(postRecorder);
	}

    // Maybe try to implement a staging buffer cache so that we don't have to create one
    // each time.
    stagingBuffer.updateLastAccessed(getDevice()->getCurrentSubmissionIndex());
    updateLastAccessed(getDevice()->getCurrentSubmissionIndex());
}



void    Image::generateMips(const uint32_t)
{
    // Do with a compute dispatch?
    // possibly allow a custom kernel to be passed in?
}


void    Image::clear()
{
    // TODO wait until the barrier manager is implemented to to this
}
