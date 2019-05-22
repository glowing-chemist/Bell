#include "DescriptorManager.hpp"
#include "RenderDevice.hpp"
#include "Core/BellLogging.hpp"

#include <iostream>
#include <numeric>
#include <cstdint>

namespace
{
    std::vector<AttachmentType> stripName(const std::vector<std::pair<std::string, AttachmentType>>& vec)
    {
        // Find a better/more efficient way to do this.
        std::vector<AttachmentType> attachments(vec.size());
        std::transform(vec.begin(), vec.end(),
                       attachments.begin(), [](const auto& attachmentInfo) { return attachmentInfo.second;});

        return attachments;
    }
}


DescriptorManager::~DescriptorManager()
{
    for(auto& pool : mPools)
    {
        getDevice()->destroyDescriptorPool(pool);
    }
}


std::vector<vk::DescriptorSet> DescriptorManager::getDescriptors(RenderGraph& graph, std::vector<vulkanResources>& resources)
{
    // move all descriptor sets pending destruction that have been finished with to the free cache.
    transferFreeDescriptorSets();

    std::vector<vk::DescriptorSet> descSets;
	uint32_t resourceIndex = 0;

	auto task = graph.taskBegin();
	auto resource = resources.begin();

    while(task != graph.taskEnd())
    {
        if((*resource).mDescSet == vk::DescriptorSet{nullptr})
        {
            vk::DescriptorSet descSet = allocateDescriptorSet(*task, *resource);
            descSets.push_back(descSet);

            (*resource).mDescSet = descSet;
        }
        else if(graph.getDescriptorsNeedUpdating()[resourceIndex])
        {
            vk::DescriptorSet descSet = allocateDescriptorSet(*task, *resource);
            descSets.push_back(descSet);

            auto attatchmentTypes = stripName((*task).getInputAttachments());
            mPendingFreeDescriptorSets[attatchmentTypes].push_back({getDevice()->getCurrentSubmissionIndex(), (*resource).mDescSet});

            (*resource).mDescSet = descSet;
        }
        else
        {
            descSets.push_back((*resource).mDescSet);
        }

		++task;
		++resource;
		++resourceIndex;
    }

    return descSets;
}


void DescriptorManager::writeDescriptors(std::vector<vk::DescriptorSet>& descSets, RenderGraph& graph, std::vector<vulkanResources>& resources)
{
    std::vector<vk::WriteDescriptorSet> descSetWrites;
    std::vector<vk::DescriptorImageInfo> imageInfos;
    std::vector<vk::DescriptorBufferInfo> bufferInfos;

    const uint64_t maxDescWrites = std::accumulate(graph.inputBindingBegin(), graph.inputBindingEnd(), 0,
                    [](uint64_t accu, auto& vec) {return accu + vec.size(); });

    imageInfos.reserve(maxDescWrites);
    bufferInfos.reserve(maxDescWrites);
	uint32_t descIndex = 0;

    auto inputBindings  = graph.inputBindingBegin();
    auto resource       = resources.begin();
    auto task           = graph.taskBegin();

    while(inputBindings != graph.inputBindingEnd())
    {
        if(!graph.getDescriptorsNeedUpdating()[descIndex])
        {
            ++resource;
            ++inputBindings;
            ++task;
            ++descIndex;
            continue;
        }

        uint32_t bindingIndex = 0;

        for(const auto& bindingInfo : *inputBindings)
        {
            vk::WriteDescriptorSet descWrite{};
            descWrite.setDescriptorCount(1);
            descWrite.setDstSet(descSets[descIndex]);
            descWrite.setDstBinding(bindingInfo.mResourceBinding);

            const auto& bindings = (*task).getInputAttachments();

            switch(bindingInfo.mResourcetype)
            {
                case RenderGraph::ResourceType::Image:
                {
                    auto& imageView = graph.getImageView(bindingInfo.mResourceIndex);
                    vk::DescriptorImageInfo info = generateDescriptorImageInfo(imageView);
                    imageInfos.push_back(info);

                    const vk::DescriptorType type = [attachmentType = bindings[bindingIndex].second]()
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
                        }
                    }();

                    descWrite.setPImageInfo(&imageInfos.back());
                    descWrite.setDescriptorType(type);
                    break;
                }

                case RenderGraph::ResourceType::Sampler:
                {
                    vk::DescriptorImageInfo info{};
                    info.setSampler(getDevice()->getImmutableSampler(graph.getSampler(bindingInfo.mResourceIndex)));

                    imageInfos.push_back(info);

                    descWrite.setPImageInfo(&imageInfos.back());
                    descWrite.setDescriptorType(vk::DescriptorType::eSampler);

                    break;
                }

                case RenderGraph::ResourceType::Buffer:
                {
                    auto& buffer = graph.getBuffer(bindingInfo.mResourceIndex);
                    vk::DescriptorBufferInfo info = generateDescriptorBufferInfo(buffer);
                    bufferInfos.push_back(info);

                    descWrite.setPBufferInfo(&bufferInfos.back());

                    if(buffer.getUsage() & vk::BufferUsageFlagBits::eUniformBuffer)
                        descWrite.setDescriptorType(vk::DescriptorType::eUniformBuffer);
                    else
                        descWrite.setDescriptorType(vk::DescriptorType::eStorageBuffer);

                    break;
                }
            }

            descSetWrites.push_back(descWrite);
            ++bindingIndex;

            (*resource).mDescriptorsWritten = true;
        }

		++resource;
		++inputBindings;
        ++task;
		++descIndex;
    }

    getDevice()->writeDescriptorSets(descSetWrites);

}


