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


enum class SyncPoint
{
	TopOfPipe = 0,
	IndirectArgs = 1,
	TransferSource = 2,
	TransferDestination = 3,
	VertexInput = 4,
	VertexShader = 5,
	FragmentShader = 6,
	FragmentShaderOutput = 7,
	ComputeShader = 8,
	BottomOfPipe = 9
};


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
