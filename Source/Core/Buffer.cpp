#include "Core/Buffer.hpp"
#include "RenderDevice.hpp"
#include "Core/BarrierManager.hpp"

Buffer::Buffer(RenderDevice* dev,
	   vk::BufferUsageFlags usage,
	   const uint32_t size,
	   const uint32_t stride,
	   const std::string &name) :
    GPUResource{dev->getCurrentSubmissionIndex()},
    DeviceChild{dev},
    mCurrentOffset{0},
    mUsage{usage},
    mSize{size},
    mStride{stride},
    mName{name}
{
    const uint32_t entries = size / stride;
    mAllignment = mUsage & vk::BufferUsageFlagBits::eUniformBuffer ?
                                   getDevice()->getLimits().minUniformBufferOffsetAlignment : 1;


    mSize = mAllignment == 1 ? mSize : entries * mAllignment;

    mBuffer = getDevice()->createBuffer(mSize, mUsage);
    const vk::MemoryRequirements bufferMemReqs = getDevice()->getMemoryRequirements(mBuffer);
	mBufferMemory = getDevice()->getMemoryManager()->Allocate(mSize, bufferMemReqs.alignment, static_cast<bool>(mUsage & vk::BufferUsageFlagBits::eTransferSrc ||
																												mUsage & vk::BufferUsageFlagBits::eUniformBuffer));
    getDevice()->getMemoryManager()->BindBuffer(mBuffer, mBufferMemory);

	if(mName != "")
	{
		getDevice()->setDebugName(mName, reinterpret_cast<uint64_t>(VkBuffer(mBuffer)), vk::DebugReportObjectTypeEXT::eBuffer);
	}
}


Buffer::~Buffer()
{
    const bool shouldDestroy = release();
    if(shouldDestroy && mBuffer != vk::Buffer{nullptr})
        getDevice()->destroyBuffer(*this);
}


Buffer& Buffer::operator=(Buffer&& other)
{
    mBuffer = other.mBuffer;
    other.mBuffer = nullptr;
    mBufferMemory = other.mBufferMemory;
    mCurrentOffset = other.mCurrentOffset;
    mSize = other.mSize;
    mStride = other.mStride;
    mAllignment = other.mAllignment;
    mName = other.mName;

    return *this;
}


Buffer::Buffer(Buffer&& other) :    GPUResource (other.getDevice()->getCurrentSubmissionIndex()),
                                    DeviceChild (other.getDevice())
{
    mBuffer = other.mBuffer;
    other.mBuffer = nullptr;
    mBufferMemory = other.mBufferMemory;
    mCurrentOffset = other.mCurrentOffset;
    mSize = other.mSize;
    mStride = other.mStride;
    mAllignment = other.mAllignment;
    mName = other.mName;
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

    BarrierRecorder barrier{getDevice()};
    barrier.makeContentsVisible(*this);

    getDevice()->execute(barrier);

    // Maybe try to implement a staging buffer cache so that we don't have to create one
    // each time.
    stagingBuffer.updateLastAccessed(getDevice()->getCurrentSubmissionIndex());
    updateLastAccessed(getDevice()->getCurrentSubmissionIndex());
}


void*   Buffer::map()
{
    return getDevice()->getMemoryManager()->MapAllocation(mBufferMemory);
}


void    Buffer::unmap()
{
    getDevice()->getMemoryManager()->UnMapAllocation(mBufferMemory);
}

