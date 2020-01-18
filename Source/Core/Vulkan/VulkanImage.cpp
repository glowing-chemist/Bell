#include "VulkanImage.hpp"
#include "Core/BellLogging.hpp"
#include "VulkanRenderDevice.hpp"
#include "VulkanBarrierManager.hpp"
#include "Core/ConversionUtils.hpp"


VulkanImage::VulkanImage(RenderDevice* dev,
			 const Format format,
			 const ImageUsage usage,
             const uint32_t x,
             const uint32_t y,
             const uint32_t z,
             const uint32_t mips,
             const uint32_t levels,
             const uint32_t samples,
			 const std::string& debugName) :
	ImageBase(dev, format, usage, x, y, z, mips, levels, samples, debugName)
{
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

	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    mImage = device->createImage(createInfo);

    vk::MemoryRequirements ImageRequirements = device->getMemoryRequirements(mImage);

	if(usage & ImageUsage::Transient)
		mImageMemory = device->getMemoryManager()->allocateTransient(ImageRequirements.size, ImageRequirements.alignment, false);
	else
		mImageMemory = device->getMemoryManager()->Allocate(ImageRequirements.size, ImageRequirements.alignment, false);

	device->getMemoryManager()->BindImage(mImage, mImageMemory);


	if(mDebugName != "")
	{
		device->setDebugName(mDebugName, reinterpret_cast<uint64_t>(VkImage(mImage)), VK_OBJECT_TYPE_IMAGE);
	}
}


VulkanImage::VulkanImage(RenderDevice* dev,
	  vk::Image& image,
	  Format format,
	  const ImageUsage usage,
      const uint32_t x,
      const uint32_t y,
      const uint32_t z,
      const uint32_t mips,
      const uint32_t levels,
      const uint32_t samples,
	  const std::string& debugName) :
		ImageBase(dev, format, usage, x, y, z, mips, levels, samples, debugName),
		mImage{image},
        mIsOwned{false}
{
    if(x != 0 && y == 0 && z == 0) mType = vk::ImageType::e1D;
    if(x != 0 && y != 0 && z == 1) mType = vk::ImageType::e2D;
    if(x != 0 && y != 0 && z >  1) mType = vk::ImageType::e3D;

	if(mDebugName != "")
	{
        getDevice()->setDebugName(mDebugName, reinterpret_cast<uint64_t>(VkImage(mImage)), VK_OBJECT_TYPE_IMAGE);
	}
}


VulkanImage::~VulkanImage()
{
	static_cast<VulkanRenderDevice*>(getDevice())->destroyImage(*this);
}


void VulkanImage::swap(ImageBase& other)
{
	VulkanImage& VKOther = static_cast<VulkanImage&>(other);

    const Allocation VulkanImageMemory = mImageMemory;
    const vk::Image VulkanImage = mImage;
    const bool isOwned = mIsOwned;
    const vk::ImageType Type = mType;

	ImageBase::swap(other);

	mImageMemory = VKOther.mImageMemory;
	VKOther.mImageMemory = VulkanImageMemory;

	mImage = VKOther.mImage;
	VKOther.mImage = VulkanImage;

    mType = VKOther.mType;
    VKOther.mType = Type;

	mIsOwned = VKOther.mIsOwned;
	VKOther.mIsOwned = isOwned;
}


void VulkanImage::setContents(const void* data,
                        const uint32_t xsize,
                        const uint32_t ysize,
                        const uint32_t zsize,
                        const uint32_t level,
                        const uint32_t lod,
                        const int32_t offsetx,
                        const int32_t offsety,
                        const int32_t offsetz)
{
	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    const uint32_t size = xsize * ysize * zsize * getPixelSize(mFormat);
    VulkanBuffer stagingBuffer(getDevice(), BufferUsage::TransferSrc, size, 1, "Staging Buffer");

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

    copyInfo.setImageOffset({offsetx, offsety, offsetz}); // copy to the VulkanImage starting at the start (0, 0, 0)
	ImageExtent ex = getExtent(level, lod);
	vk::Extent3D extent{ ex.width, ex.height, ex.depth };
    copyInfo.setImageExtent(extent);

	auto CmdBuffer = device->getCurrentCommandPool()
		->getBufferForQueue(QueueType::Graphics);

	{
		vk::ImageMemoryBarrier barrier{};
		barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryRead);
		barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
		barrier.setOldLayout(getVulkanImageLayout(getLayout(level, lod)));
		barrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
		barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, lod, 1, level, 1 });
		barrier.setImage(mImage);


		CmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlagBits::eByRegion, {}, {}, { barrier });
	}

		CmdBuffer.copyBufferToImage(stagingBuffer.getBuffer(),
                                    getImage(),
                                    vk::ImageLayout::eTransferDstOptimal,
                                    copyInfo);

	{
		vk::ImageMemoryBarrier barrier{};
		barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
		barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
		barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
		barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
		barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, lod, 1, level, 1 });
		barrier.setImage(mImage);


		CmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexShader,
			vk::DependencyFlagBits::eByRegion, {}, {}, { barrier });
	}

    // Maybe try to implement a staging buffer cache so that we don't have to create one
    // each time.
    stagingBuffer.updateLastAccessed();
    updateLastAccessed();
}

