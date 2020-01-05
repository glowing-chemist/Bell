#include "BarrierManager.hpp"
#include "Core/Image.hpp"
#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"
#include "Core/ConversionUtils.hpp"
#include "RenderDevice.hpp"

#ifdef VULKAN
#include "Core/Vulkan/VulkanBarrierManager.hpp"
#endif

#include <algorithm>


BarrierRecorderBase::BarrierRecorderBase(RenderDevice* device) :
	DeviceChild(device),
	mSrc{SyncPoint::TopOfPipe},
	mDst{ SyncPoint::BottomOfPipe }
{}


void BarrierRecorderBase::updateSyncPoints(const SyncPoint src, const SyncPoint dst)
{
	mSrc = static_cast<SyncPoint>(std::max(static_cast<uint32_t>(mSrc), static_cast<uint32_t>(src)));
	mDst = static_cast<SyncPoint>(std::min(static_cast<uint32_t>(mDst), static_cast<uint32_t>(dst)));
}


BarrierRecorder::BarrierRecorder(RenderDevice* dev)
{
#ifdef VULKAN
	mBase = std::make_shared<VulkanBarrierRecorder>(dev);
#endif
}
