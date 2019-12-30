#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include "RenderDevice.hpp"

struct GLFWwindow;

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

	GLFWwindow* getWindow() const
	{
		return mWindow;
	}

protected:

    GLFWwindow* mWindow;
};


#endif
