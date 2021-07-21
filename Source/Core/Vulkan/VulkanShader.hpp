#ifndef VK_SHADER_HPP
#define VK_SHADER_HPP

#include "Core/Shader.hpp"

#include <vulkan/vulkan.hpp>


class VulkanShader : public ShaderBase
{
public:

    VulkanShader(RenderDevice*, const std::string&);
	~VulkanShader() override;

    virtual bool compile(const std::vector<ShaderDefine>& prefix = {}) override;
	virtual bool reload() override;

	vk::ShaderModule getShaderModule() const
	{
		return mShaderModule;
	}

private:

	const wchar_t * getShaderStage(const std::string&) const;
    const wchar_t* mShaderProfile;

	vk::ShaderModule mShaderModule;

};

#endif
