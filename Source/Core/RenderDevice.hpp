#ifndef RENDERDEVICE_HPP
#define RENDERDEVICE_HPP

#include <cstddef>
#include <vulkan/vulkan.hpp>

#include "MemoryManager.hpp"

class GLFWwindow;

class RenderDevice {
public:
    RenderDevice(vk::PhysicalDevice, vk::Device, vk::SurfaceKHR, GLFWwindow*);

    // Memory management functions
    vk::PhysicalDeviceMemoryProperties getMemoryProperties() const;
    vk::DeviceMemory                   allocateMemory(vk::MemoryAllocateInfo);
    void                               freeMemory(const vk::DeviceMemory);
    void*                              mapMemory(vk::DeviceMemory, vk::DeviceSize, vk::DeviceSize);
    void                               unmapMemory(vk::DeviceMemory);

    void                               bindBufferMemory(vk::Buffer&, vk::DeviceMemory, uint64_t);
    void                               bindImageMemory(vk::Image&, vk::DeviceMemory, uint64_t);
private:

    // Keep track of when resources can be freed
    uint64_t mCurrentSubmission;
    uint64_t mFinishedSubmission;

    // underlying devices
    vk::Device mDevice;
    VkPhysicalDevice mPhysicalDevice;
};

#endif
