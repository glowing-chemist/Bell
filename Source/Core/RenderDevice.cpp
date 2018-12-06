#include "RenderDevice.hpp"

#include <vulkan/vulkan.hpp>

RenderDevice::RenderDevice(vk::PhysicalDevice physDev, vk::Device dev, vk::SurfaceKHR surface, GLFWwindow* window) :
    mCurrentSubmission{0},
    mFinishedSubmission{0},
    mDevice{dev},
    mPhysicalDevice{physDev},
    mSwapChain{mDevice, mPhysicalDevice, surface, window},
    mMemoryManager{this} {}

vk::Image   RenderDevice::createImage(vk::Format format,
                                      vk::ImageUsageFlags usage,
                                      vk::ImageType type,
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

