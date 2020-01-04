#include "OpenGLShader.hpp"
#include "Core/RenderDevice.hpp"

#include "glad/glad.h"


OpenGLShader::OpenGLShader(RenderDevice* dev, const std::string& name) :
	ShaderBase(dev, name),
	mShader(-1)
{
}


OpenGLShader::~OpenGLShader()
{
	if(mCompiled)
		glDeleteShader(mShader);
}


bool OpenGLShader::compile()
{
	const bool compiled = ShaderBase::compile();

	// create the GL shader object now we know what stage it is.
	const uint32_t stage = [this]()
	{
		switch (mShaderStage)
		{
		case EShLangVertex:
			return GL_VERTEX_SHADER_BIT;
		case EShLangTessControl:
			return GL_TESS_CONTROL_SHADER_BIT;
		case EShLangTessEvaluation:
			return GL_TESS_EVALUATION_SHADER_BIT;
		case EShLangGeometry:
			return GL_GEOMETRY_SHADER_BIT;
		case EShLangFragment:
			return GL_FRAGMENT_SHADER_BIT;
		case EShLangCompute:
			return GL_COMPUTE_SHADER_BIT;
		default:
			BELL_TRAP;
			return GL_FRAGMENT_SHADER_BIT;
		}
	}();

	mShader = glCreateShader(stage);

	glShaderBinary(1, &mShader, GL_SHADER_BINARY_FORMAT_SPIR_V, mSPIRV.data(), mSPIRV.size() * sizeof(uint32_t));

	return compiled;
}


bool OpenGLShader::reload()
{
	if (mCompiled)
		glDeleteShader(mShader);

	const bool compiled = ShaderBase::compile();

	glShaderBinary(1, &mShader, GL_SHADER_BINARY_FORMAT_SPIR_V, mSPIRV.data(), mSPIRV.size() * sizeof(uint32_t));

	return compiled;
}
