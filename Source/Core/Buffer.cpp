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
	mCurrentOffset = buf.mCurrentOffset;
	mUsage = buf.mUsage;
	mSize = buf.mSize;
	mStride = buf.mStride;
	mAllignment = buf.mAllignment;
	mName = buf.mName;

	return *this;
}


Buffer::Buffer(const Buffer& buf) :
	GPUResource{buf},
	DeviceChild{buf}
{
	mBuffer = buf.mBuffer;
	mBufferMemory = buf.mBufferMemory;
	mCurrentMap = buf.mCurrentMap;
	mCurrentOffset = buf.mCurrentOffset;
	mUsage = buf.mUsage;
	mSize = buf.mSize;
	mStride = buf.mStride;
	mAllignment = buf.mAllignment;
	mName = buf.mName;
}

void Buffer::swap(Buffer& other)
{
	vk::Buffer Buffer = mBuffer;
	Allocation BufferMemory = mBufferMemory;
	MapInfo CurrentMap = mCurrentMap;
	uint64_t CurrentOffset = mCurrentOffset;
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

	mCurrentOffset = other.mCurrentOffset;
	other.mCurrentOffset = CurrentOffset;

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


void Buffer::setContents(const void* data, const uint32_t size, const uint32_t offset)
{
    const uint32_t entries = size / mStride;

    Buffer stagingBuffer = Buffer(getDevice(), vk::BufferUsageFlagBits::eTransferSrc, size * mAllignment, mStride, "Staging Buffer");

	MapInfo mapInfo{};
	mapInfo.mOffset = 0;
	mapInfo.mSize = stagingBuffer.getSize();
	void* mappedBuffer = stagingBuffer.map(mapInfo);

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

