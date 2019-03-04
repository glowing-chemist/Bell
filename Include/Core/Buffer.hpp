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
           std::string = "");

    ~Buffer();

	vk::Buffer getBuffer() const
		{ return mBuffer; }

    void setCurrentOffset(const uint64_t offset)
        { mCurrentOffset = offset; }
    uint64_t getCurrentOffset() const
        { return mCurrentOffset; }

    Allocation getMemory()
        { return mBufferMemory; }

    void setContents(const void* data,
                     const uint32_t size,
                     const uint32_t offset = 0);


    void*   map();
    void    unmap();

private:
    vk::Buffer mBuffer;
    Allocation mBufferMemory;

    uint64_t mCurrentOffset;

    vk::BufferUsageFlags mUsage;
    uint32_t mSize;
    uint32_t mStride;
    uint32_t mAllignment;
    std::string mName;
};



#endif
