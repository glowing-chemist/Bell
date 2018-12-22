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


    void*   map();
    void    unmap();

private:
    vk::Buffer mBuffer;
    Allocation mBufferMemory;

    vk::BufferUsageFlags mUsage;
    uint32_t mSize;
    std::string mName;
};



#endif
