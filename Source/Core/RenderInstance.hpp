#ifndef VKCONTEXT_HPP
#define VKCONTEXT_HPP

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp> // use vulkan hpp for erganomics
#include <GLFW/glfw3.h>

#include <tuple>

#include "RenderDevice.hpp"

enum class DeviceFeaturesFlags {
    Geometry = 1,
    Tessalation = 2,
    Discrete = 4,
    Compute = 8
};

int operator|(DeviceFeaturesFlags, DeviceFeaturesFlags);
int operator|(int, DeviceFeaturesFlags);
bool operator&(int, DeviceFeaturesFlags);

struct QueueIndicies {
    int GraphicsQueueIndex;
    int TransferQueueIndex;
    int ComputeQueueIndex;
};


const QueueIndicies getAvailableQueues(vk::SurfaceKHR windowSurface, vk::PhysicalDevice& dev);

class RenderInstance {
public:
    RenderInstance(GLFWwindow*);
    ~RenderInstance();

    RenderDevice createRenderDevice(const int DeviceFeatureFlags = 0);

    void addDebugCallback();

    GLFWwindow* getWindow() const;
    vk::SurfaceKHR getSurface() const;

private:
    std::pair<vk::PhysicalDevice, vk::Device> findSuitableDevices(const int DeviceFeatureFlags = 0);

    vk::Instance mInstance;
    vk::DebugReportCallbackEXT debugCallback;
    vk::SurfaceKHR mWindowSurface;
    GLFWwindow* mWindow;
};


#endif
