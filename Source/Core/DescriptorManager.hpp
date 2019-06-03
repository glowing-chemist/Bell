#ifndef DESCRIPTOR_MANAGER_HPP
#define DESCRIPTOR_MANAGER_HPP

#include "DeviceChild.hpp"
#include "GPUResource.hpp"
#include "RenderGraph/RenderGraph.hpp"

#include <vulkan/vulkan.hpp>

#include <vector>
#include <map>

class RenderDevice;


class DescriptorManager : public DeviceChild
{
public:
    DescriptorManager(RenderDevice* dev) : DeviceChild{dev} { mPools.push_back(createDescriptorPool()); }
    ~DescriptorManager();

    std::vector<vk::DescriptorSet> getDescriptors(RenderGraph&, std::vector<vulkanResources>&);
    void                           writeDescriptors(std::vector<vk::DescriptorSet>&, RenderGraph&, std::vector<vulkanResources>&);

    void                           freeDescriptorSet(const std::vector<AttachmentType>&, vk::DescriptorSet);

private:

	vk::DescriptorImageInfo     generateDescriptorImageInfo(ImageView &imageView, const vk::ImageLayout) const;
    vk::DescriptorBufferInfo    generateDescriptorBufferInfo(Buffer&) const;

    vk::DescriptorSet			allocateDescriptorSet(const RenderTask&, const vulkanResources&);

    vk::DescriptorPool			createDescriptorPool();

    void                        transferFreeDescriptorSets();

    std::map<std::vector<AttachmentType>, std::vector<vk::DescriptorSet>> mFreeDescriptorSets;
    std::map<std::vector<AttachmentType>, std::vector<std::pair<uint64_t, vk::DescriptorSet>>> mPendingFreeDescriptorSets;
    std::vector<vk::DescriptorPool> mPools;
};


#endif
