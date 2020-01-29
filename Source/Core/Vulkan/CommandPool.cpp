#include "CommandPool.h"
#include "VulkanRenderDevice.hpp"

#include <array>
#include <functional>
#include <tuple>

CommandPool::CommandPool(RenderDevice* renderDevice) :
    DeviceChild{renderDevice},
    mGraphicsBuffers{},
    mComputeBuffers{},
    mTransferBuffers{}
{
	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    std::array<vk::CommandPoolCreateInfo, 3> poolCreateInfos{};
	uint8_t queueFamiltTypeIndex = 0;
	for (auto& createInfo : poolCreateInfos)
	{
		createInfo.setQueueFamilyIndex(device->getQueueFamilyIndex(static_cast<QueueType>(queueFamiltTypeIndex)));
		// we reset the pools every 3 frames so set these as transient.
        createInfo.setFlags(vk::CommandPoolCreateFlagBits::eTransient);
		++queueFamiltTypeIndex;
	}

	mGraphicsPool = device->createCommandPool(poolCreateInfos[0]);
	mComputePool  = device->createCommandPool(poolCreateInfos[1]);
	mTransferPool = device->createCommandPool(poolCreateInfos[2]);

    reset();
}


CommandPool::~CommandPool()
{
    reset();

	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

	device->destroyCommandPool(mGraphicsPool);
    device->destroyCommandPool(mComputePool);
    device->destroyCommandPool(mTransferPool);
}


vk::CommandBuffer& CommandPool::getBufferForQueue(const QueueType queueType, const uint32_t index)
{
    auto& bufferPool = getCommandBuffers(queueType);

    if (bufferPool.size() < (index + 1))
	{
        uint32_t needed_buffers = (index + 1) - bufferPool.size();
        reserve(needed_buffers, queueType);
	}

	return bufferPool[index];
}


uint32_t CommandPool::getNumberOfBuffersForQueue(const QueueType queueType)
{
    const auto& bufferPool = getCommandBuffers(queueType);

    return static_cast<uint32_t>(bufferPool.size());
}


void CommandPool::reserve(const uint32_t number, const QueueType queueType)
{
    const int32_t existingBuffers = getNumberOfBuffersForQueue(queueType);
    int32_t needed = int32_t(number) - existingBuffers;

    if(needed <= 0)
        return;

    std::vector<vk::CommandBuffer>& commandBuffers = getCommandBuffers(queueType);
    if(existingBuffers == 0)
    {
        --needed;
        auto commandBuffer = allocateCommandBuffers(1, queueType, true)[0];

        vk::CommandBufferBeginInfo primaryBegin{};
        primaryBegin.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        commandBuffer.begin(primaryBegin);

        commandBuffers.push_back(commandBuffer);
    }

    if(needed == 0)
        return;

    std::vector<vk::CommandBuffer> newBuffers = allocateCommandBuffers(needed, queueType, false);
    commandBuffers.insert(commandBuffers.end(), newBuffers.begin(), newBuffers.end());
}


void CommandPool::reset()
{
	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

	device->resetCommandPool(mGraphicsPool);
	device->resetCommandPool(mComputePool);
	device->resetCommandPool(mTransferPool);

	mGraphicsBuffers.clear();
	mComputeBuffers.clear();
	mTransferBuffers.clear();
}


std::vector<vk::CommandBuffer> CommandPool::allocateCommandBuffers(const uint32_t number, const QueueType queueType, const bool primaryNeeded)
{
    vk::CommandBufferAllocateInfo info{};
    info.setCommandPool(getCommandPool(queueType));
    info.setCommandBufferCount(number);
    info.setLevel(primaryNeeded ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary);

    return static_cast<VulkanRenderDevice*>(getDevice())->allocateCommandBuffers(info);
}


const vk::CommandPool& CommandPool::getCommandPool(const QueueType queueType) const
{
    const auto& bufferPool = [this, queueType]() -> const vk::CommandPool&
    {
        switch (queueType)
        {
        case QueueType::Graphics:
            return mGraphicsPool;
        case QueueType::Compute:
            return mComputePool;
        case QueueType::Transfer:
            return mTransferPool;
        default:
            return mGraphicsPool;
        }
    }();

    return bufferPool;
}


std::vector<vk::CommandBuffer>& CommandPool::getCommandBuffers(const QueueType queueType)
{
    auto& bufferPool = [this, queueType]() -> std::vector<vk::CommandBuffer>&
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

    return bufferPool;
}


