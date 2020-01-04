#include "OpenGLBufferView.hpp"
#include "OpenGLBuffer.hpp"


OpenGLBufferView::OpenGLBufferView(Buffer& buf, const uint64_t offset, const uint64_t size) :
	BufferViewBase(buf, offset, size),
	mBufferHandle(-1)
{
	OpenGLBuffer* GLBuffer = static_cast<OpenGLBuffer*>(buf.getBase());
	mBufferHandle = GLBuffer->getBuffer();
}
