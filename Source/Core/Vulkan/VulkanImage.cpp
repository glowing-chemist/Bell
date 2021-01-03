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

    mImageMemory = device->getMemoryManager()->Allocate(ImageRequirements.size, ImageRequirements.alignment, false, debugName);

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
	getDevice()->destroyImage(*this);
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

	stagingBuffer.setContents(data, size, 0);

    vk::BufferImageCopy copyInfo{};
    copyInfo.setBufferOffset(0);
    copyInfo.setBufferImageHeight(0);
    copyInfo.setBufferRowLength(0);

	copyInfo.setImageSubresource({vk::ImageAspectFlagBits::eColor, lod, level, 1});

    copyInfo.setImageOffset({offsetx, offsety, offsetz}); // copy to the VulkanImage starting at the start (0, 0, 0)
	ImageExtent ex = getExtent(level, lod);
	vk::Extent3D extent{ ex.width, ex.height, ex.depth };
    copyInfo.setImageExtent(extent);

    auto CmdBuffer = device->getPrefixCommandBuffer();

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

	(*mSubResourceInfo)[(level * mNumberOfMips) + lod].mLayout = ImageLayout::Sampled;

    // Maybe try to implement a staging buffer cache so that we don't have to create one
    // each time.
    stagingBuffer.updateLastAccessed();
    updateLastAccessed();
}


void VulkanImage::clear(const float4& colour)
{
	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    auto CmdBuffer = device->getPrefixCommandBuffer();

	vk::ImageLayout layout = getVulkanImageLayout((*mSubResourceInfo)[0].mLayout);

	// transition to read only optimal
	if (layout != vk::ImageLayout::eTransferDstOptimal)
	{
		vk::ImageMemoryBarrier barrier{};
		barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryRead);
		barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
		barrier.setOldLayout(layout);
		barrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
		barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, mNumberOfMips, 0, mNumberOfLevels });
		barrier.setImage(mImage);

		CmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlagBits::eByRegion, {}, {}, { barrier });

		layout = vk::ImageLayout::eTransferDstOptimal;

		for (auto& resource : *mSubResourceInfo)
		{
			resource.mLayout = ImageLayout::TransferDst;
		}
	}

    vk::ClearColorValue clear{ std::array<float, 4>{colour.x, colour.y, colour.z, colour.w} };
	vk::ImageSubresourceRange subResource{};
	subResource.aspectMask = vk::ImageAspectFlagBits::eColor;
	subResource.baseMipLevel = 0;
	subResource.levelCount = mNumberOfMips;
	subResource.baseArrayLayer = 0;
	subResource.layerCount = mNumberOfLevels;

	CmdBuffer.clearColorImage(mImage, layout, &clear, 1, &subResource);

	updateLastAccessed();
}


void VulkanImage::generateMips()
{
    BELL_ASSERT(mNumberOfLevels == 1, "Image arrays not currently supported")

    VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    auto CmdBuffer = device->getPrefixCommandBuffer();

    vk::ImageLayout srcLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    for(uint32_t i = 1; i < mNumberOfMips; ++i)
    {

        auto& srcSubResource = (*mSubResourceInfo)[i - 1];
        auto& dstSubResource = (*mSubResourceInfo)[i];
        ImageExtent srcExtent = srcSubResource.mExtent;
        ImageExtent dstExtent = dstSubResource.mExtent;

        vk::ImageMemoryBarrier srcBarrier{};
        srcBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        srcBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);
        srcBarrier.setOldLayout(srcLayout);
        srcBarrier.setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
        srcBarrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, i - 1, 1, 0, 1 });
        srcBarrier.setImage(mImage);

        vk::ImageMemoryBarrier dstBarrier{};
        dstBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
        dstBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        dstBarrier.setOldLayout(getVulkanImageLayout(dstSubResource.mLayout));
        dstBarrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
        dstBarrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, i, 1, 0, 1 });
        dstBarrier.setImage(mImage);

        CmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlagBits::eByRegion, {}, {}, { srcBarrier, dstBarrier });
        srcSubResource.mLayout = ImageLayout::TransferSrc;
        dstSubResource.mLayout = ImageLayout::TransferDst;

        vk::ImageBlit blit{};
        blit.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
        blit.srcOffsets[1] = vk::Offset3D{ static_cast<int32_t>(srcExtent.width), static_cast<int32_t>(srcExtent.height), static_cast<int32_t>(srcExtent.depth) };
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
        blit.dstOffsets[1] = vk::Offset3D{ static_cast<int32_t>(dstExtent.width), static_cast<int32_t>(dstExtent.height), static_cast<int32_t>(dstExtent.depth) };
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        CmdBuffer.blitImage(mImage, vk::ImageLayout::eTransferSrcOptimal, mImage, vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

        srcLayout = vk::ImageLayout::eTransferDstOptimal;

        vk::ImageMemoryBarrier barrier{};
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);
        barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal);
        barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, i - 1, 1, 0, 1 });
        barrier.setImage(mImage);

        CmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlagBits::eByRegion, {}, {}, { barrier });

        srcSubResource.mLayout = ImageLayout::Sampled;
    }

    // Transition mip tail.
    vk::ImageMemoryBarrier endBarrier{};
    endBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
    endBarrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
    endBarrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
    endBarrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    endBarrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, mNumberOfMips - 1, 1, 0, 1 });
    endBarrier.setImage(mImage);

    CmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
        vk::DependencyFlagBits::eByRegion, {}, {}, { endBarrier });

    (*mSubResourceInfo)[mNumberOfMips - 1].mLayout = ImageLayout::Sampled;

}

