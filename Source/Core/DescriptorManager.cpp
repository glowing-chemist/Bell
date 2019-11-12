#include "DescriptorManager.hpp"
#include "RenderDevice.hpp"
#include "Core/ConversionUtils.hpp"
#include "Core/BellLogging.hpp"

#include <iostream>
#include <numeric>
#include <cstdint>

// TODO unify in RenderDevice.cpp
#define BINDLESS_ARRAY_SIZE 16ull


DescriptorManager::DescriptorManager(RenderDevice* dev) :
	DeviceChild{ dev }
{
	for (uint32_t i = 0; i < getDevice()->getSwapChainImageCount(); ++i)
	{
		mPools.push_back({ createDescriptorPool() });
	}
}


DescriptorManager::~DescriptorManager()
{
    for(auto& pools : mPools)
    {
		for (auto& pool : pools)
		{
			getDevice()->destroyDescriptorPool(pool.mPool);
		}
	}
}


void DescriptorManager::getDescriptors(RenderGraph& graph, std::vector<vulkanResources>& resources)
{
	uint32_t resourceIndex = 0;

	auto task = graph.taskBegin();
	auto resource = resources.begin();

    while(task != graph.taskEnd())
    {
		vk::DescriptorSet descSet = allocateDescriptorSet(*task, *resource);

		resource->mDescSet = descSet;

		++task;
		++resource;
		++resourceIndex;
    }
}


void DescriptorManager::writeDescriptors(RenderGraph& graph, std::vector<vulkanResources>& resources)
{
    std::vector<vk::WriteDescriptorSet> descSetWrites;
    std::vector<vk::DescriptorImageInfo> imageInfos;
    std::vector<vk::DescriptorBufferInfo> bufferInfos;

	const uint64_t maxDescWrites = std::accumulate(graph.inputBindingBegin(), graph.inputBindingEnd(), 0ull,
					[](uint64_t accu, const auto& vec) {return accu + std::accumulate(vec.begin(), vec.end(), 0ull, []
																					  (uint64_t accu, const RenderGraph::ResourceBindingInfo& info)
		{
			return accu + (info.mResourcetype == RenderGraph::ResourceType::ImageArray ? BINDLESS_ARRAY_SIZE : 1ull);
		});
	});

    imageInfos.reserve(maxDescWrites);
    bufferInfos.reserve(maxDescWrites);
	uint32_t descIndex = 0;

    auto inputBindings  = graph.inputBindingBegin();
    auto resource       = resources.begin();
    auto task           = graph.taskBegin();

    while(inputBindings != graph.inputBindingEnd())
    {
        uint32_t bindingIndex = 0;

        for(const auto& bindingInfo : *inputBindings)
        {
            vk::WriteDescriptorSet descWrite{};
			descWrite.setDstSet(resource->mDescSet);
            descWrite.setDstBinding(bindingInfo.mResourceBinding);

            const auto& bindings = (*task).getInputAttachments();

            switch(bindingInfo.mResourcetype)
            {
                case RenderGraph::ResourceType::Image:
                {
					// We assume that no sampling happens from the swapchain image.
					const AttachmentType attachmentType = bindings[bindingInfo.mResourceBinding].second;
					const vk::DescriptorType type = [attachmentType]()
                    {
                        switch(attachmentType)
                        {
                            case AttachmentType::Image1D:
                            case AttachmentType::Image2D:
                            case AttachmentType::Image3D:
                                return vk::DescriptorType::eStorageImage;

                            case AttachmentType::Texture1D:
                            case AttachmentType::Texture2D:
                            case AttachmentType::Texture3D:
                                return vk::DescriptorType::eSampledImage;

							default:
								BELL_TRAP;
								return vk::DescriptorType::eCombinedImageSampler;
                        }
                    }();

					auto& imageView = graph.getImageView(bindingInfo.mResourceIndex);

					vk::ImageLayout adjustedLayout = imageView.getType() == ImageViewType::Depth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : getVulkanImageLayout(attachmentType);

					vk::DescriptorImageInfo info = generateDescriptorImageInfo(imageView, adjustedLayout);

					imageInfos.push_back(info);

                    descWrite.setPImageInfo(&imageInfos.back());
                    descWrite.setDescriptorType(type);
					descWrite.setDescriptorCount(1);
                    break;
                }

				// Image arrays can only be sampled in this renderer (at least for now).
				case RenderGraph::ResourceType::ImageArray:
				{
					auto imageViews = graph.getImageArrayViews(bindingInfo.mResourceIndex);

					for(auto& view : imageViews)
					{
						imageInfos.push_back(generateDescriptorImageInfo(view, vk::ImageLayout::eShaderReadOnlyOptimal));
					}

					descWrite.setPImageInfo(&imageInfos[imageInfos.size() - imageViews.size()]);
					descWrite.setDescriptorType(vk::DescriptorType::eSampledImage);
					descWrite.setDescriptorCount(static_cast<uint32_t>(imageViews.size()));
					break;
				}

                case RenderGraph::ResourceType::Sampler:
                {
                    vk::DescriptorImageInfo info{};
                    info.setSampler(getDevice()->getImmutableSampler(graph.getSampler(bindingInfo.mResourceIndex)));

                    imageInfos.push_back(info);

                    descWrite.setPImageInfo(&imageInfos.back());
                    descWrite.setDescriptorType(vk::DescriptorType::eSampler);
					descWrite.setDescriptorCount(1);

                    break;
                }

                case RenderGraph::ResourceType::Buffer:
                {
					auto& bufferView = graph.getBuffer(bindingInfo.mResourceIndex);
					vk::DescriptorBufferInfo info = generateDescriptorBufferInfo(bufferView);
                    bufferInfos.push_back(info);

                    descWrite.setPBufferInfo(&bufferInfos.back());

					if(bufferView.getUsage() & vk::BufferUsageFlagBits::eUniformBuffer)
                        descWrite.setDescriptorType(vk::DescriptorType::eUniformBuffer);
                    else
                        descWrite.setDescriptorType(vk::DescriptorType::eStorageBuffer);

					descWrite.setDescriptorCount(1);
                    break;
                }
            }

            descSetWrites.push_back(descWrite);
            ++bindingIndex;
        }

		++resource;
		++inputBindings;
        ++task;
		++descIndex;
    }

	getDevice()->writeDescriptorSets(descSetWrites);
}


