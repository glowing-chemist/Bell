#ifndef VK_SHADER_RESOURCE_SET_HPP
#define VK_SHADER_RESOURCE_SET_HPP

#include "Core/DeviceChild.hpp"
#include "Core/GPUResource.hpp"
#include "Core/ShaderResourceSet.hpp"
#include "Engine/PassTypes.hpp"

#include <vector>
#include <vulkan/vulkan.hpp>


class VulkanShaderResourceSet : public ShaderResourceSetBase
{
public:
	VulkanShaderResourceSet(RenderDevice* dev, const uint32_t maxDescriptors);
	~VulkanShaderResourceSet();

	virtual void finalise() override;

	vk::DescriptorSetLayout getLayout() const
	{
		return mLayout;
	}

	vk::DescriptorSet getDescriptorSet() const
	{
		return mDescSet;
	}

	vk::DescriptorPool getPool() const
	{
		return mPool;
	}

private:

	vk::DescriptorSetLayout mLayout;
	vk::DescriptorSet mDescSet;
	vk::DescriptorPool mPool;
};

#endif
