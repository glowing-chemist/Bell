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
    Buffer(RenderDevice*,
           vk::BufferUsageFlags,
           const uint32_t,
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



    void*   map();
    void    unmap();

private:
    vk::Buffer mBuffer;
    Allocation mBufferMemory;

    uint64_t mCurrentOffset;

    vk::BufferUsageFlags mUsage;
    uint32_t mSize;
    std::string mName;
};



#endif
