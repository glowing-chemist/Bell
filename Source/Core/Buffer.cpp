#include "Core/Buffer.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/BarrierManager.hpp"
#include "Core/BellLogging.hpp"
#include "Core/ConversionUtils.hpp"

#ifdef VULKAN
#include "Core/Vulkan/VulkanBuffer.hpp"
#endif

#ifdef DX_12
#include "Core/DX_12/DX_12Buffer.hpp"
#endif

BufferBase::BufferBase(RenderDevice* dev,
	   BufferUsage usage,
	   const uint32_t size,
	   const uint32_t stride,
	   const std::string &name) :
	GPUResource(dev->getCurrentSubmissionIndex()),
    DeviceChild{dev},
    mUsage{usage},
    mSize{size},
    mStride{stride},
    mName{name}
{
    BELL_ASSERT(size > 0 && stride > 0, "size and stride must be greater than 0")
}


void BufferBase::swap(BufferBase& other)
{
	BufferUsage Usage = mUsage;
	uint32_t Size = mSize;
	uint32_t Stride = mStride;
	uint32_t Allignment = mAllignment;
	std::string Name = mName;

	mUsage = other.mUsage;
	other.mUsage = Usage;

	mSize = other.mSize;
	other.mSize = Size;

	mStride = other.mStride;
	other.mStride = Stride;

	mAllignment = other.mAllignment;
	other.mAllignment = Allignment;

	mName = other.mName;
	other.mName = Name;
}


void BufferBase::updateLastAccessed()
{
    GPUResource::updateLastAccessed(getDevice()->getCurrentSubmissionIndex());
}


Buffer::Buffer(RenderDevice* dev,
	BufferUsage usage,
	const uint32_t size,
	const uint32_t stride,
	const std::string& name)
{
#ifdef VULKAN
	mBase = std::make_shared<VulkanBuffer>(dev, usage, size, stride, name);
#endif

#ifdef DX_12
	mBase = std::make_shared<DX_12Buffer>(dev, usage, size, stride, name);
#endif
}
