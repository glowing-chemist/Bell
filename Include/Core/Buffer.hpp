#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <vulkan/vulkan.hpp>
#include <string>

#include "Core/GPUResource.hpp"
#include "Core/DeviceChild.hpp"
#include "Core/MemoryManager.hpp"
#include "Core/BarrierManager.hpp"


class RenderDevice;


class Buffer : public GPUResource, public DeviceChild
{
public:
    Buffer(RenderDevice*,
           vk::BufferUsageFlags,
           const uint32_t,
           std::string = "");

    ~Buffer();


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
