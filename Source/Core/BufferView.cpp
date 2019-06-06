#include "Core/BufferView.hpp"
#include "Core/RenderDevice.hpp"


BufferView::BufferView(Buffer& parentBuffer, const uint32_t offset, const uint32_t size) :
	DeviceChild{parentBuffer.getDevice()},
	mOffset{offset},
	mSize{size},
	mBufferHandle{parentBuffer.getBuffer()},
	mBufferMemory{parentBuffer.getMemory()},
	mUsage{parentBuffer.getUsage()}
{
	// If no explicit range was given use the entire buffer.
	if(mSize == kWholeBufferLength)
		mSize = parentBuffer.getSize();
}

