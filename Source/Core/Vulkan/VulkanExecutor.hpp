#ifndef VULKAN_EXECUTOR_HPP
#define VULKAN_EXECUTOR_HPP

#include "Core/Executor.hpp"
#include "Core/DeviceChild.hpp"

#include <vulkan/vulkan.hpp>


struct vulkanResources;


class VulkanExecutor : public Executor
{
public:
	VulkanExecutor(vk::CommandBuffer cmdBuffer, const vulkanResources& resources);

	virtual void draw(const uint32_t vertexOffset, const uint32_t vertexCount) override;

	virtual void instancedDraw(const uint32_t vertexOffset, const uint32_t vertexCount, const uint32_t instanceCount) override;

	virtual void indexedDraw(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfIndicies) override;

	virtual void indexedInstancedDraw(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfInstances, const uint32_t numberOfIndicies) override;

    virtual void indirectDraw(const uint32_t drawCalls, const BufferView&) override;

	virtual void indexedIndirectDraw(const uint32_t drawCalls, const BufferView&) override;

    virtual void insertPushConsatnt(const void* val, const size_t size) override;

	virtual void dispatch(const uint32_t x, const uint32_t y, const uint32_t z) override;

	virtual void dispatchIndirect(const BufferView&) override;

    virtual void bindVertexBuffer(const BufferView&, const size_t offset) override;

    virtual void bindIndexBuffer(const BufferView&, const size_t offset) override;

private:

	vk::CommandBuffer mCommandBuffer;
	const vulkanResources& mResources;

};

#endif
