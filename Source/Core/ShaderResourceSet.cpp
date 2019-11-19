#include "Core/ShaderResourceSet.hpp"

#include "Core/RenderDevice.hpp"
#include "Core/ImageView.hpp"
#include "Core/BufferView.hpp"
#include "Core/Sampler.hpp"


ShaderResourceSet::ShaderResourceSet(RenderDevice* dev) :
	DeviceChild(dev),
	GPUResource(getDevice()->getCurrentSubmissionIndex()),
	mLayout{nullptr},
	mDescSet{nullptr},
	mPool{nullptr} {}


ShaderResourceSet::~ShaderResourceSet()
{
	const bool canFree = release();

	if (canFree && mDescSet != vk::DescriptorSet{ nullptr })
	{
		getDevice()->destroyShaderResourceSet(*this);
	}

	mPool = nullptr;
	mLayout = nullptr;
	mDescSet = nullptr;
}


ShaderResourceSet::ShaderResourceSet(ShaderResourceSet&& other) :
	DeviceChild(other),
	GPUResource(other)
{
	mDescSet = other.mDescSet;
	mLayout = other.mLayout;
	mPool = other.mPool;

	other.mLayout = nullptr;
	other.mDescSet = nullptr;
	other.mPool = nullptr;
}


ShaderResourceSet& ShaderResourceSet::operator=(ShaderResourceSet&& other)
{
	const bool canFree = other.release();

	if (canFree && other.mDescSet != vk::DescriptorSet{ nullptr })
	{
		getDevice()->destroyShaderResourceSet(other);
	}

	mDescSet = other.mDescSet;
	mLayout = other.mLayout;
	mPool = other.mPool;

	other.mLayout = nullptr;
	other.mDescSet = nullptr;
	other.mPool = nullptr;

	return *this;
}


ShaderResourceSet& ShaderResourceSet::operator=(const ShaderResourceSet& other)
{
	const bool canFree = release();

	if (canFree && other.mDescSet != vk::DescriptorSet{ nullptr })
	{
		getDevice()->destroyShaderResourceSet(*this);
	}

	mDescSet = other.mDescSet;
	mLayout = other.mLayout;
	mPool = other.mPool;

	return *this;
}


void ShaderResourceSet::addSampledImage(const ImageView& view)
{
	const auto index = mImageViews.size();
	mImageViews.push_back(view);

	mResources.push_back({ index, AttachmentType::Texture2D });
}


void ShaderResourceSet::addStorageImage(const ImageView& view)
{
	const auto index = mImageViews.size();
	mImageViews.push_back(view);

	mResources.push_back({ index, AttachmentType::Image2D });
}


void ShaderResourceSet::addSampledImageArray(const ImageViewArray& views)
{
	const auto index = mImageArrays.size();
	mImageArrays.push_back(views);

	mResources.push_back({ index, AttachmentType::TextureArray });
}


void ShaderResourceSet::addSampler(const Sampler& sampler)
{
	const auto index = mSamplers.size();
	mSamplers.push_back(sampler);

	mResources.push_back({ index, AttachmentType::Sampler });
}


void ShaderResourceSet::addUniformBuffer(const BufferView& view)
{
	const auto index = mBufferViews.size();
	mBufferViews.push_back(view);

	mResources.push_back({ index, AttachmentType::UniformBuffer });
}


void ShaderResourceSet::addDataBuffer(const BufferView& view)
{
	const auto index = mBufferViews.size();
	mBufferViews.push_back(view);

	mResources.push_back({ index, AttachmentType::DataBuffer });
}


void ShaderResourceSet::finalise()
{
	mLayout = getDevice()->generateDescriptorSetLayoutBindings(mResources);

	std::vector<WriteShaderResourceSet> writes{};
	uint32_t binding = 0;
	for (const auto& resource : mResources)
	{
		switch (resource.mType)
		{
		case AttachmentType::Texture1D:
		case AttachmentType::Texture2D:
		case AttachmentType::Texture3D:
		case AttachmentType::Image1D:
		case AttachmentType::Image2D:
		case AttachmentType::Image3D:
		{
			writes.emplace_back(resource.mType, binding, 0, &mImageViews[resource.mIndex], nullptr, nullptr);
			break;
		}

		case AttachmentType::TextureArray:
		{
			writes.emplace_back(resource.mType, binding, mImageArrays[resource.mIndex].size(), mImageArrays[resource.mIndex].data(), nullptr, nullptr);
			break;
		}

		case AttachmentType::Sampler:
		{
			writes.emplace_back(resource.mType, binding, 0, nullptr, nullptr, &mSamplers[resource.mIndex]);
			break;
		}

		case AttachmentType::UniformBuffer:
		case AttachmentType::DataBuffer:
		{
			writes.emplace_back(resource.mType, binding, 0, nullptr, &mBufferViews[resource.mIndex], nullptr);
			break;
		}

		}
		++binding;
	}

	mDescSet = getDevice()->getDescriptorManager()->writeShaderResourceSet(mLayout, writes);
}
