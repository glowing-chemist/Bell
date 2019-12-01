#ifndef VKCONTEXT_HPP
#define VKCONTEXT_HPP

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp> // use vulkan hpp for erganomics
#include <GLFW/glfw3.h>

#include <tuple>

#include "RenderDevice.hpp"

enum class DeviceFeaturesFlags {
    Geometry = 1,
    Tessalation = 1 << 1,
    Discrete = 1 << 2,
    Compute = 1 << 3,
	Subgroup = 1 << 4 
};

int operator|(DeviceFeaturesFlags, DeviceFeaturesFlags);
int operator|(int, DeviceFeaturesFlags);
bool operator&(int, DeviceFeaturesFlags);


const QueueIndicies getAvailableQueues(vk::SurfaceKHR windowSurface, vk::PhysicalDevice& dev);

class RenderInstance 
{
public:
    RenderInstance(GLFWwindow*);
    ~RenderInstance();

    RenderDevice createRenderDevice(const int DeviceFeatureFlags = 0);

    void addDebugCallback();
    void removeDebugCallback();

    GLFWwindow* getWindow() const;
    vk::SurfaceKHR getSurface() const;

private:
    std::pair<vk::PhysicalDevice, vk::Device> findSuitableDevices(const int DeviceFeatureFlags = 0);
	bool									  hasSubgroupSupport(vk::PhysicalDevice);

    vk::Instance mInstance;
    vk::Device mDevice;
    vk::DebugReportCallbackEXT mDebugCallback;
    vk::SurfaceKHR mWindowSurface;
    GLFWwindow* mWindow;
};


#endif
