#ifndef BARRIERMANAGER_HPP
#define BARRIERMANAGER_HPP

#include "GPUResource.hpp"
#include "DeviceChild.hpp"

#include <vector>
#include <tuple>

#include <vulkan/vulkan.hpp>

class Image;
class ImageView;
class Buffer;
class BufferView;

class BarrierRecorder : DeviceChild
{
public:
	BarrierRecorder(RenderDevice* device);

	void transferResourceToQueue(Image&, const QueueType);
	void transferResourceToQueue(Buffer&, const QueueType);

	void transitionImageLayout(Image&, const vk::ImageLayout);
    void transitionImageLayout(ImageView&, const vk::ImageLayout);

    void makeContentsVisible(const Image&);
    void makeContentsVisible(const ImageView&);
    void makeContentsVisible(const Buffer&);
	void makeContentsVisible(const BufferView&);

	std::vector<vk::ImageMemoryBarrier> getImageBarriers(QueueType);
	std::vector<vk::BufferMemoryBarrier> getBufferBarriers(QueueType);

	bool empty() const 
	{ return mImageMemoryBarriers.empty() && mBufferMemoryBarriers.empty(); }

private:

	std::vector<std::pair<QueueType, vk::ImageMemoryBarrier>> mImageMemoryBarriers;
	std::vector<std::pair<QueueType, vk::BufferMemoryBarrier>> mBufferMemoryBarriers;
};

#endif
