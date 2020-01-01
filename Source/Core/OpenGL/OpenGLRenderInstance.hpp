#ifndef GL_RENDER_INSTANCE_HPP
#define GL_RENDER_INSTANCE_HPP

#include "Core/RenderInstance.hpp"

#include "glad/glad.h"

struct GLFWwindow;


class OpenGLRenderInstance : public RenderInstance
{
public:
	OpenGLRenderInstance(GLFWwindow*);
	~OpenGLRenderInstance();

	virtual RenderDevice* createRenderDevice(const int DeviceFeatureFlags = 0) override;
};

#endif