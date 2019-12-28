#include "Core/BufferView.hpp"
#include "Core/RenderDevice.hpp"

#ifdef VULKAN
#include "Core/Vulkan/VulkanBufferView.hpp"
#endif

BufferViewBase::BufferViewBase(Buffer& parentBuffer, const uint64_t offset, const uint64_t size) :
	DeviceChild{parentBuffer->getDevice()},
	mOffset{offset},
	mSize{size},
	mUsage{parentBuffer->getUsage()}
{
}


BufferView::BufferView(Buffer& buffer, const uint64_t offset, const uint64_t size)
{
#ifdef VULKAN
	mBase = std::make_shared<VulkanBufferView>(buffer, offset, size);
#endif
}
