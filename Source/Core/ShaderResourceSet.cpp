#include "Core/ShaderResourceSet.hpp"

#include "Core/RenderDevice.hpp"
#include "Core/ImageView.hpp"
#include "Core/BufferView.hpp"
#include "Core/Sampler.hpp"

#ifdef VULKAN
#include "Core/Vulkan/VulkanShaderResourceSet.hpp"
#endif

#ifdef DX_12
#include "Core/DX_12/DX_12ShaderResourceSet.hpp"
#endif

ShaderResourceSetBase::ShaderResourceSetBase(RenderDevice* dev, const uint32_t maxDescriptors) :
	DeviceChild(dev),
	GPUResource(getDevice()->getCurrentSubmissionIndex()),
    mMaxDescriptors{maxDescriptors}
{}


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


void ShaderResourceSetBase::updateLastAccessed()
{
    const uint64_t submissionIndex = getDevice()->getCurrentSubmissionIndex();
    GPUResource::updateLastAccessed(submissionIndex);

    for(auto& view : mImageViews)
            view->updateLastAccessed();

    for(auto& views : mImageArrays)
        for(auto& view : views)
            view->updateLastAccessed();
}


ShaderResourceSet::ShaderResourceSet(RenderDevice* dev, const uint32_t maxDescriptors)
{
#ifdef VULKAN
	mBase = std::make_shared<VulkanShaderResourceSet>(dev, maxDescriptors);
#endif

#ifdef DX_12
    mBase = std::make_shared<DX_12ShaderResourceSet>(dev, maxDescriptors);
#endif
}


void ShaderResourceSet::reset(RenderDevice* dev, const uint32_t maxDescriptors)
{
    mBase.reset();

#ifdef VULKAN
    mBase = std::make_shared<VulkanShaderResourceSet>(dev, maxDescriptors);
#endif

#ifdef DX_12
    mBase = std::make_shared<DX_12ShaderResourceSet>(dev, maxDescriptors);
#endif
}
