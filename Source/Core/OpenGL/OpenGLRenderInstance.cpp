#include "OpenGLRenderInstance.hpp"
#include "OpenGLRenderDevice.hpp"

#include "glad/glad.h"
#include "GLFW/glfw3.h"


OpenGLRenderInstance::OpenGLRenderInstance(GLFWwindow* window) :
	RenderInstance(window)
{
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		throw std::runtime_error("Failed to load gl symbols");
	}

	glfwMakeContextCurrent(window);
}


OpenGLRenderInstance::~OpenGLRenderInstance()
{
	glfwMakeContextCurrent(nullptr);
}


RenderDevice* OpenGLRenderInstance::createRenderDevice(const int DeviceFeatureFlags)
{
	return new OpenGLRenderDevice(mWindow);
}