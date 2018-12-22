#include "BarrierManager.hpp"
#include "Core/Image.hpp"
#include "Core/Buffer.hpp"
#include "RenderDevice.hpp"

BarrierManager::BarrierManager(RenderDevice* device) : DeviceChild(device) {}


void BarrierManager::transferResourceToQueue(Image& image, const QueueType queueType)
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
}


void BarrierManager::transferResourceToQueue(Buffer& buffer, const QueueType queueType)
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
}