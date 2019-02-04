#include "DescriptorManager.hpp"
#include "RenderDevice.hpp"

#include <iostream>
#include <numeric>
#include <cstdint>

DescriptorManager::~DescriptorManager()
{
    for(auto& pool : mPools)
    {
        getDevice()->destroyDescriptorPool(pool);
    }
}


std::vector<vk::DescriptorSet> DescriptorManager::getDescriptors(const RenderGraph& graph)
{
    std::vector<vk::DescriptorSet> descSets;

    uint32_t resourcesIndex = 0;
    for(auto [taskType, taskIndex] : graph.mTaskOrder)
    {
        if(graph.mVulkanResources[resourcesIndex].mDescSet == vk::DescriptorSet{nullptr})
        {
            const RenderTask& task = graph.getTask(taskType, taskIndex);
            vk::DescriptorSet descSet = allocateDescriptorSet(graph, task, resourcesIndex);
            descSets.push_back(descSet);
        }
        else
        {
            descSets.push_back(graph.mVulkanResources[resourcesIndex].mDescSet);
        }
        ++resourcesIndex;
    }

    return descSets;
}



void DescriptorManager::writeDescriptors(std::vector<vk::DescriptorSet>& descSets, RenderGraph& graph)
{
    std::vector<vk::WriteDescriptorSet> descSetWrites;
    std::vector<vk::DescriptorImageInfo> imageInfos;
    std::vector<vk::DescriptorBufferInfo> bufferInfos;

    const uint64_t maxDescWrites = std::accumulate(graph.mInputResources.begin(), graph.mInputResources.end(), 0,
                    [](uint64_t accu, auto& vec) {return accu + vec.size(); });

    imageInfos.reserve(maxDescWrites);
    bufferInfos.reserve(maxDescWrites);
    uint32_t currentDescriptorIndex = 0;

    for(const auto& inputs : graph.mInputResources)
    {
        for(const auto& bindingInfo : inputs)
        {
            vk::WriteDescriptorSet descWrite{};
            descWrite.setDescriptorCount(1);
            descWrite.setDstSet(descSets[currentDescriptorIndex]);
            descWrite.setDstBinding(bindingInfo.mResourceBinding);

            switch(bindingInfo.mResourcetype)
            {
                case RenderGraph::ResourceType::Image:
                {
                    auto& image = static_cast<Image&>(graph.getResource(bindingInfo.mResourcetype, bindingInfo.mResourceIndex));
                    vk::DescriptorImageInfo info = generateDescriptorImageInfo(image);
                    imageInfos.push_back(info);

                    descWrite.setPImageInfo(&imageInfos.back());
                    break;
                }
                case RenderGraph::ResourceType::Buffer:
                {
                    auto& buffer = static_cast<Buffer&>(graph.getResource(bindingInfo.mResourcetype, bindingInfo.mResourceIndex));
                    vk::DescriptorBufferInfo info = generateDescriptorBufferInfo(buffer);
                    bufferInfos.push_back(info);

                    descWrite.setPBufferInfo(&bufferInfos.back());
                    break;
                }

            }

            descSetWrites.push_back(descWrite);
        }

        ++currentDescriptorIndex;
    }

    getDevice()->writeDescriptorSets(descSetWrites);

}


void DescriptorManager::freeDescriptorSets(RenderGraph& graph)
{
    // Add descriptor sets back to the free list.
}


vk::DescriptorSet DescriptorManager::allocateDescriptorSet(const RenderGraph& graph,
                                                           const RenderTask& task,
                                                           const uint32_t taskIndex)
{    
    // Find a better/more efficient way to do this.
    std::vector<AttachmentType> attachments(task.getInputAttachments().size());
    std::transform(task.getInputAttachments().begin(), task.getInputAttachments().end(),
                   attachments.begin(), [](const auto& attachmentInfo) { return attachmentInfo.second;});

    if(mFreeDescriptoSets[attachments].size() != 0)
    {
        const vk::DescriptorSet descSet = mFreeDescriptoSets[attachments].back();
        mFreeDescriptoSets[attachments].pop_back();

        return descSet;
    }

    for(auto& pool : mPools)
    {
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.setDescriptorPool(pool);
        allocInfo.setDescriptorSetCount(1);
        allocInfo.setPSetLayouts(&graph.mVulkanResources[taskIndex].mDescSetLayout);

        try
        {
            vk::DescriptorSet descSet = getDevice()->allocateDescriptorSet(allocInfo)[0];

            return descSet;

            // TODO: check we're allocating from the first pool, if not swap it with the first pool
            // as this one still has descriptor sets to allocate
        }
        catch (...)
        {
            std::cerr << "pool exhausted trying next descriptor pool \n";
        }
    }
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

    vk::DescriptorPoolSize imageSamplerrDescPoolSize{};
    imageSamplerrDescPoolSize.setType(vk::DescriptorType::eCombinedImageSampler);
    imageSamplerrDescPoolSize.setDescriptorCount(100);

    std::array<vk::DescriptorPoolSize, 3> descPoolSizes{uniformBufferDescPoolSize, imageSamplerrDescPoolSize, dataBufferDescPoolSize};

    vk::DescriptorPoolCreateInfo uniformBufferDescPoolInfo{};
    uniformBufferDescPoolInfo.setPoolSizeCount(descPoolSizes.size()); // two pools one for uniform buffers and one for combined image samplers
    uniformBufferDescPoolInfo.setPPoolSizes(descPoolSizes.data());
    uniformBufferDescPoolInfo.setMaxSets(100);

    return getDevice()->createDescriptorPool(uniformBufferDescPoolInfo);
}


vk::DescriptorImageInfo DescriptorManager::generateDescriptorImageInfo(Image& image) const
{
    vk::DescriptorImageInfo imageInfo{};
    imageInfo.setSampler(image.getCurrentSampler());
    imageInfo.setImageView(image.getCurrentImageView());
    imageInfo.setImageLayout(image.getLayout());

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
