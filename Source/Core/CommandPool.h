#ifndef COMMANDPOOL_HPP
#define COMMANDPOOL_HPP

#include "DeviceChild.hpp"
#include "GPUResource.hpp"

#include <vulkan/vulkan.hpp>
#include <vector>

class RenderDevice;


class CommandPool : public DeviceChild
{
public:
	CommandPool(RenderDevice*);
	~CommandPool();

	vk::CommandBuffer& getBufferForQueue(const QueueType, const uint32_t = 0);
	uint32_t		   getNumberOfBuffersForQueue(const QueueType);

	void			   reset();

private:
	vk::CommandPool mGraphicsPool;
	std::vector<vk::CommandBuffer> mGraphicsBuffers;
	
	vk::CommandPool mComputePool;
	std::vector<vk::CommandBuffer> mComputeBuffers;
	
	vk::CommandPool mTransferPool;
	std::vector<vk::CommandBuffer> mTransferBuffers;
};

#endif