vk::DescriptorSet DescriptorManager::allocateDescriptorSet(const RenderTask& task,
                                                           const vulkanResources& resources)
{    
    const std::vector<std::pair<std::string, AttachmentType>>& attachments = task.getInputAttachments();

	// Find a pool with enough descriptors in to fufill the request.
	DescriptorPool pool = findSuitablePool(attachments);

    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.setDescriptorPool(pool.mPool);
    allocInfo.setDescriptorSetCount(1);
    allocInfo.setPSetLayouts(&resources.mDescSetLayout);

    vk::DescriptorSet descSet = getDevice()->allocateDescriptorSet(allocInfo)[0];

    return descSet;

}


DescriptorManager::DescriptorPool DescriptorManager::createDescriptorPool()
{
    // This pool size had been picked pretty randomly so may need to change this in the future.
    vk::DescriptorPoolSize uniformBufferDescPoolSize{};
    uniformBufferDescPoolSize.setType(vk::DescriptorType::eUniformBuffer);
    uniformBufferDescPoolSize.setDescriptorCount(100);

    vk::DescriptorPoolSize dataBufferDescPoolSize{};
    dataBufferDescPoolSize.setType(vk::DescriptorType::eStorageBuffer);
    dataBufferDescPoolSize.setDescriptorCount(100);

    vk::DescriptorPoolSize samplerrDescPoolSize{};
    samplerrDescPoolSize.setType(vk::DescriptorType::eSampler);
    samplerrDescPoolSize.setDescriptorCount(100);

    vk::DescriptorPoolSize imageDescPoolSize{};
    imageDescPoolSize.setType(vk::DescriptorType::eSampledImage);
    imageDescPoolSize.setDescriptorCount(100);

    vk::DescriptorPoolSize storageImageDescPoolSize{};
    storageImageDescPoolSize.setType(vk::DescriptorType::eStorageImage);
    storageImageDescPoolSize.setDescriptorCount(100);

    std::array<vk::DescriptorPoolSize, 5> descPoolSizes{uniformBufferDescPoolSize,
                                                        samplerrDescPoolSize,
                                                        dataBufferDescPoolSize,
                                                        imageDescPoolSize,
                                                        storageImageDescPoolSize};

    vk::DescriptorPoolCreateInfo DescPoolInfo{};
	DescPoolInfo.setPoolSizeCount(descPoolSizes.size());
	DescPoolInfo.setPPoolSizes(descPoolSizes.data());
	DescPoolInfo.setMaxSets(300);

	DescriptorPool newPool
	{
		100,
		100,
		100,
		100,
		100,
		getDevice()->createDescriptorPool(DescPoolInfo)
	};

	return newPool;
}


