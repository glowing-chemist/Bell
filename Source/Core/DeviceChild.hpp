#ifndef DEVICECHILD_HPP
#define DEVICECHILD_HPP

#include <optional>
#include <vulkan/vulkan.hpp>

class RenderDevice;

// Put this here for now to avoid cyclic header dependancies.
struct vulkanResources
{
	vk::Pipeline mPipeline;
	vk::PipelineLayout mPipelineLayout;
	vk::DescriptorSetLayout mDescSetLayout;
	// Only needed for graphics tasks
	std::optional<vk::RenderPass> mRenderPass;
	std::optional<vk::VertexInputBindingDescription> mVertexBindingDescription;
	std::optional<std::vector<vk::VertexInputAttributeDescription>> mVertexAttributeDescription;

	std::optional<vk::Framebuffer> mFrameBuffer;
	vk::DescriptorSet mDescSet;
	bool mDescriptorsWritten;
};


class DeviceChild {
public:
    DeviceChild(RenderDevice* dev) : mDevice{dev} {}

    RenderDevice* getDevice() { return mDevice; }

private:
   RenderDevice*  mDevice;
};


#endif
