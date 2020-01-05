#include "VulkanExecutor.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanBufferView.hpp"
#include "VulkanPipeline.hpp"


VulkanExecutor::VulkanExecutor(vk::CommandBuffer cmdBuffer, const vulkanResources& resources) :
	mCommandBuffer{ cmdBuffer },
	mResources{ resources }
{
}


void VulkanExecutor::draw(const uint32_t vertexOffset, const uint32_t vertexCount)
{
	mCommandBuffer.draw(vertexCount, 1, 0, 0);
}


void VulkanExecutor::instancedDraw(const uint32_t vertexOffset, const uint32_t vertexCount, const uint32_t instanceCount)
{
	mCommandBuffer.draw(vertexCount, instanceCount, vertexOffset, 0);
}


void VulkanExecutor::indexedDraw(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfIndicies)
{
	mCommandBuffer.drawIndexed(numberOfIndicies, 1, indexOffset, vertexOffset, 0);
}


void VulkanExecutor::indexedInstancedDraw(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfInstances, const uint32_t numberOfIndicies)
{
	mCommandBuffer.drawIndexed(numberOfIndicies, numberOfInstances, indexOffset, vertexOffset, 0);
}


void VulkanExecutor::indirectDraw(const uint32_t drawCalls, const BufferView& view)
{
	const VulkanBufferView& VKBuffer = static_cast<const VulkanBufferView&>(*view.getBase());
	mCommandBuffer.drawIndirect(VKBuffer.getBuffer(), view->getOffset(), drawCalls, sizeof(vk::DrawIndirectCommand));
}


void VulkanExecutor::indexedIndirectDraw(const uint32_t drawCalls, const BufferView& view)
{
	const VulkanBufferView& VKBuffer = static_cast<const VulkanBufferView&>(*view.getBase());
	mCommandBuffer.drawIndexedIndirect(VKBuffer.getBuffer(), view->getOffset(), drawCalls, sizeof(vk::DrawIndirectCommand));
}


void VulkanExecutor::insertPushConsatnt(const glm::mat4& val)
{
	mCommandBuffer.pushConstants(mResources.mPipeline->getLayoutHandle(), vk::ShaderStageFlagBits::eAll, 0, sizeof(glm::mat4), (void*)&val);
}


void VulkanExecutor::dispatch(const uint32_t x, const uint32_t y, const uint32_t z)
{
	mCommandBuffer.dispatch(x, y, z);
}


void VulkanExecutor::dispatchIndirect(const BufferView& view)
{
	const VulkanBufferView& VKBuffer = static_cast<const VulkanBufferView&>(*view.getBase());
	mCommandBuffer.dispatchIndirect(VKBuffer.getBuffer(), view->getOffset());
}


void VulkanExecutor::bindVertexBuffer(const Buffer& buffer, const size_t offset)
{
	const VulkanBuffer& VKBuffer = static_cast<const VulkanBuffer&>(*buffer.getBase());
	mCommandBuffer.bindVertexBuffers(0, VKBuffer.getBuffer(), offset);
}


void VulkanExecutor::bindIndexBuffer(const Buffer& buffer, const size_t offset)
{
	const VulkanBuffer& VKBuffer = static_cast<const VulkanBuffer&>(*buffer.getBase());
	mCommandBuffer.bindIndexBuffer(VKBuffer.getBuffer(), offset, vk::IndexType::eUint32);
}
