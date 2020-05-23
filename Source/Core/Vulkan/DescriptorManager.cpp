#include "DescriptorManager.hpp"
#include "VulkanRenderDevice.hpp"
#include "VulkanBufferView.hpp"
#include "Core/ConversionUtils.hpp"
#include "Core/BellLogging.hpp"

#include <iostream>
#include <numeric>
#include <cstdint>


DescriptorManager::DescriptorManager(RenderDevice* dev) :
	DeviceChild{ dev }
{
	for (uint32_t i = 0; i < getDevice()->getSwapChainImageCount(); ++i)
	{
		mPerFramePools.push_back({ createDescriptorPool() });
	}

	mPersistentPools.push_back(createDescriptorPool(true));
}


DescriptorManager::~DescriptorManager()
{
	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    for(auto& pools : mPerFramePools)
    {
		for (auto& pool : pools)
		{
			device->destroyDescriptorPool(pool.mPool);
		}
	}

	for (auto& pool : mPersistentPools)
	{
		device->destroyDescriptorPool(pool.mPool);
	}
}


std::vector<vk::DescriptorSet> DescriptorManager::getDescriptors(const RenderGraph &graph, const uint32_t taskIndex, const vk::DescriptorSetLayout layout)
{
    const RenderTask& task = graph.getTask(taskIndex);

    vk::DescriptorSet descSet = allocateDescriptorSet(task, layout);

    std::vector<vk::DescriptorSet> descSets{};
    descSets.push_back(descSet);

    // Now add all the ShaderResourceSet descriptors.
    const auto& inputs = task.getInputAttachments();
    for (const auto& [slot, type, _] : inputs)
    {
        if (type == AttachmentType::ShaderResourceSet)
        {
            descSets.push_back(static_cast<const VulkanShaderResourceSet&>(*graph.getBoundShaderResourceSet(slot).getBase()).getDescriptorSet());
        }
    }

    return descSets;
}


void DescriptorManager::writeDescriptors(const RenderGraph& graph, const uint32_t taskIndex, vk::DescriptorSet descSet)
{
    std::vector<vk::WriteDescriptorSet> descSetWrites;
    std::vector<vk::DescriptorImageInfo> imageInfos;
    std::vector<vk::DescriptorBufferInfo> bufferInfos;

	const uint64_t maxDescWrites = std::accumulate(graph.inputBindingBegin(), graph.inputBindingEnd(), 0ull,
                    [&](uint64_t accu, const auto& vec) {return accu + std::accumulate(vec.begin(), vec.end(), 0ull, [&]
																					  (uint64_t accu, const RenderGraph::ResourceBindingInfo& info)
		{
            return accu + (info.mResourcetype == RenderGraph::ResourceType::ImageArray ? graph.getImageArrayViews(info.mResourceIndex).size() : 1ull);
		});
	});

    imageInfos.reserve(maxDescWrites);
    bufferInfos.reserve(maxDescWrites);

    auto inputBindings  = graph.inputBindingBegin() + taskIndex;
    const RenderTask& task           = graph.getTask(taskIndex);

    const auto& bindings = task.getInputAttachments();

    for(const auto& bindingInfo : *inputBindings)
    {
        const AttachmentType attachmentType = bindings[bindingInfo.mResourceBinding].mType;

        if(attachmentType == AttachmentType::VertexBuffer ||
                attachmentType == AttachmentType::IndexBuffer)
        {
            continue;
        }

        vk::WriteDescriptorSet descWrite{};
        descWrite.setDstSet(descSet);
        descWrite.setDstBinding(bindingInfo.mResourceBinding);

        switch(bindingInfo.mResourcetype)
        {
        case RenderGraph::ResourceType::Image:
        {
            // We assume that no sampling happens from the swapchain image.
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

            vk::ImageLayout adjustedLayout = imageView->getType() == ImageViewType::Depth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : getVulkanImageLayout(attachmentType);

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
            const auto& imageViews = graph.getImageArrayViews(bindingInfo.mResourceIndex);

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
            info.setSampler(static_cast<VulkanRenderDevice*>(getDevice())->getImmutableSampler(graph.getSampler(bindingInfo.mResourceIndex)));

            imageInfos.push_back(info);

            descWrite.setPImageInfo(&imageInfos.back());
            descWrite.setDescriptorType(vk::DescriptorType::eSampler);
            descWrite.setDescriptorCount(1);

            break;
        }

        case RenderGraph::ResourceType::Buffer:
        case RenderGraph::ResourceType::VertexBuffer:
        case RenderGraph::ResourceType::IndexBuffer:
        {
            auto& bufferView = graph.getBuffer(bindingInfo.mResourceIndex);
            vk::DescriptorBufferInfo info = generateDescriptorBufferInfo(bufferView);
            bufferInfos.push_back(info);

            descWrite.setPBufferInfo(&bufferInfos.back());

            if (attachmentType == AttachmentType::IndirectBuffer)
                continue;
            else if (attachmentType == AttachmentType::UniformBuffer)
                descWrite.setDescriptorType(vk::DescriptorType::eUniformBuffer);
            else
                descWrite.setDescriptorType(vk::DescriptorType::eStorageBuffer);

            descWrite.setDescriptorCount(1);
            break;
        }

        case RenderGraph::ResourceType::SRS:
            BELL_TRAP;
            break;
        }

        descSetWrites.push_back(descWrite);
    }

	static_cast<VulkanRenderDevice*>(getDevice())->writeDescriptorSets(descSetWrites);
}


