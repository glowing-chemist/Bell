#include "BarrierManager.hpp"
#include "Core/Image.hpp"
#include "Core/Buffer.hpp"
#include "RenderDevice.hpp"

#include <algorithm>


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

	image.setOwningQueueType(queueType);
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

	buffer.setOwningQueueType(queueType);
}


void BarrierManager::transitionImageLayout(Image& image, const vk::ImageLayout layout)
{
	if (layout == image.getLayout())
		return;

	vk::ImageMemoryBarrier barrier{};
	barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	barrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
	barrier.setOldLayout(image.getLayout());
	barrier.setNewLayout(layout);
	barrier.setImage(image.getImage());

	mImageMemoryBarriers.push_back({image.getOwningQueueType(), barrier});

	image.mLayout = layout;
}


void BarrierManager::flushAllBarriers()
{
	auto* commandPool = getDevice()->getCurrentCommandPool();

	std::vector<vk::ImageMemoryBarrier> imageBarriers;
	
	for (uint8_t i = 0; i < static_cast<uint8_t>(QueueType::MaxQueues); ++i)
	{
		const QueueType queueIndex = static_cast<QueueType>(i);

		std::for_each(mImageMemoryBarriers.begin(), mImageMemoryBarriers.end(),
			[&imageBarriers, queueIndex](const std::pair<QueueType, vk::ImageMemoryBarrier>& barrier) { if (barrier.first == queueIndex) imageBarriers.push_back(barrier.second); });


		commandPool->getBufferForQueue(queueIndex).pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eFragmentShader,
			vk::DependencyFlagBits::eByRegion,
			0, nullptr,
			0, nullptr,
			imageBarriers.size(), imageBarriers.data());

		imageBarriers.clear();
	}


	std::vector<vk::BufferMemoryBarrier> bufferBarriers;

	for (uint8_t i = 0; i < static_cast<uint8_t>(QueueType::MaxQueues); ++i)
	{
		const QueueType queueIndex = static_cast<QueueType>(i);

		std::for_each(mBufferMemoryBarriers.begin(), mBufferMemoryBarriers.end(),
			[&bufferBarriers, queueIndex](const std::pair<QueueType, vk::BufferMemoryBarrier>& barrier) { if (barrier.first == queueIndex) bufferBarriers.push_back(barrier.second); });


		commandPool->getBufferForQueue(queueIndex).pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eFragmentShader,
			vk::DependencyFlagBits::eByRegion,
			0, nullptr,
			bufferBarriers.size(), bufferBarriers.data(),
			0, nullptr);

		bufferBarriers.clear();
	}
}