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
             const uint32_t mips,
             const uint32_t levels,
             const uint32_t samples,
			 const std::string& debugName) :
    GPUResource{dev->getCurrentSubmissionIndex()},
    DeviceChild{dev},
    mNumberOfMips{mips},
    mNumberOfLevels{levels},
    mIsOwned{true},
    mFormat{format},
    mUsage{usage},
    mSamples{samples},
    mDebugName{debugName}
{
    if(x != 0 && y == 0 && z == 0) mType = vk::ImageType::e1D;
    if(x != 0 && y != 0 && z == 1) mType = vk::ImageType::e2D;
    if(x != 0 && y != 0 && z >  1) mType = vk::ImageType::e3D;

    vk::ImageCreateInfo createInfo(vk::ImageCreateFlags{},
                                  mType,
                                  format,
                                  vk::Extent3D{x, y, z},
                                  mNumberOfMips,
                                  mNumberOfLevels,
                                   static_cast<vk::SampleCountFlagBits>(mSamples),
                                  vk::ImageTiling::eOptimal,
                                  mUsage,
                                  vk::SharingMode::eExclusive,
                                  0,
                                  nullptr,
                                  vk::ImageLayout::eUndefined);

    mImage = getDevice()->createImage(createInfo);

    vk::MemoryRequirements imageRequirements = getDevice()->getMemoryRequirements(mImage);

    mImageMemory = getDevice()->getMemoryManager()->Allocate(imageRequirements.size, imageRequirements.alignment, false);
    getDevice()->getMemoryManager()->BindImage(mImage, mImageMemory);

    for(uint32_t arrayLevel = 0; arrayLevel < mNumberOfLevels; ++arrayLevel)
    {
        vk::Extent3D extent{x, y, z};

        for(uint32_t mipsLevel = 0; mipsLevel < mNumberOfMips; ++mipsLevel)
        {
            SubResourceInfo subInfo{};
            subInfo.mLayout = vk::ImageLayout::eUndefined;
            subInfo.mExtent = extent;

            extent = vk::Extent3D{extent.width / 2, extent.height / 2, extent.depth / 2};

            mSubResourceInfo.push_back(subInfo);
        }
    }

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
      const uint32_t mips,
      const uint32_t levels,
      const uint32_t samples,
	  const std::string& debugName
      )
    :
        GPUResource{dev->getCurrentSubmissionIndex()},
        DeviceChild{dev},
        mImage{image},
        mNumberOfMips{mips},
        mNumberOfLevels{levels},
        mIsOwned{false},
        mFormat{format},
        mUsage{usage},
        mSamples{samples},
        mDebugName{debugName}
{
    if(x != 0 && y == 0 && z == 0) mType = vk::ImageType::e1D;
    if(x != 0 && y != 0 && z == 1) mType = vk::ImageType::e2D;
    if(x != 0 && y != 0 && z >  1) mType = vk::ImageType::e3D;

    for(uint32_t arrayLevel = 0; arrayLevel < mNumberOfLevels; ++arrayLevel)
    {
        vk::Extent3D extent{x, y, z};

        for(uint32_t mipsLevel = 0; mipsLevel < mNumberOfMips; ++mipsLevel)
        {
            SubResourceInfo subInfo{};
            subInfo.mLayout = vk::ImageLayout::eUndefined;
            subInfo.mExtent = extent;

            extent = vk::Extent3D{extent.width / 2, extent.height / 2, extent.depth / 2};

            mSubResourceInfo.push_back(subInfo);
        }
    }

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


void Image::swap(Image& other)
{
    const Allocation imageMemory = mImageMemory;
    const vk::Image image = mImage;
    const bool isOwned = mIsOwned;
    const vk::Format Format = mFormat;
    const vk::ImageUsageFlags Usage = mUsage;
    const uint32_t NumberOfMips = mNumberOfMips;
    const uint32_t NumberOfLevels = mNumberOfLevels;
    const vk::ImageType Type = mType;
    const std::string DebugName = mDebugName;


	mImageMemory = other.mImageMemory;
	other.mImageMemory = imageMemory;

	mImage = other.mImage;
	other.mImage = image;

	mIsOwned = other.mIsOwned;
	other.mIsOwned = isOwned;

	mFormat = other.mFormat;
	other.mFormat = Format;

	mUsage = other.mUsage;
	other.mUsage = Usage;

    mSubResourceInfo.swap(other.mSubResourceInfo);

	mNumberOfMips = other.mNumberOfMips;
	other.mNumberOfMips = NumberOfMips;

    mNumberOfLevels = other.mNumberOfLevels;
    other.mNumberOfLevels = NumberOfLevels;

	mType = other.mType;
	other.mType = Type;

	mDebugName = other.mDebugName;
	other.mDebugName = DebugName;
}


void Image::setContents(const void* data,
                        const uint32_t xsize,
                        const uint32_t ysize,
                        const uint32_t zsize,
                        const uint32_t level,
                        const uint32_t lod,
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
    copyInfo.setImageExtent(getExtent(level, lod));

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
