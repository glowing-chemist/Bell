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


enum class SyncPoint
{
	TopOfPipe = 0,
	TransferSource = 1,
	TransferDestination = 2,
	VertexInput = 3,
	VertexShader = 4,
	FragmentShader = 5,
	FragmentShaderOutput = 6,
	ComputeShader = 7,
	BottomOfPipe = 8
};


enum class Hazard
{
	WriteAfterRead,
	ReadAfterWrite
};


class BarrierRecorder : DeviceChild
{
public:
	BarrierRecorder(RenderDevice* device);

	void transferResourceToQueue(Image&, const QueueType, const Hazard, const SyncPoint src, const SyncPoint dst);
	void transferResourceToQueue(Buffer&, const QueueType, const Hazard, const SyncPoint src, const SyncPoint dst);

	void memoryBarrier(Image& img, const Hazard, const SyncPoint src, const SyncPoint dst);
	void memoryBarrier(ImageView& img, const Hazard, const SyncPoint src, const SyncPoint dst);
	void memoryBarrier(Buffer& img, const Hazard, const SyncPoint src, const SyncPoint dst);
	void memoryBarrier(BufferView& img, const Hazard, const SyncPoint src, const SyncPoint dst);
	void memoryBarrier(const Hazard, const SyncPoint src, const SyncPoint dst);

	void transitionLayout(Image& img, const ImageLayout, const Hazard, const SyncPoint src, const SyncPoint dst);
	void transitionLayout(ImageView& img, const ImageLayout, const Hazard, const SyncPoint src, const SyncPoint dst);

	std::vector<vk::ImageMemoryBarrier> getImageBarriers(const QueueType) const;
	std::vector<vk::BufferMemoryBarrier> getBufferBarriers(const QueueType) const;
	std::vector<vk::MemoryBarrier> getMemoryBarriers(const QueueType) const;

	bool empty() const 
	{ return mImageMemoryBarriers.empty() && mBufferMemoryBarriers.empty() && mMemoryBarriers.empty(); }

	SyncPoint getSource() const
	{ return mSrc; }

	SyncPoint getDestination() const
	{ return mDst; }

private:

	void updateSyncPoints(const SyncPoint src, const SyncPoint dst);

	std::vector<std::pair<QueueType, vk::ImageMemoryBarrier>> mImageMemoryBarriers;
	std::vector<std::pair<QueueType, vk::BufferMemoryBarrier>> mBufferMemoryBarriers;
	std::vector<std::pair<QueueType, vk::MemoryBarrier>> mMemoryBarriers;

	SyncPoint mSrc;
	SyncPoint mDst;
};

#endif
