#include "CommandPool.h"
#include "VulkanRenderDevice.hpp"

#include <array>
#include <functional>
#include <tuple>

CommandPool::CommandPool(RenderDevice* renderDevice, const QueueType queueType) :
    DeviceChild{renderDevice},
    mCmdBuffers{}
{
	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    vk::CommandPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.setQueueFamilyIndex(device->getQueueFamilyIndex(queueType));
    poolCreateInfo.setFlags(vk::CommandPoolCreateFlagBits::eTransient);

    mPool = device->createCommandPool(poolCreateInfo);
    reset();
}


CommandPool::~CommandPool()
{
    reset();

	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    device->destroyCommandPool(mPool);
}


vk::CommandBuffer& CommandPool::getBufferForQueue(const uint32_t index)
{
    auto& bufferPool = getCommandBuffers();

    if (bufferPool.size() < (index + 1))
	{
        uint32_t needed_buffers = (index + 1) - bufferPool.size();
        reserve(needed_buffers);
	}

    BELL_ASSERT(index < bufferPool.size(), "Invalid command buffer index")
	return bufferPool[index];
}


uint32_t CommandPool::getNumberOfBuffersForQueue()
{
    return static_cast<uint32_t>(mCmdBuffers.size());
}


void CommandPool::reserve(const uint32_t number)
{
    const int32_t existingBuffers = getNumberOfBuffersForQueue();
    int32_t needed = int32_t(number) - existingBuffers;

    if(needed <= 0)
        return;

    std::vector<vk::CommandBuffer>& commandBuffers = getCommandBuffers();
    if(existingBuffers == 0)
    {
        --needed;
        auto commandBuffer = allocateCommandBuffers(1, true)[0];

        vk::CommandBufferBeginInfo primaryBegin{};

        commandBuffer.begin(primaryBegin);

        commandBuffers.push_back(commandBuffer);
    }

    if(needed == 0)
        return;

    std::vector<vk::CommandBuffer> newBuffers = allocateCommandBuffers(needed, false);
    commandBuffers.insert(commandBuffers.end(), newBuffers.begin(), newBuffers.end());
}


void CommandPool::reset()
{
	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    device->resetCommandPool(mPool);

    if(!mCmdBuffers.empty())
    {
        vk::CommandBufferBeginInfo primaryBegin{};
        mCmdBuffers[0].begin(primaryBegin);
    }
}


std::vector<vk::CommandBuffer> CommandPool::allocateCommandBuffers(const uint32_t number, const bool primaryNeeded)
{
    vk::CommandBufferAllocateInfo info{};
    info.setCommandPool(getCommandPool());
    info.setCommandBufferCount(number);
    info.setLevel(primaryNeeded ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary);

    return static_cast<VulkanRenderDevice*>(getDevice())->allocateCommandBuffers(info);
}


const vk::CommandPool& CommandPool::getCommandPool() const
{
    return mPool;
}


std::vector<vk::CommandBuffer>& CommandPool::getCommandBuffers()
{
    return mCmdBuffers;
}


