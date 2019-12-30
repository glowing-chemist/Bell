// Local includes
#include "RenderInstance.hpp"
#include "Core/BellLogging.hpp"


RenderInstance::RenderInstance(GLFWwindow* window) :
	mWindow{ window }
{
}


int operator|(DeviceFeaturesFlags lhs, DeviceFeaturesFlags rhs)
{
    return static_cast<int>(lhs) | static_cast<int>(rhs);
}


int operator|(int lhs, DeviceFeaturesFlags rhs)
{
	return lhs | static_cast<int>(rhs);
}


bool operator&(int flags, DeviceFeaturesFlags rhs)
{
    return flags & static_cast<int>(rhs);
}
