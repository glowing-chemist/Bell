#include "RenderDevice.hpp"
#include "RenderInstance.hpp"
#include <vulkan/vulkan.hpp>


RenderDevice::RenderDevice(vk::PhysicalDevice physDev, vk::Device dev, vk::SurfaceKHR surface, GLFWwindow* window) :
    mCurrentSubmission{0},
    mFinishedSubmission{0},
    mDevice{dev},
    mPhysicalDevice{physDev},
    mSwapChain{mDevice, mPhysicalDevice, surface, window},
    mMemoryManager{this} 
{

    mQueueFamilyIndicies = getAvailableQueues(surface, mPhysicalDevice);
    mGraphicsQueue = mDevice.getQueue(mQueueFamilyIndicies.GraphicsQueueIndex, 0);
    mComputeQueue  = mDevice.getQueue(mQueueFamilyIndicies.ComputeQueueIndex, 0);
    mTransferQueue = mDevice.getQueue(mQueueFamilyIndicies.TransferQueueIndex, 0);

	// Create a command pool for each frame.
	for (uint32_t i = 0; i < mSwapChain.getNumberOfSwapChainImages(); ++i)
	{
		mCommandPools.push_back(CommandPool{ this });
	}
}

vk::Image   RenderDevice::createImage(const vk::Format format,
                                      const vk::ImageUsageFlags usage,
                                      const vk::ImageType type,
                                      const uint32_t x,
                                      const uint32_t y,
                                      const uint32_t z)
{
    // default to only allowing 2D images for now, with no MS
    vk::ImageCreateInfo createInfo(vk::ImageCreateFlags{},
                                  type,
                                  format,
                                  vk::Extent3D{x, y, z},
                                  0,
                                  1,
                                  vk::SampleCountFlagBits::e1,
                                  vk::ImageTiling::eOptimal,
                                  usage,
                                  vk::SharingMode::eExclusive,
                                  0,
                                  nullptr,
                                  vk::ImageLayout::eUndefined);

    return mDevice.createImage(createInfo);
}


vk::Buffer  RenderDevice::createBuffer(const uint32_t size, const vk::BufferUsageFlags usage)
{
    vk::BufferCreateInfo createInfo{};
    createInfo.setSize(size);
    createInfo.setUsage(usage);

    return mDevice.createBuffer(createInfo);
}


vk::Queue&  RenderDevice::getQueue(const QueueType type)
{
    switch(type)
    {
        case QueueType::Graphics:
            return mGraphicsQueue;
        case QueueType::Compute:
            return mComputeQueue;
        case QueueType::Transfer:
            return mTransferQueue;
    }
    return mGraphicsQueue; // This should be unreachable unless I add more queue types.
}


uint32_t RenderDevice::getQueueFamilyIndex(const QueueType type) const
{
	switch(type)
	{
	case QueueType::Graphics:
		return mQueueFamilyIndicies.GraphicsQueueIndex;
	case QueueType::Compute:
		return mQueueFamilyIndicies.ComputeQueueIndex;
	case QueueType::Transfer:
		return mQueueFamilyIndicies.TransferQueueIndex;
	}
	return mQueueFamilyIndicies.GraphicsQueueIndex; // This should be unreachable unless I add more queue types.
}


std::pair<vk::Pipeline, vk::PipelineLayout>	RenderDevice::generatePipelineFromTask(GraphicsTask&)
{
	
}


std::pair<vk::Pipeline, vk::PipelineLayout>	RenderDevice::generatePipelineFromTask(ComputeTask&)
{

}


vk::RenderPass								RenderDevice::generateRenderPassFromTask(GraphicsTask&)
{

}


// Memory management functions
vk::PhysicalDeviceMemoryProperties RenderDevice::getMemoryProperties() const
{
    return mPhysicalDevice.getMemoryProperties();
}


vk::DeviceMemory    RenderDevice::allocateMemory(vk::MemoryAllocateInfo allocInfo)
{
    return mDevice.allocateMemory(allocInfo);
}


void    RenderDevice::freeMemory(const vk::DeviceMemory memory)
{
    mDevice.freeMemory(memory);
}


void*   RenderDevice::mapMemory(vk::DeviceMemory memory, vk::DeviceSize size, vk::DeviceSize offset)
{
    return mDevice.mapMemory(memory, offset, size);
}


void    RenderDevice::unmapMemory(vk::DeviceMemory memory)
{
    mDevice.unmapMemory(memory);
}


void    RenderDevice::bindBufferMemory(vk::Buffer& buffer, vk::DeviceMemory memory, uint64_t offset)
{
    mDevice.bindBufferMemory(buffer, memory, offset);
}


void    RenderDevice::bindImageMemory(vk::Image& image, vk::DeviceMemory memory, const uint64_t offset)
{
    mDevice.bindImageMemory(image, memory, offset);
}

