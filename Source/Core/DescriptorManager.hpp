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
    DescriptorManager(RenderDevice* dev) : DeviceChild{dev} {}
    ~DescriptorManager();

    std::vector<vk::DescriptorSet> getDescriptors(RenderGraph&);
    void                           writeDescriptors(std::vector<vk::DescriptorSet>&, RenderGraph&);

    void                           freeDescriptorSets(RenderGraph&);

private:

    vk::DescriptorImageInfo     generateDescriptorImageInfo(Image&) const;
    vk::DescriptorBufferInfo    generateDescriptorBufferInfo(Buffer&) const;

    vk::DescriptorSet			allocateDescriptorSet(const RenderGraph&, const RenderTask&, const RenderGraph::vulkanResources&);

    vk::DescriptorPool			createDescriptorPool();

    std::map<std::vector<AttachmentType>, std::vector<vk::DescriptorSet>> mFreeDescriptoSets;
    std::vector<vk::DescriptorPool> mPools;
};


#endif
