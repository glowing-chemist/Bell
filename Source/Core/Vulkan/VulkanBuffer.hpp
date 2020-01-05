#ifndef VK_BUFFER_HPP
#define VK_BUFFER_HPP

#include <vulkan/vulkan.hpp>
#include "Core/Buffer.hpp"
#include "MemoryManager.hpp"


class VulkanBuffer : public BufferBase
{
public:

	VulkanBuffer(RenderDevice* dev,
		BufferUsage usage,
		const uint32_t size,
		const uint32_t stride,
		const std::string & = "");

	~VulkanBuffer();

	virtual void swap(BufferBase&) override;

	// will only resize to a larger buffer.
	// returns whether or not the buffer was resized (and views need to be recreated)
	virtual bool resize(const uint32_t newSize, const bool preserContents) override;

	virtual void setContents(const void* data,
		const uint32_t size,
		const uint32_t offset = 0) override;

	// Repeat the data in the range (start + offset, start + offset + size]
	virtual void setContents(const int data,
		const uint32_t size,
		const uint32_t offset = 0) override;


	virtual void* map(MapInfo& mapInfo) override;
	virtual void    unmap() override;

	vk::Buffer getBuffer() const
	{
		return mBuffer;
	}

	Allocation getMemory() const
	{
		return mBufferMemory;
	}

private:

	void resizePreserveContents(const uint32_t newSize);
	void resizeDiscardContents(const uint32_t newSize);

	vk::Buffer mBuffer;
	Allocation mBufferMemory;
	MapInfo mCurrentMap;
};

#endif
