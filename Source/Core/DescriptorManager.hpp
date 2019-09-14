#ifndef DESCRIPTOR_MANAGER_HPP
#define DESCRIPTOR_MANAGER_HPP

#include "DeviceChild.hpp"
#include "GPUResource.hpp"
#include "RenderGraph/RenderGraph.hpp"

#include <vulkan/vulkan.hpp>

#include <algorithm>
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

    void                           freeDescriptorSet(const std::vector<std::pair<std::string, AttachmentType>>&, vk::DescriptorSet);

private:

	vk::DescriptorImageInfo     generateDescriptorImageInfo(ImageView &imageView, const vk::ImageLayout) const;
	vk::DescriptorBufferInfo    generateDescriptorBufferInfo(BufferView &) const;

    vk::DescriptorSet			allocateDescriptorSet(const RenderTask&, const vulkanResources&);

    vk::DescriptorPool			createDescriptorPool();

    void                        transferFreeDescriptorSets();

    struct AttachmentComparitor
    {
        bool operator()(const std::vector<std::pair<std::string, AttachmentType>>& lhs,
                      const std::vector<std::pair<std::string, AttachmentType>>& rhs) const noexcept
        {
            return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), [] (const std::pair<std::string, AttachmentType>& lhs,
                                                                                 const std::pair<std::string, AttachmentType>& rhs)
            {
                return lhs.second < rhs.second;
            });
        }
    };

    std::map<std::vector<std::pair<std::string, AttachmentType>>,
             std::vector<vk::DescriptorSet>,
             AttachmentComparitor> mFreeDescriptorSets;

    std::map<std::vector<std::pair<std::string, AttachmentType>>,
             std::vector<std::pair<uint64_t, vk::DescriptorSet>>,
             AttachmentComparitor> mPendingFreeDescriptorSets;

    std::vector<vk::DescriptorPool> mPools;
};


#endif
