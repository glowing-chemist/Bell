#ifndef BARRIERMANAGER_HPP
#define BARRIERMANAGER_HPP

#include "GPUResource.hpp"
#include "DeviceChild.hpp"

#include <vector>
#include <tuple>


class Image;
class ImageView;
class Buffer;
class BufferView;


enum class SyncPoint : uint32_t
{
    TopOfPipe = 1,
    IndirectArgs = 1 << 1,
    CommandPredication = 1 << 2,
    VertexInput = 1 << 3,
    VertexShader = 1 << 4,
    FragmentShader = 1 << 5,
    FragmentShaderOutput = 1 << 6,
    ComputeShader = 1 << 7,
    TransferSource = 1 << 8,
    TransferDestination = 1 << 9,
    BottomOfPipe = 1 << 10
};

inline SyncPoint operator|(const SyncPoint& lhs, const SyncPoint& rhs)
{
    return static_cast<SyncPoint>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline uint32_t operator&(const SyncPoint& lhs, const SyncPoint& rhs)
{
    return static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs);
}

enum class Hazard
{
	WriteAfterRead,
	ReadAfterWrite
};


class BarrierRecorderBase : public DeviceChild
{
public:
	BarrierRecorderBase(RenderDevice* device);

	virtual void transferResourceToQueue(Image&, const QueueType, const Hazard, const SyncPoint src, const SyncPoint dst) = 0;
	virtual void transferResourceToQueue(Buffer&, const QueueType, const Hazard, const SyncPoint src, const SyncPoint dst) = 0;

    virtual void memoryBarrier(Image& img, const Hazard, const SyncPoint src, const SyncPoint dst) = 0;
    virtual void memoryBarrier(ImageView& img, const Hazard, const SyncPoint src, const SyncPoint dst) = 0;
    virtual void memoryBarrier(Buffer& img, const Hazard, const SyncPoint src, const SyncPoint dst) = 0;
    virtual void memoryBarrier(BufferView& img, const Hazard, const SyncPoint src, const SyncPoint dst) = 0;
    virtual void memoryBarrier(const Hazard, const SyncPoint src, const SyncPoint dst) = 0;

    virtual void transitionLayout(Image& img, const ImageLayout, const Hazard, const SyncPoint src, const SyncPoint dst) = 0;
    virtual void transitionLayout(ImageView& img, const ImageLayout, const Hazard, const SyncPoint src, const SyncPoint dst) = 0;

    virtual void signalAsyncQueueSemaphore(const uint64_t val) = 0;
    virtual void waitOnAsyncQueueSemaphore(const uint64_t val) = 0;

	SyncPoint getSource() const
	{ return mSrc; }

	SyncPoint getDestination() const
	{ return mDst; }

protected:

	void updateSyncPoints(const SyncPoint src, const SyncPoint dst);

	SyncPoint mSrc;
	SyncPoint mDst;
};


class BarrierRecorder
{
public:

	BarrierRecorder(RenderDevice*);
	~BarrierRecorder() = default;


	BarrierRecorderBase* getBase()
	{
		return mBase.get();
	}

	const BarrierRecorderBase* getBase() const
	{
		return mBase.get();
	}

	BarrierRecorderBase* operator->()
	{
		return getBase();
	}

	const BarrierRecorderBase* operator->() const
	{
		return getBase();
	}

private:

	std::shared_ptr<BarrierRecorderBase> mBase;

};

#endif
