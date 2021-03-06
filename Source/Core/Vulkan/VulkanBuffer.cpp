#include "VulkanBuffer.hpp"
#include "VulkanRenderDevice.hpp"
#include "VulkanBarrierManager.hpp"
#include "Core/BarrierManager.hpp"
#include "Core/BellLogging.hpp"
#include "Core/ConversionUtils.hpp"


VulkanBuffer::VulkanBuffer(RenderDevice* dev,
	BufferUsage usage,
	const uint32_t size,
	const uint32_t stride,
	const std::string& name) :
	BufferBase(dev, usage, size, stride, name)
{
	VulkanRenderDevice * device = static_cast<VulkanRenderDevice*>(dev);

    mBuffer = device->createBuffer(mSize, getVulkanBufferUsage(mUsage));
    const vk::MemoryRequirements bufferMemReqs = device->getMemoryRequirements(mBuffer);
    mBufferMemory = device->getMemoryManager()->Allocate(bufferMemReqs.size, bufferMemReqs.alignment, isMappable(), mName);
    device->getMemoryManager()->BindBuffer(mBuffer, mBufferMemory);

    mAllignment = bufferMemReqs.alignment;

	if(mName != "")
	{
        getDevice()->setDebugName(mName, reinterpret_cast<uint64_t>(VkBuffer(mBuffer)), VK_OBJECT_TYPE_BUFFER);
	}
}


VulkanBuffer::~VulkanBuffer()
{
	getDevice()->destroyBuffer(*this);
}


void VulkanBuffer::swap(BufferBase& other)
{
	VulkanBuffer& VkOther = static_cast<VulkanBuffer&>(other);

	vk::Buffer Buffer = mBuffer;
	Allocation BufferMemory = mBufferMemory;
	MapInfo CurrentMap = mCurrentMap;

	BufferBase::swap(other);

	mBuffer = VkOther.mBuffer;
	VkOther.mBuffer = Buffer;

	mBufferMemory = VkOther.mBufferMemory;
	VkOther.mBufferMemory = BufferMemory;

	mCurrentMap = VkOther.mCurrentMap;
	VkOther.mCurrentMap = CurrentMap;
}


void VulkanBuffer::setContents(const void* data, const uint32_t size, const uint32_t offset)
{
    setContents(static_cast<VulkanRenderDevice*>(getDevice())->getPrefixCommandBuffer(), data, size, offset);
}

void VulkanBuffer::setContents(vk::CommandBuffer cmd,
                                const void* data,
                                const uint32_t size,
                                const uint32_t offset)
{
    BELL_ASSERT(offset + size <= mSize, "Attempting to upload more data than buffer can hold")

        if (isMappable())
        {
            BELL_ASSERT(size <= mSize, "Attempting to map a larger range than the buffer supports.")

                MapInfo mapInfo {};
            mapInfo.mOffset = offset;
            mapInfo.mSize = size;

            void* mappedBuffer = map(mapInfo);

            std::memcpy(mappedBuffer, data, size);

            unmap();
        }
        else
        {
            BELL_ASSERT((mUsage & BufferUsage::TransferDest) != 0, "Buffer needs usage transfer dest")

                // Copy the data in to the command buffer and then perform the copy from there.
                if (size <= 1 << 16)
                {
                    cmd.updateBuffer(mBuffer, offset, size, data);
                }
                else
                {
                    VulkanBuffer stagingBuffer(getDevice(), BufferUsage::TransferSrc, size, mStride, "Staging Buffer");

                    stagingBuffer.setContents(data, size);

                    vk::BufferCopy copyInfo{};
                    copyInfo.setSize(size);
                    copyInfo.setSrcOffset(0);
                    copyInfo.setDstOffset(offset);

                    cmd.copyBuffer(stagingBuffer.getBuffer(), getBuffer(), copyInfo);
                    stagingBuffer.updateLastAccessed();
                }
       }
    updateLastAccessed();
}

