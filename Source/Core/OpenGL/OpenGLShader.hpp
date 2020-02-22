#ifndef GL_SHADER_HPP
#define GL_SHADER_HPP

#include "Core/Shader.hpp"


class OpenGLShader : public ShaderBase
{
public:

	OpenGLShader(RenderDevice*, const std::string&);
	~OpenGLShader();

	virtual bool compile(const std::string& prefix = "") override;
	virtual bool reload() override;

	uint32_t getShader() const
	{
		return mShader;
	}

private:

	uint32_t mShader;

};

#endif