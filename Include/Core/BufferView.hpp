#ifndef BUFFERVIEW_HPP
#define BUFFERVIEW_HPP

#include "Buffer.hpp"

#include <limits>

#include <vulkan/vulkan.hpp>

class BarrierRecorder;

class BufferView : public DeviceChild
{
    friend BarrierRecorder;
public:

    BufferView(Buffer&, const uint64_t offset = 0, const uint64_t size = VK_WHOLE_SIZE);
    // buffer views don't manage any resources so can have a trivial destructor.
    ~BufferView() = default;

    vk::Buffer getBuffer() const
    { return mBufferHandle; }

    uint64_t getOffset() const
    { return mOffset; }

    uint64_t getSize() const
    { return mSize; }

    vk::BufferUsageFlags getUsage() const
    { return mUsage; }


private:

    uint64_t mOffset;
    uint64_t mSize;

    vk::Buffer mBufferHandle;
    Allocation mBufferMemory;
    vk::BufferUsageFlags mUsage;

};

#endif
