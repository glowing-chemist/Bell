#include "Core/Buffer.hpp"
#include "RenderDevice.hpp"

Buffer::Buffer(RenderDevice* dev,
       vk::BufferUsageFlags usage,
       const uint32_t size,
       const uint32_t stride,
       std::string name) :
    GPUResource{dev->getCurrentFrameIndex()},
    DeviceChild{dev},
    mUsage{usage},
    mSize{size},
    mStride{stride},
    mName{name}
{
    const uint32_t entries = size / stride;
    mAllignment = mUsage & vk::BufferUsageFlagBits::eUniformBuffer ?
                                   getDevice()->getLimits().minUniformBufferOffsetAlignment : 1;

    mBuffer = getDevice()->createBuffer(entries * mAllignment, mUsage);
    const vk::MemoryRequirements bufferMemReqs = getDevice()->getMemoryRequirements(mBuffer);
    mBufferMemory = getDevice()->getMemoryManager()->Allocate(mSize, bufferMemReqs.alignment, static_cast<bool>(mUsage & vk::BufferUsageFlagBits::eTransferSrc));
    getDevice()->getMemoryManager()->BindBuffer(mBuffer, mBufferMemory);
}


Buffer::~Buffer()
{
    const bool shouldDestroy = release();
    if(shouldDestroy)
        getDevice()->destroyBuffer(*this);
}


void Buffer::setContents(const void* data, const uint32_t size, const uint32_t offset)
{
    const uint32_t entries = size / mStride;

    Buffer stagingBuffer = Buffer(getDevice(), vk::BufferUsageFlagBits::eTransferSrc, size * mAllignment, mStride, "Staging Buffer");
    void* mappedBuffer = stagingBuffer.map();

    for(uint32_t i = 0; i < entries; ++i)
    {
        std::memcpy(reinterpret_cast<char*>(mappedBuffer) + (i * mAllignment),
                    reinterpret_cast<const char*>(data) + (i * mStride),
                    mStride);
    }

    stagingBuffer.unmap();

    vk::BufferCopy copyInfo{};
    copyInfo.setSize(size * mAllignment);
    copyInfo.setSrcOffset(0);
    copyInfo.setDstOffset(offset);

    getDevice()->getCurrentCommandPool()->getBufferForQueue(QueueType::Graphics)
            .copyBuffer(stagingBuffer.getBuffer(), getBuffer(), copyInfo);

    // Maybe try to implement a staging buffer cache so that we don't have to create one
    // each time.
    stagingBuffer.updateLastAccessed(getDevice()->getCurrentSubmissionIndex());
    getDevice()->destroyBuffer(stagingBuffer);
}


void*   Buffer::map()
{
    return getDevice()->getMemoryManager()->MapAllocation(mBufferMemory);
}


void    Buffer::unmap()
{
    getDevice()->getMemoryManager()->UnMapAllocation(mBufferMemory);
}

