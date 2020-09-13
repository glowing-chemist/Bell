#ifndef VK_BARRIER_MANAGER_HPP
#define VK_BARRIER_MANAGER_HPP

#include "Core/BarrierManager.hpp"
#include "VulkanRenderDevice.hpp"


class VulkanBarrierRecorder : public BarrierRecorderBase
{
public:
	VulkanBarrierRecorder(RenderDevice* device);

	virtual void transferResourceToQueue(Image&, const QueueType, const Hazard, const SyncPoint src, const SyncPoint dst) override;
	virtual void transferResourceToQueue(Buffer&, const QueueType, const Hazard, const SyncPoint src, const SyncPoint dst) override;

    virtual void memoryBarrier(Image& img, const Hazard, const SyncPoint src, const SyncPoint dst) override;
    virtual void memoryBarrier(ImageView& img, const Hazard, const SyncPoint src, const SyncPoint dst) override;
    virtual void memoryBarrier(Buffer& img, const Hazard, const SyncPoint src, const SyncPoint dst) override;
    virtual void memoryBarrier(BufferView& img, const Hazard, const SyncPoint src, const SyncPoint dst) override;
    virtual void memoryBarrier(const Hazard, const SyncPoint src, const SyncPoint dst) override;

    virtual void transitionLayout(Image& img, const ImageLayout, const Hazard, const SyncPoint src, const SyncPoint dst) override;
    virtual void transitionLayout(ImageView& img, const ImageLayout, const Hazard, const SyncPoint src, const SyncPoint dst) override;

    virtual void signalAsyncQueueSemaphore(const uint64_t val) override;
    virtual void waitOnAsyncQueueSemaphore(const uint64_t val) override;

	std::vector<vk::ImageMemoryBarrier> getImageBarriers(const QueueType) const;
	std::vector<vk::BufferMemoryBarrier> getBufferBarriers(const QueueType) const;
	std::vector<vk::MemoryBarrier> getMemoryBarriers(const QueueType) const;

	bool empty() const
	{
		return mImageMemoryBarriers.empty() && mBufferMemoryBarriers.empty() && mMemoryBarriers.empty();
	}

    struct SemaphoreSignalInfo
    {
        bool mWriteSemaphore;
        uint64_t mValue;
    };

    const std::vector<SemaphoreSignalInfo>& getSemaphoreSignalInfo() const
    {
        return mSemaphoreOps;
    }


private:
	std::vector<std::pair<QueueType, vk::ImageMemoryBarrier>> mImageMemoryBarriers;
	std::vector<std::pair<QueueType, vk::BufferMemoryBarrier>> mBufferMemoryBarriers;
	std::vector<std::pair<QueueType, vk::MemoryBarrier>> mMemoryBarriers;
    std::vector<SemaphoreSignalInfo>                    mSemaphoreOps;
};

#endif
