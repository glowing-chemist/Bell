#ifndef DEVICECHILD_HPP
#define DEVICECHILD_HPP

#include <memory>
#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>

class RenderDevice;
class Pipeline;

// Put this here for now to avoid cyclic header dependancies.
struct vulkanResources
{
	std::shared_ptr<Pipeline> mPipeline;
	std::vector<vk::DescriptorSetLayout> mDescSetLayout;
	// Only needed for graphics tasks
	std::optional<vk::RenderPass> mRenderPass;
	std::optional<vk::VertexInputBindingDescription> mVertexBindingDescription;
	std::optional<std::vector<vk::VertexInputAttributeDescription>> mVertexAttributeDescription;

	std::optional<vk::Framebuffer> mFrameBuffer;
	std::vector<vk::DescriptorSet> mDescSet;
};


class DeviceChild {
public:
    DeviceChild(RenderDevice* dev) : mDevice{dev} {}

    RenderDevice* getDevice() { return mDevice; }

    const RenderDevice* getDevice() const { return mDevice; }

private:
   RenderDevice*  mDevice;
};


#endif
