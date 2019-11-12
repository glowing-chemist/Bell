#ifndef DESCRIPTOR_MANAGER_HPP
#define DESCRIPTOR_MANAGER_HPP

#include "DeviceChild.hpp"
#include "GPUResource.hpp"
#include "RenderGraph/RenderGraph.hpp"

#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <vector>
#include <map>


class RenderDevice;


class DescriptorManager : public DeviceChild
{
public:
	DescriptorManager(RenderDevice* dev);
    ~DescriptorManager();

	void	 getDescriptors(RenderGraph&, std::vector<vulkanResources>&);
	void     writeDescriptors(RenderGraph&, std::vector<vulkanResources>&);

	void	 reset();

private:

	vk::DescriptorImageInfo     generateDescriptorImageInfo(ImageView &imageView, const vk::ImageLayout) const;
	vk::DescriptorBufferInfo    generateDescriptorBufferInfo(BufferView &) const;

    vk::DescriptorSet			allocateDescriptorSet(const RenderTask&, const vulkanResources&);

	struct DescriptorPool
	{
		size_t mStorageImageCount;
		size_t mSampledImageCount;
		size_t mSamplerCount;
		size_t mUniformBufferCount;
		size_t mStorageBufferCount;
		vk::DescriptorPool mPool;
	};

	DescriptorPool& findSuitablePool(const std::vector<std::pair<std::string, AttachmentType>>&);

	DescriptorPool	createDescriptorPool();

    std::vector<std::vector<DescriptorPool>> mPools;
};


#endif
