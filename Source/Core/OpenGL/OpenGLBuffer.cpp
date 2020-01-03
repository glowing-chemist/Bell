#include "OpenGLBuffer.hpp"
#include "OpenGLRenderDevice.hpp"

#include "glad/glad.h"


OpenGLBuffer::OpenGLBuffer(RenderDevice* dev,
	BufferUsage usage,
	const uint32_t size,
	const uint32_t stride,
	const std::string& name) :
	BufferBase(dev, usage, size, stride, name)
{
	glGenBuffers(1, &mBuffer);
	mUsageFlags = 0;

	if (usage & BufferUsage::Vertex)
	{
		mSlot = GL_ARRAY_BUFFER;
		mUsageFlags |= GL_STREAM_DRAW;
	}
	else if (usage & BufferUsage::Index)
	{
		mSlot = GL_ELEMENT_ARRAY_BUFFER;
		mUsageFlags |= GL_STREAM_DRAW;
	}
	else if (usage & BufferUsage::DataBuffer)
	{
		mSlot = GL_SHADER_STORAGE_BUFFER;
		mUsageFlags |= GL_STREAM_READ;
	}
	else if (usage & BufferUsage::IndirectArgs)
	{
		mSlot = GL_DISPATCH_INDIRECT_BUFFER;
		mUsageFlags |= GL_STREAM_DRAW;
	}
	else if (usage & BufferUsage::Uniform)
	{
		mSlot = GL_UNIFORM_BUFFER;
		mUsageFlags |= GL_DYNAMIC_READ;
	}
	else if (usage & BufferUsage::TransferDest)
	{
		mSlot = GL_COPY_WRITE_BUFFER;
		mUsageFlags |= GL_STATIC_DRAW;
	}
	else if (usage & BufferUsage::TransferSrc)
	{
		mSlot = GL_COPY_READ_BUFFER;
		mUsageFlags |= GL_STATIC_DRAW;
	}

	glBindBuffer(mSlot, mBuffer);

	glBufferData(mSlot, size, nullptr, mUsageFlags);
}


OpenGLBuffer::~OpenGLBuffer()
{
	getDevice()->destroyBuffer(*this);
}


void OpenGLBuffer::setContents(const void* data,
	const uint32_t size,
	const uint32_t offset)
{
	glBindBuffer(mSlot, mBuffer);

	glBufferSubData(mSlot, offset, size, data);

}


void OpenGLBuffer::setContents(const int data,
	const uint32_t size,
	const uint32_t offset)
{
	glBindBuffer(mSlot, mBuffer);
}


void* OpenGLBuffer::map(MapInfo& mapInfo)
{
	glBindBuffer(mSlot, mBuffer);
	mCurrentMap = mapInfo;

	return glMapBufferRange(mSlot, mapInfo.mOffset, mapInfo.mSize, GL_MAP_WRITE_BIT);
}


void OpenGLBuffer::unmap()
{
	glBindBuffer(mSlot, mBuffer);

	glFlushMappedBufferRange(mSlot, mCurrentMap.mOffset, mCurrentMap.mSize);
	glUnmapBuffer(mSlot);
}


void OpenGLBuffer::swap(BufferBase& base)
{
	OpenGLBuffer& GLBuffer = static_cast<OpenGLBuffer&>(base);

	BufferBase::swap(base);

	uint32_t tempBuffer = mBuffer;
	uint32_t tempSlot = mSlot;
	MapInfo  tempMap = mCurrentMap;

	mBuffer = GLBuffer.mBuffer;
	mSlot = GLBuffer.mSlot;
	mCurrentMap = GLBuffer.mCurrentMap;

	GLBuffer.mBuffer = tempBuffer;
	GLBuffer.mSlot = tempSlot;
	GLBuffer.mCurrentMap = tempMap;
}


bool OpenGLBuffer::resize(const uint32_t newSize, const bool preserContents)
{
	glBindBuffer(mSlot, mBuffer);

	if(!preserContents)
		glBufferData(mSlot, newSize, nullptr, mUsageFlags);
	else
	{
		BELL_TRAP; // currently not implemented.
	}

	return true;
}