void VulkanBuffer::setContents(const int data,
                 const uint32_t size,
                 const uint32_t offset)
{
    if(isMappable())
    {
        BELL_ASSERT(size <= mSize, "Attempting to map a larger range than the buffer supports.")

        MapInfo mapInfo{};
        mapInfo.mOffset = offset;
        mapInfo.mSize = size;

        void* mappedBuffer = map(mapInfo);

        std::memset(mappedBuffer, data, size);

        unmap();
    }
    else
    {
        BELL_ASSERT(size <= 1 << 16, "Need to implement s staging buffer method for this if we trap here")
        BELL_ASSERT((mUsage & BufferUsage::TransferDest) != 0, "Buffer needs usage transfer dest")
        BELL_ASSERT((size & 3) == 0, "Size must be aligned to 4 bytes")

		VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

        device->getPrefixCommandBuffer()
                .fillBuffer(mBuffer, offset, size, static_cast<uint32_t>(data));
    }

    updateLastAccessed();
}


void* VulkanBuffer::map(MapInfo& mapInfo)
{
    BELL_ASSERT(isMappable(), "Buffer is not mappable")

	mCurrentMap = mapInfo;

	updateLastAccessed();

	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());
	return device->getMemoryManager()->MapAllocation(mapInfo, mBufferMemory);
}


void  VulkanBuffer::unmap()
{
    BELL_ASSERT(isMappable(), "Buffer is not mappable")

	updateLastAccessed();

	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());
	device->getMemoryManager()->UnMapAllocation(mCurrentMap, mBufferMemory);
}


bool VulkanBuffer::resize(const uint32_t newSize, const bool preserContents)
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

    updateLastAccessed();

    return true;
}


uint64_t VulkanBuffer::getDeviceAddress() const
{
    return static_cast<const VulkanRenderDevice*>(getDevice())->getBufferDeviceAddress(this);
}


void VulkanBuffer::resizePreserveContents(const uint32_t newSize)
{
	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    vk::Buffer newBuffer = device->createBuffer(newSize, getVulkanBufferUsage(mUsage));
    const vk::MemoryRequirements bufferMemReqs = device->getMemoryRequirements(newBuffer);
    Allocation newMemory = device->getMemoryManager()->Allocate(bufferMemReqs.size, bufferMemReqs.alignment, isMappable());
    device->getMemoryManager()->BindBuffer(newBuffer, newMemory);

    vk::BufferCopy copyInfo{};
    copyInfo.setSize(mSize);
    copyInfo.setSrcOffset(0);
    copyInfo.setDstOffset(0);

    // Copy the contents
    device->getPrefixCommandBuffer()
            .copyBuffer(newBuffer, getBuffer(), copyInfo);

    // add the current buffer to deferred destruction queue.
    device->destroyBuffer(*this);

    mBuffer = newBuffer;
    mBufferMemory = newMemory;
    mSize = newSize;


	vk::BufferMemoryBarrier barrier{};
	barrier.setSize(mSize);
	barrier.setOffset(0);
	barrier.setBuffer(mBuffer);
	barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	barrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);

    device->getPrefixCommandBuffer()
		.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexInput,
			vk::DependencyFlagBits::eByRegion, {}, { barrier }, {});
    
}


void VulkanBuffer::resizeDiscardContents(const uint32_t newSize)
{
	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    vk::Buffer newBuffer = device->createBuffer(newSize, getVulkanBufferUsage(mUsage));
    const vk::MemoryRequirements bufferMemReqs = device->getMemoryRequirements(newBuffer);
    Allocation newMemory = device->getMemoryManager()->Allocate(bufferMemReqs.size, bufferMemReqs.alignment, isMappable());
    device->getMemoryManager()->BindBuffer(newBuffer, newMemory);

    // add the current buffer to deferred destruction queue.
    getDevice()->destroyBuffer(*this);

    mBuffer = newBuffer;
    mBufferMemory = newMemory;
    mSize = newSize;
}

