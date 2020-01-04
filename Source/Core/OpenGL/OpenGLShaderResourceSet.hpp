#ifndef GL_SHADER_RESOURCE_SET
#define GL_SHADER_RESOURCE_SET

#include "Core/ShaderResourceSet.hpp"


class OpenGLShaderResourceSet : public ShaderResourceSetBase
{
public:

	OpenGLShaderResourceSet(RenderDevice* dev) :
		ShaderResourceSetBase(dev) {}

	~OpenGLShaderResourceSet() = default;

	virtual void finalise() override {};
};

#endif