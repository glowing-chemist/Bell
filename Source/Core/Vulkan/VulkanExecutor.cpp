#include "VulkanExecutor.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanBufferView.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanRenderDevice.hpp"
#include "VulkanBarrierManager.hpp"
#include "Core/ConversionUtils.hpp"


VulkanExecutor::VulkanExecutor(vk::CommandBuffer cmdBuffer) :
    mCommandBuffer{ cmdBuffer },
    mRecordedCommands{0}
{
}


void VulkanExecutor::draw(const uint32_t vertexOffset, const uint32_t vertexCount)
{
    mCommandBuffer.draw(vertexCount, 1, vertexOffset, 0);
    ++mRecordedCommands;
}


void VulkanExecutor::instancedDraw(const uint32_t vertexOffset, const uint32_t vertexCount, const uint32_t instanceCount)
{
	mCommandBuffer.draw(vertexCount, instanceCount, vertexOffset, 0);
    ++mRecordedCommands;
}


void VulkanExecutor::indexedDraw(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfIndicies)
{
	mCommandBuffer.drawIndexed(numberOfIndicies, 1, indexOffset, vertexOffset, 0);
    ++mRecordedCommands;
}


void VulkanExecutor::indexedInstancedDraw(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfInstances, const uint32_t numberOfIndicies)
{
	mCommandBuffer.drawIndexed(numberOfIndicies, numberOfInstances, indexOffset, vertexOffset, 0);
    ++mRecordedCommands;
}


void VulkanExecutor::indirectDraw(const uint32_t drawCalls, const BufferView& view)
{
	const VulkanBufferView& VKBuffer = static_cast<const VulkanBufferView&>(*view.getBase());
	mCommandBuffer.drawIndirect(VKBuffer.getBuffer(), view->getOffset(), drawCalls, sizeof(vk::DrawIndirectCommand));
    ++mRecordedCommands;
}


void VulkanExecutor::indexedIndirectDraw(const uint32_t drawCalls, const BufferView& view)
{
	const VulkanBufferView& VKBuffer = static_cast<const VulkanBufferView&>(*view.getBase());
	mCommandBuffer.drawIndexedIndirect(VKBuffer.getBuffer(), view->getOffset(), drawCalls, sizeof(vk::DrawIndirectCommand));
    ++mRecordedCommands;
}


void VulkanExecutor::insertPushConsatnt(const void *val, const size_t size)
{
    mCommandBuffer.pushConstants(mPipelineLayout, vk::ShaderStageFlagBits::eAll, 0, size, val);
    ++mRecordedCommands;
}


void VulkanExecutor::dispatch(const uint32_t x, const uint32_t y, const uint32_t z)
{
	mCommandBuffer.dispatch(x, y, z);
    ++mRecordedCommands;
}


void VulkanExecutor::dispatchIndirect(const BufferView& view)
{
	const VulkanBufferView& VKBuffer = static_cast<const VulkanBufferView&>(*view.getBase());
	mCommandBuffer.dispatchIndirect(VKBuffer.getBuffer(), view->getOffset());
    ++mRecordedCommands;
}


void VulkanExecutor::bindVertexBuffer(const BufferView& buffer, const size_t offset)
{
    const VulkanBufferView& VKBuffer = static_cast<const VulkanBufferView&>(*buffer.getBase());
	mCommandBuffer.bindVertexBuffers(0, VKBuffer.getBuffer(), offset);
    ++mRecordedCommands;
}


void VulkanExecutor::bindIndexBuffer(const BufferView& buffer, const size_t offset)
{
    const VulkanBufferView& VKBuffer = static_cast<const VulkanBufferView&>(*buffer.getBase());
	mCommandBuffer.bindIndexBuffer(VKBuffer.getBuffer(), offset, vk::IndexType::eUint32);
    ++mRecordedCommands;
}


void VulkanExecutor::recordBarriers(BarrierRecorder& recorder)
{
    VulkanBarrierRecorder& VKRecorder = static_cast<VulkanBarrierRecorder&>(*recorder.getBase());

    if (!VKRecorder.empty())
    {
        const auto imageBarriers = VKRecorder.getImageBarriers(QueueType::Graphics);
        const auto bufferBarriers = VKRecorder.getBufferBarriers(QueueType::Graphics);
        const auto memoryBarriers = VKRecorder.getMemoryBarriers(QueueType::Graphics);

        if(bufferBarriers.empty() && imageBarriers.empty() && memoryBarriers.empty())
          return;

        mCommandBuffer.pipelineBarrier(getVulkanPipelineStage(VKRecorder.getSource()), getVulkanPipelineStage(VKRecorder.getDestination()),
        vk::DependencyFlagBits::eByRegion,
        static_cast<uint32_t>(memoryBarriers.size()), memoryBarriers.data(),
        static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
        static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data());
    }
    ++mRecordedCommands;
}


void VulkanExecutor::startCommandPredication(const BufferView& buffer, const uint32_t index)
{
    vk::ConditionalRenderingBeginInfoEXT beginInfo{};
    beginInfo.setBuffer(static_cast<const VulkanBufferView&>(*buffer.getBase()).getBuffer());
    beginInfo.setOffset(index * sizeof(uint32_t));

    mCommandBuffer.beginConditionalRenderingEXT(&beginInfo);
}


void VulkanExecutor::endCommandPredication()
{
    mCommandBuffer.endConditionalRenderingEXT();
}