vk::DescriptorSet DescriptorManager::writeShaderResourceSet(const vk::DescriptorSetLayout layout, const std::vector<WriteShaderResourceSet>& writes, vk::DescriptorPool& outPool)
{
	std::vector<vk::WriteDescriptorSet> descSetWrites;
	std::vector<vk::DescriptorImageInfo> imageInfos;
	std::vector<vk::DescriptorBufferInfo> bufferInfos;

	const auto maxImages = std::accumulate(writes.begin(), writes.end(), 0ull, []
	(uint64_t accu, const WriteShaderResourceSet& info)
		{
            return accu + (info.mType == AttachmentType::TextureArray ? info.mArraySize : 1ull);
		});

	imageInfos.reserve(maxImages);
	bufferInfos.reserve(writes.size());

    vk::DescriptorPool allocatedPool;
    vk::DescriptorSet descSet = allocatePersistentDescriptorSet(layout, writes, allocatedPool);
    outPool = allocatedPool;

	for (uint32_t i = 0; i < writes.size(); ++i)
	{
		vk::WriteDescriptorSet descWrite{};
		descWrite.setDstSet(descSet);
		descWrite.setDstBinding(i);

		const AttachmentType attachmentType = writes[i].mType;

		switch (attachmentType)
		{
		case AttachmentType::Image1D:
		case AttachmentType::Image2D:
		case AttachmentType::Image3D:
		{
            BELL_ASSERT(writes[i].mImage, "Attachment type set incorrectly")
			const auto& imageView = *writes[i].mImage;

			vk::ImageLayout adjustedLayout = imageView->getType() == ImageViewType::Depth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : getVulkanImageLayout(attachmentType);

			vk::DescriptorImageInfo info = generateDescriptorImageInfo(imageView, adjustedLayout);

			imageInfos.push_back(info);

			descWrite.setPImageInfo(&imageInfos.back());
			descWrite.setDescriptorType(vk::DescriptorType::eStorageImage);
			descWrite.setDescriptorCount(1);
			break;
		}

		case AttachmentType::Texture1D:
		case AttachmentType::Texture2D:
		case AttachmentType::Texture3D:
		{
            BELL_ASSERT(writes[i].mImage, "Attachment type set incorrectly")
			const auto& imageView = *writes[i].mImage;

			vk::ImageLayout adjustedLayout = imageView->getType() == ImageViewType::Depth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : getVulkanImageLayout(attachmentType);

			vk::DescriptorImageInfo info = generateDescriptorImageInfo(imageView, adjustedLayout);

			imageInfos.push_back(info);

			descWrite.setPImageInfo(&imageInfos.back());
			descWrite.setDescriptorType(vk::DescriptorType::eSampledImage);
			descWrite.setDescriptorCount(1);
			break;
		}

		// Image arrays can only be sampled in this renderer (at least for now).
		case AttachmentType::TextureArray:
		{
            BELL_ASSERT(writes[i].mImage, "Attachment type set incorrectly")
			const auto* imageViews = writes[i].mImage;

			for(uint32_t k = 0; k < writes[i].mArraySize; ++k)
			{
				imageInfos.push_back(generateDescriptorImageInfo(imageViews[k], vk::ImageLayout::eShaderReadOnlyOptimal));
			}

			descWrite.setPImageInfo(&imageInfos[imageInfos.size() - writes[i].mArraySize]);
			descWrite.setDescriptorType(vk::DescriptorType::eSampledImage);
			descWrite.setDescriptorCount(static_cast<uint32_t>(writes[i].mArraySize));
			break;
		}

		case AttachmentType::Sampler:
		{
            BELL_ASSERT(writes[i].mSampler, "Attachment type set incorrectly")
			vk::DescriptorImageInfo info{};
			info.setSampler(static_cast<VulkanRenderDevice*>(getDevice())->getImmutableSampler(*writes[i].mSampler));

			imageInfos.push_back(info);

			descWrite.setPImageInfo(&imageInfos.back());
			descWrite.setDescriptorType(vk::DescriptorType::eSampler);
			descWrite.setDescriptorCount(1);

			break;
		}

		case AttachmentType::UniformBuffer:
		{
            BELL_ASSERT(writes[i].mBuffer, "Attachment type set incorrectly")
			auto& bufferView = *writes[i].mBuffer;
			vk::DescriptorBufferInfo info = generateDescriptorBufferInfo(bufferView);
			bufferInfos.push_back(info);

			descWrite.setPBufferInfo(&bufferInfos.back());
			descWrite.setDescriptorType(vk::DescriptorType::eUniformBuffer);

			descWrite.setDescriptorCount(1);
			break;
		}

        case AttachmentType::DataBufferRO:
        case AttachmentType::DataBufferRW:
        case AttachmentType::DataBufferWO:
		{
            BELL_ASSERT(writes[i].mBuffer, "Attachment type set incorrectly")
			auto& bufferView = *writes[i].mBuffer;
			vk::DescriptorBufferInfo info = generateDescriptorBufferInfo(bufferView);
			bufferInfos.push_back(info);

			descWrite.setPBufferInfo(&bufferInfos.back());
			descWrite.setDescriptorType(vk::DescriptorType::eStorageBuffer);

			descWrite.setDescriptorCount(1);
			break;
		}

        default:
            // Other attachment types don't get written in descripotor sets e.g push constants or frame buffer types.
            BELL_TRAP;
            break;
		}

		descSetWrites.push_back(descWrite);
	}

	static_cast<VulkanRenderDevice*>(getDevice())->writeDescriptorSets(descSetWrites);

	return descSet;
}


