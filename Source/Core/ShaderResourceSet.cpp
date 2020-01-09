#include "Core/ShaderResourceSet.hpp"

#include "Core/RenderDevice.hpp"
#include "Core/ImageView.hpp"
#include "Core/BufferView.hpp"
#include "Core/Sampler.hpp"

#ifdef VULKAN
#include "Core/Vulkan/VulkanShaderResourceSet.hpp"
#endif

ShaderResourceSetBase::ShaderResourceSetBase(RenderDevice* dev) :
	DeviceChild(dev),
	GPUResource(getDevice()->getCurrentSubmissionIndex()) {}


void ShaderResourceSetBase::addSampledImage(const ImageView& view)
{
	const auto index = mImageViews.size();
	mImageViews.push_back(view);

    mResources.push_back({ index, AttachmentType::Texture2D, 0 });
}


void ShaderResourceSetBase::addStorageImage(const ImageView& view)
{
	const auto index = mImageViews.size();
	mImageViews.push_back(view);

    mResources.push_back({ index, AttachmentType::Image2D, 0 });
}


void ShaderResourceSetBase::addSampledImageArray(const ImageViewArray& views)
{
	const auto index = mImageArrays.size();
	mImageArrays.push_back(views);

    mResources.push_back({ index, AttachmentType::TextureArray, views.size() });
}


void ShaderResourceSetBase::addSampler(const Sampler& sampler)
{
	const auto index = mSamplers.size();
	mSamplers.push_back(sampler);

    mResources.push_back({ index, AttachmentType::Sampler, 0 });
}


void ShaderResourceSetBase::addUniformBuffer(const BufferView& view)
{
	const auto index = mBufferViews.size();
	mBufferViews.push_back(view);

    mResources.push_back({ index, AttachmentType::UniformBuffer, 0 });
}

void ShaderResourceSetBase::addDataBufferRO(const BufferView& view)
{
    const auto index = mBufferViews.size();
    mBufferViews.push_back(view);

    mResources.push_back({ index, AttachmentType::DataBufferRO, 0 });
}


void ShaderResourceSetBase::addDataBufferRW(const BufferView& view)
{
    const auto index = mBufferViews.size();
    mBufferViews.push_back(view);

    mResources.push_back({ index, AttachmentType::DataBufferRW, 0 });
}


void ShaderResourceSetBase::addDataBufferWO(const BufferView& view)
{
    const auto index = mBufferViews.size();
    mBufferViews.push_back(view);

    mResources.push_back({ index, AttachmentType::DataBufferWO, 0 });
}


ShaderResourceSet::ShaderResourceSet(RenderDevice* dev)
{
#ifdef VULKAN
	mBase = std::make_shared<VulkanShaderResourceSet>(dev);
#endif
}
