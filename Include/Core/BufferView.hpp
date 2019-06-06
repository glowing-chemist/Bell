#ifndef BUFFERVIEW_HPP
#define BUFFERVIEW_HPP

#include "Buffer.hpp"

#include <limits>

#include <vulkan/vulkan.hpp>

class BarrierRecorder;

constexpr uint32_t kWholeBufferLength = std::numeric_limits<uint32_t>::max();


class BufferView : public DeviceChild
{
    friend BarrierRecorder;
public:

    BufferView(Buffer&, const uint32_t offset = 0, const uint32_t size = kWholeBufferLength);
    // buffer views don't manage any resources so can have a trivial destructor.
    ~BufferView() = default;

    vk::Buffer getBuffer() const
    { return mBufferHandle; }

    uint32_t getOffset() const
    { return mOffset; }

    uint32_t getSize() const
    { return mSize; }

    vk::BufferUsageFlags getUsage() const
    { return mUsage; }


private:

    uint32_t mOffset;
    uint32_t mSize;

    vk::Buffer mBufferHandle;
    Allocation mBufferMemory;
    vk::BufferUsageFlags mUsage;

};

#endif
