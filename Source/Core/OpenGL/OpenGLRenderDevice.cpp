#include "OpenGLRenderDevice.hpp"
#include "OpenGLImage.hpp"
#include "OpenGLBuffer.hpp"

#include "glad/glad.h"
#include "GLFW/glfw3.h"


OpenGLRenderDevice::OpenGLRenderDevice(GLFWwindow* window) :
	mWindow(window)
{

}


OpenGLRenderDevice::~OpenGLRenderDevice()
{

}


void OpenGLRenderDevice::generateFrameResources(RenderGraph& graph)
{

}


void OpenGLRenderDevice::startPass(const RenderTask&)
{

}


Executor* OpenGLRenderDevice::getPassExecutor()
{

}


void OpenGLRenderDevice::freePassExecutor(Executor*)
{

}


void OpenGLRenderDevice::endPass()
{
	return;
}

void OpenGLRenderDevice::startFrame()
{

}


void OpenGLRenderDevice::endFrame()
{

}


void OpenGLRenderDevice::destroyImage(ImageBase& image)
{
	OpenGLImage& GLImage = static_cast<OpenGLImage&>(image);
	const uint32_t textureHandle = GLImage.getImage();

	glDeleteTextures(1, &textureHandle);
}


void OpenGLRenderDevice::destroyBuffer(BufferBase& buffer)
{
	OpenGLBuffer& GLBuffer = static_cast<OpenGLBuffer&>(buffer);
	const uint32_t bufferHandle = GLBuffer.getBuffer();

	glDeleteBuffers(1, &bufferHandle);
}


void OpenGLRenderDevice::setDebugName(const std::string& name, const uint64_t handle, const uint64_t objectType)
{
	glLabelObjectEXT(objectType, handle, name.size(), name.c_str());
}


void OpenGLRenderDevice::flushWait() const
{
	glFlush();
}


void OpenGLRenderDevice::swap()
{
	glfwSwapBuffers(mWindows);
}


size_t OpenGLRenderDevice::getMinStorageBufferAlignment() const
{
	return 1;
}
