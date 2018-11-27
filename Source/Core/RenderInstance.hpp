#ifndef VKCONTEXT_HPP
#define VKCONTEXT_HPP

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp> // use vulkan hpp for erganomics
#include <GLFW/glfw3.h>

#include <tuple>

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

class VKInstance {
public:
    VKInstance(GLFWwindow*);
    ~VKInstance();

    void addDebugCallback();

    std::pair<vk::PhysicalDevice, vk::Device> findSuitableDevices(int DeviceFeatureFlags = 0);
    GLFWwindow* getWindow() const;
    vk::SurfaceKHR getSurface() const;

private:

    vk::Instance mInstance;
    vk::DebugReportCallbackEXT debugCallback;
    vk::SurfaceKHR mWindowSurface;
    GLFWwindow* mWindow;
};


#endif
