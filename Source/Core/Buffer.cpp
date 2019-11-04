#include "Core/Buffer.hpp"
#include "RenderDevice.hpp"
#include "Core/BarrierManager.hpp"
#include "Core/BellLogging.hpp"


Buffer::Buffer(RenderDevice* dev,
	   vk::BufferUsageFlags usage,
	   const uint32_t size,
	   const uint32_t stride,
	   const std::string &name) :
    GPUResource{dev->getCurrentSubmissionIndex()},
    DeviceChild{dev},
    mUsage{usage},
    mSize{size},
    mStride{stride},
    mName{name}
{
    BELL_ASSERT(size > 0 && stride > 0, "size and stride must be greater than 0")

    mAllignment = mUsage & vk::BufferUsageFlagBits::eUniformBuffer ?
                                   getDevice()->getLimits().minUniformBufferOffsetAlignment : 1;

    mBuffer = getDevice()->createBuffer(mSize, mUsage);
    const vk::MemoryRequirements bufferMemReqs = getDevice()->getMemoryRequirements(mBuffer);
    mBufferMemory = getDevice()->getMemoryManager()->Allocate(bufferMemReqs.size, bufferMemReqs.alignment, isMappable());
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
	{
        getDevice()->destroyBuffer(*this);
	}
}


Buffer& Buffer::operator=(const Buffer& buf)
{
	if(mBuffer != vk::Buffer(nullptr))
	{
		const bool shouldDestroy = release();
		if(shouldDestroy)
			getDevice()->destroyBuffer(*this);
	}

	GPUResource::operator=(buf);
	DeviceChild::operator=(buf);

	mBuffer = buf.mBuffer;
	mBufferMemory = buf.mBufferMemory;
	mCurrentMap = buf.mCurrentMap;
	mUsage = buf.mUsage;
	mSize = buf.mSize;
	mStride = buf.mStride;
	mAllignment = buf.mAllignment;
	mName = buf.mName;

	return *this;
}


void Buffer::swap(Buffer& other)
{
	vk::Buffer Buffer = mBuffer;
	Allocation BufferMemory = mBufferMemory;
	MapInfo CurrentMap = mCurrentMap;
	vk::BufferUsageFlags Usage = mUsage;
	uint32_t Size = mSize;
	uint32_t Stride = mStride;
	uint32_t Allignment = mAllignment;
	std::string Name = mName;

	mBuffer = other.mBuffer;
	other.mBuffer = Buffer;

	mBufferMemory = other.mBufferMemory;
	other.mBufferMemory = BufferMemory;

	mCurrentMap = other.mCurrentMap;
	other.mCurrentMap = CurrentMap;

	mUsage = other.mUsage;
	other.mUsage = Usage;

	mSize = other.mSize;
	other.mSize = Size;

	mStride = other.mStride;
	other.mStride = Stride;

	mAllignment = other.mAllignment;
	other.mAllignment = Allignment;

	mName = other.mName;
	other.mName = Name;
}


Buffer& Buffer::operator=(Buffer&& rhs)
{
	swap(rhs);

	return *this;
}


Buffer::Buffer(Buffer&& rhs) :
	GPUResource(rhs),
	DeviceChild(rhs)
{
	mBuffer = vk::Buffer{ nullptr };

	swap(rhs);
}


