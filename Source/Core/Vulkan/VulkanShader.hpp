#ifndef VK_SHADER_HPP
#define VK_SHADER_HPP

#include "Core/Shader.hpp"

#include <vulkan/vulkan.hpp>

#include "shaderc/shaderc.hpp"


class VulkanShader : public ShaderBase
{
public:

	VulkanShader(RenderDevice*, const std::string&);
	~VulkanShader();

	virtual bool compile(const std::vector<std::string>& prefix = {}) override;
	virtual bool reload() override;

	vk::ShaderModule getShaderModule() const
	{
		return mShaderModule;
	}

private:

	shaderc_shader_kind getShaderStage(const std::string&) const;
	shaderc_shader_kind mShaderStage;

	vk::ShaderModule mShaderModule;

};

#endif
