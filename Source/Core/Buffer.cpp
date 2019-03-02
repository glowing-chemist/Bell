#include "Core/Buffer.hpp"
#include "RenderDevice.hpp"

Buffer::Buffer(RenderDevice* dev,
       vk::BufferUsageFlags usage,
       const uint32_t size,
       std::string name) :
    GPUResource{dev->getCurrentFrameIndex()},
    DeviceChild{dev},
    mUsage{usage},
    mSize{size},
    mName{name}
{
    mBuffer = getDevice()->createBuffer(mSize, mUsage);
    const vk::MemoryRequirements bufferMemReqs = getDevice()->getMemoryRequirements(mBuffer);
    mBufferMemory = getDevice()->getMemoryManager()->Allocate(mSize, bufferMemReqs.alignment, static_cast<bool>(mUsage & vk::BufferUsageFlagBits::eTransferSrc));
}


Buffer::~Buffer()
{
    const bool shouldDestroy = release();
    if(shouldDestroy)
        getDevice()->destroyBuffer(*this);
}


void*   Buffer::map()
{
    return getDevice()->getMemoryManager()->MapAllocation(mBufferMemory);
}


void    Buffer::unmap()
{
    getDevice()->getMemoryManager()->UnMapAllocation(mBufferMemory);
}

