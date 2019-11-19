#ifndef SHADER_RESOURCE_SET_HPP
#define SHADER_RESOURCE_SET_HPP

#include <vulkan/vulkan.hpp>

#include "Core/DeviceChild.hpp"
#include "Core/GPUResource.hpp"
#include "Engine/PassTypes.hpp"

#include <vector>

class ImageView;
class BufferView;
class Sampler;
using ImageViewArray = std::vector<ImageView>;


class ShaderResourceSet : public DeviceChild, public GPUResource
{
public:
	ShaderResourceSet(RenderDevice* dev);
	~ShaderResourceSet();

	ShaderResourceSet(const ShaderResourceSet&) = default;
	ShaderResourceSet& operator=(const ShaderResourceSet&);

	ShaderResourceSet(ShaderResourceSet&&);
	ShaderResourceSet& operator=(ShaderResourceSet&&);

	void addSampledImage(const ImageView&);
	void addStorageImage(const ImageView&);
	void addSampledImageArray(const ImageViewArray&);
	void addSampler(const Sampler&);
	void addUniformBuffer(const BufferView&);
	void addDataBuffer(const BufferView&);
	void finalise();

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

	struct ResourceInfo
	{
		size_t mIndex;
		AttachmentType mType;
	};

private:

	std::vector<ImageView> mImageViews;
	std::vector<BufferView> mBufferViews;
	std::vector<Sampler>   mSamplers;
	std::vector<ImageViewArray> mImageArrays;

	std::vector<ResourceInfo> mResources;

	vk::DescriptorSetLayout mLayout;
	vk::DescriptorSet mDescSet;
	vk::DescriptorPool mPool;
};

#endif