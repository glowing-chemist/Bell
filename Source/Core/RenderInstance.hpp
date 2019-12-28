#ifndef INSTANCE_HPP
#define INSTANCE_HPP

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

class RenderInstance 
{
public:
    RenderInstance(GLFWwindow*);
    virtual ~RenderInstance() = default;

    virtual RenderDevice* createRenderDevice(const int DeviceFeatureFlags = 0) = 0;

    GLFWwindow* getWindow() const;

protected:

    GLFWwindow* mWindow;
};


#endif
