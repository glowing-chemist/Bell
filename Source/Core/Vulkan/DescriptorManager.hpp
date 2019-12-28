#ifndef DESCRIPTOR_MANAGER_HPP
#define DESCRIPTOR_MANAGER_HPP

#include "Core/DeviceChild.hpp"
#include "Core/GPUResource.hpp"
#include "RenderGraph/RenderGraph.hpp"

#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <vector>
#include <map>


class RenderDevice;
class ImageView;
class BufferView;


struct WriteShaderResourceSet
{
	WriteShaderResourceSet(const AttachmentType type, const uint32_t binding, const uint32_t arraySize,
							ImageView* imageView, BufferView* bufView, Sampler* sampler) :
		mType{type},
		mBinding{binding},
		mArraySize{arraySize},
		mImage{imageView},
		mBuffer{bufView},
		mSampler{sampler} {}

	AttachmentType mType;
	uint32_t mBinding;
	uint32_t mArraySize;
	ImageView* mImage;
	BufferView* mBuffer;
	Sampler* mSampler;
};


class DescriptorManager : public DeviceChild
{
public:
	DescriptorManager(RenderDevice* dev);
    ~DescriptorManager();

	void	 getDescriptors(RenderGraph&, std::vector<vulkanResources>&);
	void     writeDescriptors(RenderGraph&, std::vector<vulkanResources>&);


	vk::DescriptorSet	writeShaderResourceSet(const vk::DescriptorSetLayout layout, const std::vector<WriteShaderResourceSet>&);

	void	 reset();

private:

	vk::DescriptorImageInfo     generateDescriptorImageInfo(const ImageView &imageView, const vk::ImageLayout) const;
	vk::DescriptorBufferInfo    generateDescriptorBufferInfo(const BufferView &) const;

    vk::DescriptorSet			allocateDescriptorSet(const RenderTask&, const vulkanResources&);
	vk::DescriptorSet			allocatePersistentDescriptorSet(const vk::DescriptorSetLayout layout, const std::vector<WriteShaderResourceSet>&);

	struct DescriptorPool
	{
		size_t mStorageImageCount;
		size_t mSampledImageCount;
		size_t mSamplerCount;
		size_t mUniformBufferCount;
		size_t mStorageBufferCount;
		vk::DescriptorPool mPool;
	};

	template<typename T>
	DescriptorPool& findSuitablePool(const std::vector<T>&, std::vector<DescriptorPool>&);

	DescriptorPool	createDescriptorPool(const bool allowIndividualReset = false);

    std::vector<std::vector<DescriptorPool>> mPerFramePools;
	std::vector<DescriptorPool> mPersistentPools;
};


#endif
