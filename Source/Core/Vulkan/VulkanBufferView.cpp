#include "VulkanBufferView.hpp"
#include "VulkanBuffer.hpp"
#include "Core/RenderDevice.hpp"


VulkanBufferView::VulkanBufferView(Buffer& parentBuffer, const uint64_t offset, const uint64_t size) :
	BufferViewBase(parentBuffer, offset, size),
	mBufferHandle{static_cast<VulkanBuffer&>(*parentBuffer.getBase()).getBuffer()},
	mBufferMemory{static_cast<VulkanBuffer&>(*parentBuffer.getBase()).getMemory()}
{
}
