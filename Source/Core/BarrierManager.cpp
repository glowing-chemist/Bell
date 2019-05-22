#include "BarrierManager.hpp"
#include "Core/Image.hpp"
#include "Core/Buffer.hpp"
#include "RenderDevice.hpp"

#include <algorithm>


BarrierRecorder::BarrierRecorder(RenderDevice* device) : DeviceChild(device) {}


void BarrierRecorder::transferResourceToQueue(Image& image, const QueueType queueType)
{
	if (queueType == image.getOwningQueueType())
		return;

	vk::ImageMemoryBarrier releaseBarrier{};
	releaseBarrier.setSrcQueueFamilyIndex(getDevice()->getQueueFamilyIndex(image.getOwningQueueType()));
	releaseBarrier.setDstQueueFamilyIndex(getDevice()->getQueueFamilyIndex(queueType));
	releaseBarrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	releaseBarrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
	releaseBarrier.setImage(image.getImage());

	vk::ImageMemoryBarrier aquireBarrier{};
	aquireBarrier.setSrcQueueFamilyIndex(getDevice()->getQueueFamilyIndex(image.getOwningQueueType()));
	aquireBarrier.setDstQueueFamilyIndex(getDevice()->getQueueFamilyIndex(queueType));
	aquireBarrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	releaseBarrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
	aquireBarrier.setImage(image.getImage());

	mImageMemoryBarriers.push_back({image.getOwningQueueType(), releaseBarrier});
	mImageMemoryBarriers.push_back({ queueType, aquireBarrier });

	image.setOwningQueueType(queueType);
}


void BarrierRecorder::transferResourceToQueue(Buffer& buffer, const QueueType queueType)
{
	if (queueType == buffer.getOwningQueueType())
		return;

	vk::BufferMemoryBarrier releaseBarrier{};
	releaseBarrier.setSrcQueueFamilyIndex(getDevice()->getQueueFamilyIndex(buffer.getOwningQueueType()));
	releaseBarrier.setDstQueueFamilyIndex(getDevice()->getQueueFamilyIndex(queueType));
	releaseBarrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	releaseBarrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
	releaseBarrier.setBuffer(buffer.getBuffer());

	vk::BufferMemoryBarrier aquireBarrier{};
	aquireBarrier.setSrcQueueFamilyIndex(getDevice()->getQueueFamilyIndex(buffer.getOwningQueueType()));
	aquireBarrier.setDstQueueFamilyIndex(getDevice()->getQueueFamilyIndex(queueType));
	aquireBarrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	releaseBarrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
	aquireBarrier.setBuffer(buffer.getBuffer());

	mBufferMemoryBarriers.push_back({ buffer.getOwningQueueType(), releaseBarrier });
	mBufferMemoryBarriers.push_back({ queueType, aquireBarrier });

	buffer.setOwningQueueType(queueType);
}


void BarrierRecorder::transitionImageLayout(Image& image, const vk::ImageLayout layout)
{
    for(uint32_t arrayLevel = 0; arrayLevel < image.numberOfLevels(); ++arrayLevel)
    {
        for(uint32_t mipLevel = 0; mipLevel < image.numberOfMips(); ++mipLevel)
        {
            vk::ImageMemoryBarrier barrier{};
            barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
            barrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
            barrier.setOldLayout(image.getLayout(arrayLevel, mipLevel));
            barrier.setNewLayout(layout);
            barrier.setImage(image.getImage());
            if(layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
                barrier.setSubresourceRange({vk::ImageAspectFlagBits::eDepth, 0, image.numberOfMips(), 0, image.numberOfLevels()});
            } else {
                barrier.setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, image.numberOfMips(), 0, image.numberOfLevels()});
            }

            image.mSubResourceInfo[(arrayLevel * image.numberOfMips()) + mipLevel].mLayout = layout;

            mImageMemoryBarriers.push_back({image.getOwningQueueType(), barrier});
        }
    }
}


void BarrierRecorder::makeContentsVisible(Image& image)
{

    for(uint32_t arrayLevel = 0; arrayLevel < image.numberOfLevels(); ++arrayLevel)
    {
        for(uint32_t mipLevel = 0; mipLevel < image.numberOfMips(); ++mipLevel)
        {
            vk::ImageMemoryBarrier barrier{};
            barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
            barrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
            barrier.setOldLayout(image.getLayout(arrayLevel, mipLevel));
            barrier.setNewLayout(image.getLayout(arrayLevel, mipLevel));
            if(image.getLayout(arrayLevel, mipLevel) == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
                barrier.setSubresourceRange({vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
            } else {
                barrier.setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
            }

            mImageMemoryBarriers.push_back({image.getOwningQueueType(), barrier});
        }
    }
}


void BarrierRecorder::transitionImageLayout(ImageView& imageView, const vk::ImageLayout layout)
{
    if (layout == imageView.getImageLayout())
        return;

    vk::ImageMemoryBarrier barrier{};
    barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
    barrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
    barrier.setOldLayout(imageView.getImageLayout());
    barrier.setNewLayout(layout);
    barrier.setImage(imageView.getImage());
    if(layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.setSubresourceRange({vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
    } else {
        barrier.setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    }

    mImageMemoryBarriers.push_back({imageView.getOwningQueueType(), barrier});

    imageView.mLayout = layout;
}


void BarrierRecorder::makeContentsVisible(ImageView& imageView)
{
    vk::ImageMemoryBarrier barrier{};
    barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
    barrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
    barrier.setOldLayout(imageView.getImageLayout());
    barrier.setNewLayout(imageView.getImageLayout());
    if(imageView.getImageLayout() == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.setSubresourceRange({vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
    } else {
        barrier.setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    }

    mImageMemoryBarriers.push_back({imageView.getOwningQueueType(), barrier});
}


void BarrierRecorder::makeContentsVisible(Buffer& buffer)
{
    vk::BufferMemoryBarrier barrier{};
    barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
    barrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
    barrier.setBuffer(buffer.getBuffer());

    mBufferMemoryBarriers.push_back({buffer.getOwningQueueType(), barrier});
}


std::vector<vk::ImageMemoryBarrier> BarrierRecorder::getImageBarriers(QueueType type)
{
	std::vector<vk::ImageMemoryBarrier> barriers;

	for (auto[queueType, barrier] : mImageMemoryBarriers)
	{
		if (queueType == type)
			barriers.push_back(barrier);
	}

	return barriers;
}


std::vector<vk::BufferMemoryBarrier> BarrierRecorder::getBufferBarriers(QueueType type)
{
	std::vector<vk::BufferMemoryBarrier> barriers;

	for (auto[queueType, barrier] : mBufferMemoryBarriers)
	{
		if (queueType == type)
			barriers.push_back(barrier);
	}

	return barriers;
}
