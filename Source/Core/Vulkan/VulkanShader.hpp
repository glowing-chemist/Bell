#ifndef VK_SHADER_HPP
#define VK_SHADER_HPP

#include "Core/Shader.hpp"

#include <Vulkan/vulkan.hpp>


class VulkanShader : public ShaderBase
{
public:

	VulkanShader(RenderDevice*, const std::string&);
	~VulkanShader();

	virtual bool compile() override;
	virtual bool reload() override;

	vk::ShaderModule getShaderModule() const
	{
		return mShaderModule;
	}

private:

	vk::ShaderModule mShaderModule;

};

#endif