#ifndef GL_BUFFER_HPP
#define GL_BUFFER_HPP

#include "Core/Buffer.hpp"

class OpenGLBuffer : public BufferBase
{
public:
	OpenGLBuffer(RenderDevice* dev,
		BufferUsage usage,
		const uint32_t size,
		const uint32_t stride,
		const std::string& = "");

	~OpenGLBuffer();

	virtual void swap(BufferBase&) override;

	virtual bool resize(const uint32_t newSize, const bool preserContents) override;

	virtual void setContents(const void* data,
		const uint32_t size,
		const uint32_t offset = 0) override;

	// Repeat the data in the range (start + offset, start + offset + size]
	virtual void setContents(const int data,
		const uint32_t size,
		const uint32_t offset = 0) override;


	virtual void* map(MapInfo& mapInfo) override;
	virtual void unmap() override;

	uint32_t getBuffer() const
	{
		return mBuffer;
	}

private:

	MapInfo mCurrentMap;

	uint32_t mBuffer;
	uint32_t mSlot;
	uint32_t mUsageFlags;

};

#endif