void DescriptorManager::freeDescriptorSet(const std::vector<AttachmentType>& layout, vk::DescriptorSet descSet)
{
    mFreeDescriptorSets[layout].push_back(descSet);
}


vk::DescriptorSet DescriptorManager::allocateDescriptorSet(const RenderTask& task,
                                                           const vulkanResources& resources)
{    
    std::vector<AttachmentType> attachments = stripName(task.getInputAttachments());

    if(mFreeDescriptorSets[attachments].size() != 0)
    {
        const vk::DescriptorSet descSet = mFreeDescriptorSets[attachments].back();
        mFreeDescriptorSets[attachments].pop_back();

        return descSet;
    }

    for(auto& pool : mPools)
    {
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.setDescriptorPool(pool);
        allocInfo.setDescriptorSetCount(1);
        allocInfo.setPSetLayouts(&resources.mDescSetLayout);

        try
        {
            vk::DescriptorSet descSet = getDevice()->allocateDescriptorSet(allocInfo)[0];

            return descSet;

            // TODO: check we're allocating from the first pool, if not swap it with the first pool
            // as this one still has descriptor sets to allocate
        }
        catch (...)
        {
            BELL_LOG("pool exhausted trying next descriptor pool")
        }
    }


	// We have failed to allocate from any of the exhisting pools
	// so allocate create a new one and allocate a set from it.
	mPools.insert(mPools.begin(), createDescriptorPool());

    return allocateDescriptorSet(task, resources);
}


vk::DescriptorPool DescriptorManager::createDescriptorPool()
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

    vk::DescriptorPoolCreateInfo uniformBufferDescPoolInfo{};
    uniformBufferDescPoolInfo.setPoolSizeCount(descPoolSizes.size());
    uniformBufferDescPoolInfo.setPPoolSizes(descPoolSizes.data());
    uniformBufferDescPoolInfo.setMaxSets(100);

    return getDevice()->createDescriptorPool(uniformBufferDescPoolInfo);
}


vk::DescriptorImageInfo DescriptorManager::generateDescriptorImageInfo(ImageView& imageView) const
{
    vk::DescriptorImageInfo imageInfo{};
    imageInfo.setImageView(imageView.getImageView());
    imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    return imageInfo;
}


vk::DescriptorBufferInfo DescriptorManager::generateDescriptorBufferInfo(Buffer& buffer) const
{
    vk::DescriptorBufferInfo bufferInfo{};
    bufferInfo.setBuffer(buffer.getBuffer());
    bufferInfo.setOffset(buffer.getCurrentOffset());
    bufferInfo.setRange(VK_WHOLE_SIZE);

    return bufferInfo;
}


void DescriptorManager::transferFreeDescriptorSets()
{
    for(auto& [attachmentTypes, submissionIndexAndDescriptorSet] : mPendingFreeDescriptorSets)
    {
        uint32_t descriptorIndex = 0;
        for(const auto&[lastUsedSubmissionIndex, descriptorSet] : submissionIndexAndDescriptorSet)
        {
            if(lastUsedSubmissionIndex <= getDevice()->getFinishedSubmissionIndex())
            {
                mFreeDescriptorSets[attachmentTypes].push_back(descriptorSet);

                submissionIndexAndDescriptorSet.erase(submissionIndexAndDescriptorSet.begin() + descriptorIndex);
            }
            ++descriptorIndex;
        }
    }
}

