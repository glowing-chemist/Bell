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
    mSrc{0},
    mDst{0}
{}


void BarrierRecorderBase::updateSyncPoints(const SyncPoint src, const SyncPoint dst)
{
    mSrc = mSrc | src;
    mDst = mDst | dst;
}


BarrierRecorder::BarrierRecorder(RenderDevice* dev)
{
#ifdef VULKAN
	mBase = std::make_shared<VulkanBarrierRecorder>(dev);
#endif
}