vk::DescriptorSet DescriptorManager::allocatePersistentDescriptorSet(const vk::DescriptorSetLayout layout, const std::vector<WriteShaderResourceSet>& writes, vk::DescriptorPool& outPool)
{
	DescriptorPool pool = findSuitablePool(writes, mPersistentPools);
    outPool = pool.mPool;

	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.setDescriptorPool(pool.mPool);
	allocInfo.setDescriptorSetCount(1);
	allocInfo.setPSetLayouts(&layout);

	vk::DescriptorSet descSet = static_cast<VulkanRenderDevice*>(getDevice())->allocateDescriptorSet(allocInfo)[0];

	return descSet;
}


vk::DescriptorSet DescriptorManager::allocateDescriptorSet(const RenderTask& task,
                                                           const vk::DescriptorSetLayout layout)
{    
    const auto& attachments = task.getInputAttachments();

	// Find a pool with enough descriptors in to fufill the request.
	DescriptorPool pool = findSuitablePool(attachments, mPerFramePools[getDevice()->getCurrentFrameIndex()]);

    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.setDescriptorPool(pool.mPool);
    allocInfo.setDescriptorSetCount(1);
    allocInfo.setPSetLayouts(&layout);

    vk::DescriptorSet descSet = static_cast<VulkanRenderDevice*>(getDevice())->allocateDescriptorSet(allocInfo)[0];

    return descSet;

}


DescriptorManager::DescriptorPool DescriptorManager::createDescriptorPool(const bool allowIndividualReset)
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
	DescPoolInfo.setFlags(allowIndividualReset ? vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet : vk::DescriptorPoolCreateFlags{});

	DescriptorPool newPool
	{
		100,
		100,
		100,
		100,
		100,
		static_cast<VulkanRenderDevice*>(getDevice())->createDescriptorPool(DescPoolInfo)
	};

	return newPool;
}


vk::DescriptorImageInfo DescriptorManager::generateDescriptorImageInfo(const ImageView& imageView, const vk::ImageLayout layout) const
{
    vk::DescriptorImageInfo imageInfo{};
    imageInfo.setImageView(static_cast<const VulkanImageView&>(*imageView.getBase()).getImageView());
	imageInfo.setImageLayout(layout);

    return imageInfo;
}


vk::DescriptorBufferInfo DescriptorManager::generateDescriptorBufferInfo(const BufferView& buffer) const
{
    vk::DescriptorBufferInfo bufferInfo{};
    bufferInfo.setBuffer(static_cast<const VulkanBufferView&>(*buffer.getBase()).getBuffer());
	bufferInfo.setOffset(buffer->getOffset());
	bufferInfo.setRange(buffer->getSize());

    return bufferInfo;
}


