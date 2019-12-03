#include "Core/BufferView.hpp"
#include "Core/RenderDevice.hpp"


BufferView::BufferView(Buffer& parentBuffer, const uint64_t offset, const uint64_t size) :
	DeviceChild{parentBuffer.getDevice()},
	mOffset{offset},
	mSize{size},
	mBufferHandle{parentBuffer.getBuffer()},
	mBufferMemory{parentBuffer.getMemory()},
	mUsage{parentBuffer.getUsage()}
{
}
