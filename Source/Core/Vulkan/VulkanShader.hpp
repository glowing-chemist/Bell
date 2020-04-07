#ifndef VK_SHADER_HPP
#define VK_SHADER_HPP

#include "Core/Shader.hpp"

#include <vulkan/vulkan.hpp>

#include "glslang/Public/ShaderLang.h"


class VulkanShader : public ShaderBase
{
public:

	VulkanShader(RenderDevice*, const std::string&);
	~VulkanShader();

	virtual bool compile(const std::string& prefix = "") override;
	virtual bool reload() override;

	vk::ShaderModule getShaderModule() const
	{
		return mShaderModule;
	}

private:

	EShLanguage getShaderStage(const std::string&) const;
	EShLanguage mShaderStage;

	vk::ShaderModule mShaderModule;

};

#endif
