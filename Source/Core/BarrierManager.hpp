#ifndef BARRIERMANAGER_HPP
#define BARRIERMANAGER_HPP

#include "DeviceChild.hpp"
#include "GPUResource.hpp"

#include <vector>
#include <tuple>

#include <vulkan/vulkan.hpp>

class Image;
class Buffer;

class BarrierManager : public DeviceChild
{
public:
	BarrierManager(RenderDevice*);

	void transferResourceToQueue(Image&, const QueueType);
	void transferResourceToQueue(Buffer&, const QueueType);

	void transitionImageLayout(Image&, const vk::ImageLayout);

	void flushAllBarriers();

private:

	std::vector<std::pair<QueueType, vk::ImageMemoryBarrier>> mImageMemoryBarriers;
	std::vector<std::pair<QueueType, vk::BufferMemoryBarrier>> mBufferMemoryBarriers;
};

#endif
