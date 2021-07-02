#ifndef VK_INSTANCE_HPP
#define VK_INSTANCE_HPP

#define VK_ENABLE_BETA_EXTENSIONS

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp> // use vulkan hpp for erganomics
#include <GLFW/glfw3.h>

#include <tuple>

#include "Core/RenderDevice.hpp"
#include "Core/RenderInstance.hpp"


struct QueueIndicies
{
	int GraphicsQueueIndex;
	int TransferQueueIndex;
	int ComputeQueueIndex;
};

QueueIndicies getAvailableQueues(vk::SurfaceKHR windowSurface, vk::PhysicalDevice& dev);

class VulkanRenderInstance : public RenderInstance
{
public:
    VulkanRenderInstance(GLFWwindow*);
    ~VulkanRenderInstance();

    RenderDevice* createRenderDevice(int DeviceFeatureFlags = 0, const bool vsyn = false) override;

    void addDebugCallback();
    void removeDebugCallback();

    vk::SurfaceKHR getSurface() const;

private:
    std::pair<vk::PhysicalDevice, vk::Device> findSuitableDevices(int& DeviceFeatureFlags);
	bool									  hasSubgroupSupport(vk::PhysicalDevice);

    vk::Instance mInstance;
    vk::DynamicLoader mLoader;
    vk::Device mDevice;
    vk::DebugUtilsMessengerEXT mDebugMessenger;
    vk::SurfaceKHR mWindowSurface;
};


#endif