void Buffer::setContents(const void* data, const uint32_t size, const uint32_t offset)
{
    const uint32_t entries = size / mStride;

    if(isMappable())
    {
        BELL_ASSERT(size <= mSize, "Attempting to map a larger range than the buffer supports.")

        MapInfo mapInfo{};
        mapInfo.mOffset = offset;
        mapInfo.mSize = size;

        void* mappedBuffer = map(mapInfo);

        std::memcpy(mappedBuffer, data, size);

        unmap();
    }
    else
    {
        Buffer stagingBuffer(getDevice(), vk::BufferUsageFlagBits::eTransferSrc, size, mStride, "Staging Buffer");

        MapInfo mapInfo{};
        mapInfo.mOffset = 0;
        mapInfo.mSize = stagingBuffer.getSize();
        void* mappedBuffer = stagingBuffer.map(mapInfo);

            std::memcpy(mappedBuffer, data, size);

        stagingBuffer.unmap();

        vk::BufferCopy copyInfo{};
        copyInfo.setSize(size);
        copyInfo.setSrcOffset(0);
        copyInfo.setDstOffset(offset);

        getDevice()->getCurrentCommandPool()->getBufferForQueue(QueueType::Graphics)
                .copyBuffer(stagingBuffer.getBuffer(), getBuffer(), copyInfo);

        // TODO: Implement a staging buffer cache so that we don't have to create one
        // each time.
        stagingBuffer.updateLastAccessed(getDevice()->getCurrentSubmissionIndex());
    }
    updateLastAccessed(getDevice()->getCurrentSubmissionIndex());
}


void* Buffer::map(MapInfo& mapInfo)
{
	mapInfo.mMemory = mBufferMemory;
	mCurrentMap = mapInfo;

	updateLastAccessed(getDevice()->getCurrentSubmissionIndex());

	return getDevice()->getMemoryManager()->MapAllocation(mapInfo);
}


void  Buffer::unmap()
{
	updateLastAccessed(getDevice()->getCurrentSubmissionIndex());

	getDevice()->getMemoryManager()->UnMapAllocation(mCurrentMap);
}


bool Buffer::resize(const uint32_t newSize, const bool preserContents)
{
    if(newSize <= mSize)
        return false;

	// if the stride is the same as the size (it's one contiguos data block)
	// we need to update the stride.
	if (mSize == mStride)
		mStride = newSize;

    if(preserContents)
        resizePreserveContents(newSize);
    else
        resizeDiscardContents(newSize);

    updateLastAccessed(getDevice()->getCurrentSubmissionIndex());

    return true;
}


void Buffer::resizePreserveContents(const uint32_t newSize)
{
    vk::Buffer newBuffer = getDevice()->createBuffer(newSize, mUsage);
    const vk::MemoryRequirements bufferMemReqs = getDevice()->getMemoryRequirements(newBuffer);
    Allocation newMemory = getDevice()->getMemoryManager()->Allocate(bufferMemReqs.size, bufferMemReqs.alignment, isMappable());
    getDevice()->getMemoryManager()->BindBuffer(newBuffer, newMemory);

    vk::BufferCopy copyInfo{};
    copyInfo.setSize(mSize);
    copyInfo.setSrcOffset(0);
    copyInfo.setDstOffset(0);

    // Copy the contents
    getDevice()->getCurrentCommandPool()->getBufferForQueue(QueueType::Graphics)
            .copyBuffer(newBuffer, getBuffer(), copyInfo);

    // add the current buffer to deferred destruction queue.
    getDevice()->destroyBuffer(*this);

    mBuffer = newBuffer;
    mBufferMemory = newMemory;
    mSize = newSize;

    BarrierRecorder barrier{getDevice()};
    barrier.makeContentsVisible(*this);

	getDevice()->execute(barrier, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexInput);
}


void Buffer::resizeDiscardContents(const uint32_t newSize)
{
    vk::Buffer newBuffer = getDevice()->createBuffer(newSize, mUsage);
    const vk::MemoryRequirements bufferMemReqs = getDevice()->getMemoryRequirements(newBuffer);
    Allocation newMemory = getDevice()->getMemoryManager()->Allocate(bufferMemReqs.size, bufferMemReqs.alignment, isMappable());
    getDevice()->getMemoryManager()->BindBuffer(newBuffer, newMemory);

    // add the current buffer to deferred destruction queue.
    getDevice()->destroyBuffer(*this);

    mBuffer = newBuffer;
    mBufferMemory = newMemory;
    mSize = newSize;
}
