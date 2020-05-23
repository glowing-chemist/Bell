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
struct vulkanResources;


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

    std::vector<vk::DescriptorSet>	 getDescriptors(const RenderGraph&, const uint32_t taskIndex, const vk::DescriptorSetLayout);
    void     writeDescriptors(const RenderGraph&, const uint32_t taskIndex, vk::DescriptorSet);


    vk::DescriptorSet	writeShaderResourceSet(const vk::DescriptorSetLayout layout, const std::vector<WriteShaderResourceSet>&, vk::DescriptorPool& outPool);

	void	 reset();

private:

	vk::DescriptorImageInfo     generateDescriptorImageInfo(const ImageView &imageView, const vk::ImageLayout) const;
	vk::DescriptorBufferInfo    generateDescriptorBufferInfo(const BufferView &) const;

    vk::DescriptorSet			allocateDescriptorSet(const RenderTask&, const vk::DescriptorSetLayout);
    vk::DescriptorSet			allocatePersistentDescriptorSet(const vk::DescriptorSetLayout layout, const std::vector<WriteShaderResourceSet>&, vk::DescriptorPool &pool);

	struct DescriptorPool
	{
		size_t mStorageImageCount;
		size_t mSampledImageCount;
		size_t mSamplerCount;
		size_t mUniformBufferCount;
		size_t mStorageBufferCount;
		vk::DescriptorPool mPool;
	};

    DescriptorPool& findSuitablePool(const std::vector<WriteShaderResourceSet>&, std::vector<DescriptorPool>&);
    DescriptorPool& findSuitablePool(const std::vector<RenderTask::InputAttachmentInfo>&, std::vector<DescriptorPool>&);

	DescriptorPool	createDescriptorPool(const bool allowIndividualReset = false);

    std::vector<std::vector<DescriptorPool>> mPerFramePools;
	std::vector<DescriptorPool> mPersistentPools;
};


#endif
