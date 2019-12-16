#include "Core/Image.hpp"
#include "Core/BellLogging.hpp"
#include "RenderDevice.hpp"
#include "Core/ConversionUtils.hpp"


Image::Image(RenderDevice* dev,
			 const Format format,
			 const ImageUsage usage,
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
	BELL_ASSERT(x > 0 && y > 0 && z > 0, "Image must have non zero dimensions")
	BELL_ASSERT(mips > 0 && levels > 0, "Image must have non zero mips and/or levels")

    if(x != 0 && y == 0 && z == 0) mType = vk::ImageType::e1D;
    if(x != 0 && y != 0 && z == 1) mType = vk::ImageType::e2D;
    if(x != 0 && y != 0 && z >  1) mType = vk::ImageType::e3D;

	vk::ImageCreateFlags createFlags = usage & ImageUsage::CubeMap ? vk::ImageCreateFlagBits::eCubeCompatible : vk::ImageCreateFlags{};

	vk::ImageCreateInfo createInfo(createFlags,
                                  mType,
								  getVulkanImageFormat(format),
                                  vk::Extent3D{x, y, z},
                                  mNumberOfMips,
                                  mNumberOfLevels,
                                   static_cast<vk::SampleCountFlagBits>(mSamples),
                                  vk::ImageTiling::eOptimal,
								  getVulkanImageUsage(mUsage),
                                  vk::SharingMode::eExclusive,
                                  0,
                                  nullptr,
                                  vk::ImageLayout::eUndefined);

    mImage = getDevice()->createImage(createInfo);

    vk::MemoryRequirements imageRequirements = getDevice()->getMemoryRequirements(mImage);

    mImageMemory = getDevice()->getMemoryManager()->Allocate(imageRequirements.size, imageRequirements.alignment, false);
    getDevice()->getMemoryManager()->BindImage(mImage, mImageMemory);

	mSubResourceInfo = new std::vector<SubResourceInfo>{};

    for(uint32_t arrayLevel = 0; arrayLevel < mNumberOfLevels; ++arrayLevel)
    {
        vk::Extent3D extent{x, y, z};

        for(uint32_t mipsLevel = 0; mipsLevel < mNumberOfMips; ++mipsLevel)
        {
            SubResourceInfo subInfo{};
            subInfo.mLayout = vk::ImageLayout::eUndefined;
            subInfo.mExtent = extent;

            extent = vk::Extent3D{extent.width / 2, extent.height / 2, extent.depth / 2};

			mSubResourceInfo->push_back(subInfo);
        }
    }

	if(mDebugName != "")
	{
        getDevice()->setDebugName(mDebugName, reinterpret_cast<uint64_t>(VkImage(mImage)), VK_OBJECT_TYPE_IMAGE);
	}
}


Image::Image(RenderDevice* dev,
	  vk::Image& image,
	  Format format,
	  const ImageUsage usage,
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
	BELL_ASSERT(x > 0 && y > 0 && z > 0, "Image must have non zero dimensions")
	BELL_ASSERT(mips > 0 && levels > 0, "Image must have non zero mips and/or levels")

    if(x != 0 && y == 0 && z == 0) mType = vk::ImageType::e1D;
    if(x != 0 && y != 0 && z == 1) mType = vk::ImageType::e2D;
    if(x != 0 && y != 0 && z >  1) mType = vk::ImageType::e3D;

	mSubResourceInfo = new std::vector<SubResourceInfo>{};

    for(uint32_t arrayLevel = 0; arrayLevel < mNumberOfLevels; ++arrayLevel)
    {
        vk::Extent3D extent{x, y, z};

        for(uint32_t mipsLevel = 0; mipsLevel < mNumberOfMips; ++mipsLevel)
        {
            SubResourceInfo subInfo{};
            subInfo.mLayout = vk::ImageLayout::eUndefined;
            subInfo.mExtent = extent;

            extent = vk::Extent3D{extent.width / 2, extent.height / 2, extent.depth / 2};

			mSubResourceInfo->push_back(subInfo);
        }
    }

	if(mDebugName != "")
	{
        getDevice()->setDebugName(mDebugName, reinterpret_cast<uint64_t>(VkImage(mImage)), VK_OBJECT_TYPE_IMAGE);
	}
}


Image::~Image()
{
    const bool shouldDestroy = release() && mIsOwned;

    // Don't add a moved from image to the defered queue.
	if(shouldDestroy && mImage != vk::Image(nullptr))
	{
        getDevice()->destroyImage(*this);
		delete mSubResourceInfo;
	}
}


void Image::swap(Image& other)
{
    const Allocation imageMemory = mImageMemory;
    const vk::Image image = mImage;
    const bool isOwned = mIsOwned;
	const Format Format = mFormat;
	const ImageUsage Usage = mUsage;
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

	std::vector<SubResourceInfo>* temp;
	temp = mSubResourceInfo;
	mSubResourceInfo = other.mSubResourceInfo;
	other.mSubResourceInfo = temp;

	mNumberOfMips = other.mNumberOfMips;
	other.mNumberOfMips = NumberOfMips;

    mNumberOfLevels = other.mNumberOfLevels;
    other.mNumberOfLevels = NumberOfLevels;

	mType = other.mType;
	other.mType = Type;

	mDebugName = other.mDebugName;
	other.mDebugName = DebugName;
}


Image& Image::operator=(Image&& rhs)
{
	swap(rhs);

	return *this;
}


Image::Image(Image&& rhs) :
	GPUResource(rhs),
	DeviceChild(rhs)
{
	mImage = vk::Image{ nullptr };

	swap(rhs);
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
    Buffer stagingBuffer(getDevice(), BufferUsage::TransferSrc, size, 1, "Staging Buffer");

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

	copyInfo.setImageSubresource({vk::ImageAspectFlagBits::eColor, lod, level, 1});

    copyInfo.setImageOffset({offsetx, offsety, offsetz}); // copy to the image starting at the start (0, 0, 0)
    copyInfo.setImageExtent(getExtent(level, lod));

	{
		// This needs to get recorded before the upload is recorded in to the primrary
		BarrierRecorder preBarrier{ getDevice() };
		preBarrier.transitionImageLayout(*this, vk::ImageLayout::eTransferDstOptimal);
		getDevice()->execute(preBarrier, vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer);
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
		getDevice()->execute(postRecorder, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader);
	}

    // Maybe try to implement a staging buffer cache so that we don't have to create one
    // each time.
    stagingBuffer.updateLastAccessed();
    updateLastAccessed();
}


void Image::updateLastAccessed()
{
    GPUResource::updateLastAccessed(getDevice()->getCurrentSubmissionIndex());
}
