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

    void makeContentsVisibleToCompute(const Image&);
    void makeContentsVisibleToCompute(const ImageView&);
    void makeContentsVisibleToCompute(const Buffer&);
	void makeContentsVisibleToCompute(const BufferView&);

	void makeContentsVisibleToGraphics(const Image&);
	void makeContentsVisibleToGraphics(const ImageView&);
	void makeContentsVisibleToGraphics(const Buffer&);
	void makeContentsVisibleToGraphics(const BufferView&);

	void makeContentsVisibleFromTransfer(const Image&);
	void makeContentsVisibleFromTransfer(const ImageView&);
	void makeContentsVisibleFromTransfer(const Buffer&);
	void makeContentsVisibleFromTransfer(const BufferView&);

	void memoryBarrierComputeToCompute();

	std::vector<vk::ImageMemoryBarrier> getImageBarriers(QueueType);
	std::vector<vk::BufferMemoryBarrier> getBufferBarriers(QueueType);

	bool empty() const 
	{ return mImageMemoryBarriers.empty() && mBufferMemoryBarriers.empty(); }

	vk::PipelineStageFlags getSource() const
	{ return mSrc; }

	vk::PipelineStageFlags getDestination() const
	{ return mDst; }

private:

	void makeContentsVisible(const Image&);
	void makeContentsVisible(const ImageView&);

	std::vector<std::pair<QueueType, vk::ImageMemoryBarrier>> mImageMemoryBarriers;
	std::vector<std::pair<QueueType, vk::BufferMemoryBarrier>> mBufferMemoryBarriers;
	std::vector<std::pair<QueueType, vk::MemoryBarrier>> mMemoryBarriers;

	vk::PipelineStageFlags mSrc;
	vk::PipelineStageFlags mDst;
};

#endif
