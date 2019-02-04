#include "CommandPool.h"
#include "RenderDevice.hpp"

#include <array>
#include <tuple>

CommandPool::CommandPool(RenderDevice* renderDevice) :
	DeviceChild{renderDevice}
{
	std::array<vk::CommandPoolCreateInfo, 3> poolCreateInfos;
	uint8_t queueFamiltTypeIndex = 0;
	for (auto& createInfo : poolCreateInfos)
	{
		createInfo.setQueueFamilyIndex(getDevice()->getQueueFamilyIndex(static_cast<QueueType>(queueFamiltTypeIndex)));
		++queueFamiltTypeIndex;
	}

	mGraphicsPool = getDevice()->createCommandPool(poolCreateInfos[0]);
	mComputePool  = getDevice()->createCommandPool(poolCreateInfos[1]);
	mTransferPool = getDevice()->createCommandPool(poolCreateInfos[2]);
}


CommandPool::~CommandPool()
{
	getDevice()->destroyCommandPool(mGraphicsPool);
	getDevice()->destroyCommandPool(mComputePool);
	getDevice()->destroyCommandPool(mTransferPool);
}


vk::CommandBuffer& CommandPool::getBufferForQueue(const QueueType queueType, const uint32_t index)
{
    auto [bufferPool, commandPool] = [this, queueType]()
	{
		switch (queueType)
		{
		case QueueType::Graphics:
            return std::make_pair<std::vector<vk::CommandBuffer>&, vk::CommandPool&>(mGraphicsBuffers, mGraphicsPool);
		case QueueType::Compute:
            return std::make_pair<std::vector<vk::CommandBuffer>&, vk::CommandPool&>(mComputeBuffers, mComputePool);
		case QueueType::Transfer:
            return std::make_pair<std::vector<vk::CommandBuffer>&, vk::CommandPool&>(mTransferBuffers, mTransferPool);
		}
	}();

	if (bufferPool.size() > (index + 1))
	{
		vk::CommandBufferAllocateInfo info{};
		info.setCommandPool(commandPool);
        info.setLevel(index == 0 ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary);
		info.setCommandBufferCount((index + 1) - bufferPool.size());

		const auto newBuffers = getDevice()->allocateCommandBuffers(info);

		bufferPool.insert(bufferPool.end(), newBuffers.begin(), newBuffers.end());
	}

	return bufferPool[index];
}


uint32_t	CommandPool::getNumberOfBuffersForQueue(const QueueType queueType)
{
    const auto& bufferPool = [this, queueType]()
	{
		switch (queueType)
		{
		case QueueType::Graphics:
			return mGraphicsBuffers;
		case QueueType::Compute:
			return mComputeBuffers;
		case QueueType::Transfer:
			return mTransferBuffers;
        default:
            return mGraphicsBuffers;
		}
	}();

	return bufferPool.size();
}


void	CommandPool::reset()
{
	getDevice()->resetCommandPool(mGraphicsPool);
	getDevice()->resetCommandPool(mComputePool);
	getDevice()->resetCommandPool(mTransferPool);

	mGraphicsBuffers.clear();
	mComputeBuffers.clear();
	mTransferBuffers.clear();
}
