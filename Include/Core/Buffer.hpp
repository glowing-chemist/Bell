#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <vulkan/vulkan.hpp>
#include <string>

#include "Core/GPUResource.hpp"
#include "Core/DeviceChild.hpp"
#include "Core/MemoryManager.hpp"

#include "Engine/PassTypes.hpp"

class RenderDevice;


class Buffer final : public GPUResource, public DeviceChild
{
public:
    Buffer(RenderDevice* dev,
           BufferUsage usage,
           const uint32_t size,
           const uint32_t stride,
		   const std::string& = "");

    ~Buffer();

	Buffer& operator=(const Buffer&);
    Buffer(const Buffer&) = default;

	Buffer& operator=(Buffer&&);
	Buffer(Buffer&&);

	void swap(Buffer&);

    // will only resize to a larger buffer.
    // returns whether or not the buffer was resized (and views need to be recreated)
    bool resize(const uint32_t newSize, const bool preserContents);

	vk::Buffer getBuffer() const
		{ return mBuffer; }

    BufferUsage getUsage() const
        { return mUsage; }

	Allocation getMemory() const
        { return mBufferMemory; }

	uint32_t getSize() const
		{ return mSize;}

    void setContents(const void* data,
                     const uint32_t size,
                     const uint32_t offset = 0);

    // Repeat the data in the range (start + offset, start + offset + size]
    void setContents(const int data,
                     const uint32_t size,
                     const uint32_t offset = 0);


	void*   map(MapInfo &mapInfo);
    void    unmap();

    void    updateLastAccessed();

private:

    void resizePreserveContents(const uint32_t);
    void resizeDiscardContents(const uint32_t);

    inline bool isMappable() const
    {
        return static_cast<bool>(mUsage & BufferUsage::TransferSrc ||
        mUsage & BufferUsage::Uniform);
    }

    vk::Buffer mBuffer;
    Allocation mBufferMemory;
	MapInfo mCurrentMap;

    BufferUsage mUsage;
    uint32_t mSize;
    uint32_t mStride;
    uint32_t mAllignment;
    std::string mName;
};



#endif