void DescriptorManager::reset()
{
	const auto frameindex = getDevice()->getCurrentFrameIndex();

	std::vector<DescriptorPool>& pools = mPerFramePools[frameindex];

	for (auto& pool : pools)
	{
		static_cast<VulkanRenderDevice*>(getDevice())->resetDescriptorPool(pool.mPool);
		pool.mSampledImageCount = 100;
		pool.mSamplerCount = 100;
		pool.mStorageBufferCount = 100;
		pool.mStorageBufferCount = 100;
		pool.mStorageImageCount = 100;
		pool.mUniformBufferCount = 100;
	}
}


DescriptorManager::DescriptorPool& DescriptorManager::findSuitablePool(const std::vector<WriteShaderResourceSet>& attachments, std::vector<DescriptorPool>& pools)
{
	size_t requiredStorageImageDescriptors = 0;
	size_t requiredSampledImageDescriptors = 0;
	size_t requiredSamplerDescriptors = 0;
	size_t requiredUniformBufferDescriptors = 0;
	size_t requiredStorageBufferDescriptors = 0;

	for (const auto& attachment : attachments)
	{
		switch (attachment.mType)
		{

        case AttachmentType::DataBufferRO:
        case AttachmentType::DataBufferRW:
        case AttachmentType::DataBufferWO:
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
            requiredSampledImageDescriptors += attachment.mArraySize;
			break;

		case AttachmentType::Sampler:
			requiredSamplerDescriptors++;
			break;

		default:
			break;
		}
	}

	auto it = std::find_if(pools.begin(), pools.end(), [=](const DescriptorPool& p)
		{
			return	requiredStorageImageDescriptors <= p.mStorageImageCount &&
					requiredSampledImageDescriptors <= p.mSampledImageCount &&
					requiredSamplerDescriptors <= p.mSamplerCount &&
					requiredUniformBufferDescriptors <= p.mUniformBufferCount &&
					requiredStorageBufferDescriptors <= p.mStorageBufferCount;
		});

	if (it != pools.end())
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
		pools.push_back(createDescriptorPool());

		DescriptorPool& pool = pools.back();

		pool.mStorageImageCount -= requiredStorageImageDescriptors;
		pool.mSampledImageCount -= requiredSampledImageDescriptors;
		pool.mSamplerCount -= requiredSamplerDescriptors;
		pool.mUniformBufferCount -= requiredUniformBufferDescriptors;
		pool.mStorageBufferCount -= requiredStorageBufferDescriptors;

		return pool;
	}
}


DescriptorManager::DescriptorPool& DescriptorManager::findSuitablePool(const std::vector<RenderTask::InputAttachmentInfo>& attachments, std::vector<DescriptorPool>& pools)
{
    size_t requiredStorageImageDescriptors = 0;
    size_t requiredSampledImageDescriptors = 0;
    size_t requiredSamplerDescriptors = 0;
    size_t requiredUniformBufferDescriptors = 0;
    size_t requiredStorageBufferDescriptors = 0;

    for (const auto& attachment : attachments)
    {
        switch (attachment.mType)
        {

        case AttachmentType::DataBufferRO:
        case AttachmentType::DataBufferRW:
        case AttachmentType::DataBufferWO:
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
            BELL_TRAP; // If you hit here you should probably consider using a shader resource set instead.
            // Removed bacuase was unused and inconvient.
            //requiredSampledImageDescriptors += BINDLESS_ARRAY_SIZE;
            break;

        case AttachmentType::Sampler:
            requiredSamplerDescriptors++;
            break;

        default:
            break;
        }
    }

    auto it = std::find_if(pools.begin(), pools.end(), [=](const DescriptorPool& p)
        {
            return	requiredStorageImageDescriptors <= p.mStorageImageCount &&
                    requiredSampledImageDescriptors <= p.mSampledImageCount &&
                    requiredSamplerDescriptors <= p.mSamplerCount &&
                    requiredUniformBufferDescriptors <= p.mUniformBufferCount &&
                    requiredStorageBufferDescriptors <= p.mStorageBufferCount;
        });

    if (it != pools.end())
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
        pools.push_back(createDescriptorPool());

        DescriptorPool& pool = pools.back();

        pool.mStorageImageCount -= requiredStorageImageDescriptors;
        pool.mSampledImageCount -= requiredSampledImageDescriptors;
        pool.mSamplerCount -= requiredSamplerDescriptors;
        pool.mUniformBufferCount -= requiredUniformBufferDescriptors;
        pool.mStorageBufferCount -= requiredStorageBufferDescriptors;

        return pool;
    }
}

