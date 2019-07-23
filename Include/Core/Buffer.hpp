#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <vulkan/vulkan.hpp>
#include <string>

#include "Core/GPUResource.hpp"
#include "Core/DeviceChild.hpp"
#include "Core/MemoryManager.hpp"


class RenderDevice;


class Buffer final : public GPUResource, public DeviceChild
{
public:
    Buffer(RenderDevice* dev,
           vk::BufferUsageFlags usage,
           const uint32_t size,
           const uint32_t stride,
		   const std::string& = "");

    ~Buffer();

	Buffer& operator=(const Buffer&);
    Buffer(const Buffer&) = default;

	Buffer& operator=(Buffer&&);
	Buffer(Buffer&&);

	void swap(Buffer&);

	vk::Buffer getBuffer() const
		{ return mBuffer; }

    vk::BufferUsageFlags getUsage() const
        { return mUsage; }

	Allocation getMemory() const
        { return mBufferMemory; }

	uint32_t getSize() const
		{ return mSize;}

    void setContents(const void* data,
                     const uint32_t size,
                     const uint32_t offset = 0);


	void*   map(MapInfo &mapInfo);
    void    unmap();

private:
    vk::Buffer mBuffer;
    Allocation mBufferMemory;
	MapInfo mCurrentMap;

    vk::BufferUsageFlags mUsage;
    uint32_t mSize;
    uint32_t mStride;
    uint32_t mAllignment;
    std::string mName;
};



#endif