vk::DescriptorImageInfo DescriptorManager::generateDescriptorImageInfo(ImageView& imageView, const vk::ImageLayout layout) const
{
    vk::DescriptorImageInfo imageInfo{};
    imageInfo.setImageView(imageView.getImageView());
	imageInfo.setImageLayout(layout);

    return imageInfo;
}


vk::DescriptorBufferInfo DescriptorManager::generateDescriptorBufferInfo(BufferView& buffer) const
{
    vk::DescriptorBufferInfo bufferInfo{};
    bufferInfo.setBuffer(buffer.getBuffer());
	bufferInfo.setOffset(buffer.getOffset());
	bufferInfo.setRange(buffer.getSize());

    return bufferInfo;
}


void DescriptorManager::reset()
{
	const auto frameindex = getDevice()->getCurrentFrameIndex();

	std::vector<DescriptorPool>& pools = mPools[frameindex];

	for (auto& pool : pools)
	{
		getDevice()->resetDescriptorPool(pool.mPool);
	}
}


DescriptorManager::DescriptorPool& DescriptorManager::findSuitablePool(const std::vector<std::pair<std::string, AttachmentType>>& attachments)
{
	size_t requiredStorageImageDescriptors = 0;
	size_t requiredSampledImageDescriptors = 0;
	size_t requiredSamplerDescriptors = 0;
	size_t requiredUniformBufferDescriptors = 0;
	size_t requiredStorageBufferDescriptors = 0;

	for (const auto& atta : attachments)
	{
		switch (atta.second)
		{

		case AttachmentType::DataBuffer:
			requiredStorageBufferDescriptors++;
			break;

		case AttachmentType::UniformBuffer:
			requiredUniformBufferDescriptors++;
			break;

		case AttachmentType::Texture1D:
		case AttachmentType::Texture2D:
		case AttachmentType::Texture3D:
			requiredSampledImageDescriptors++;
			break;

		case AttachmentType::Image1D:
		case AttachmentType::Image2D:
		case AttachmentType::Image3D:
			requiredStorageImageDescriptors++;
			break;

		case AttachmentType::TextureArray:
			requiredSampledImageDescriptors += BINDLESS_ARRAY_SIZE;
			break;

		case AttachmentType::Sampler:
			requiredSamplerDescriptors++;
			break;

		default:
			break;
		}
	}

	std::vector<DescriptorPool>& framePools = mPools[getDevice()->getCurrentFrameIndex()];

	auto it = std::find_if(framePools.begin(), framePools.end(), [=](const DescriptorPool& p)
		{
			return	requiredStorageImageDescriptors <= p.mStorageImageCount &&
					requiredSampledImageDescriptors <= p.mSampledImageCount &&
					requiredSamplerDescriptors <= p.mSamplerCount &&
					requiredUniformBufferDescriptors <= p.mUniformBufferCount &&
					requiredStorageBufferDescriptors <= p.mStorageBufferCount;
		});

	if (it != framePools.end())
	{
		DescriptorPool& pool = *it;

		pool.mStorageImageCount -= requiredStorageImageDescriptors;
		pool.mSampledImageCount -= requiredSampledImageDescriptors;
		pool.mSamplerCount		-= requiredSamplerDescriptors;
		pool.mUniformBufferCount -= requiredUniformBufferDescriptors;
		pool.mStorageBufferCount -= requiredStorageBufferDescriptors;

		return pool;
	}
	else
	{
		framePools.push_back(createDescriptorPool());

		DescriptorPool& pool = framePools.back();

		pool.mStorageImageCount -= requiredStorageImageDescriptors;
		pool.mSampledImageCount -= requiredSampledImageDescriptors;
		pool.mSamplerCount -= requiredSamplerDescriptors;
		pool.mUniformBufferCount -= requiredUniformBufferDescriptors;
		pool.mStorageBufferCount -= requiredStorageBufferDescriptors;

		return pool;
	}
}
