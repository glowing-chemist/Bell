#ifndef COMMANDPOOL_HPP
#define COMMANDPOOL_HPP

#include "Core/DeviceChild.hpp"
#include "Core/GPUResource.hpp"

#include <vulkan/vulkan.hpp>
#include <vector>

class RenderDevice;

class CommandPool : public DeviceChild
{
public:
    CommandPool(RenderDevice*, const QueueType);
	~CommandPool();

    vk::CommandBuffer& getBufferForQueue(const uint32_t = 0);
    uint32_t		   getNumberOfBuffersForQueue();

    void               reserve(const uint32_t);

	void			   reset();

private:
    std::vector<vk::CommandBuffer> allocateCommandBuffers(const uint32_t, const bool);
    const vk::CommandPool& getCommandPool() const;
    std::vector<vk::CommandBuffer>& getCommandBuffers();

    vk::CommandPool mPool;
    std::vector<vk::CommandBuffer> mCmdBuffers;
};

#endif
