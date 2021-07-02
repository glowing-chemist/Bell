#ifndef RENDER_INSTANCE_HPP
#define RENDER_INSTANCE_HPP

#include "RenderDevice.hpp"

struct GLFWwindow;

enum class DeviceFeaturesFlags {
    Geometry = 1,
    Tessalation = 1 << 1,
    Discrete = 1 << 2,
    Compute = 1 << 3,
	Subgroup = 1 << 4,
    RayTracing = 1 << 5
};

int operator|(DeviceFeaturesFlags, DeviceFeaturesFlags);
int operator|(int, DeviceFeaturesFlags);
bool operator&(int, DeviceFeaturesFlags);

class RenderInstance 
{
public:
    RenderInstance(GLFWwindow*);
    virtual ~RenderInstance() = default;

    virtual RenderDevice* createRenderDevice(const int DeviceFeatureFlags = 0, const bool vsync = false) = 0;

	GLFWwindow* getWindow() const
	{
		return mWindow;
	}

protected:

    GLFWwindow* mWindow;
};


#endif
