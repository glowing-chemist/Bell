#ifndef VULKAN_EXECUTOR_HPP
#define VULKAN_EXECUTOR_HPP

#include "Core/Executor.hpp"
#include "Core/DeviceChild.hpp"

#include <vulkan/vulkan.hpp>


struct vulkanResources;


class VulkanExecutor : public Executor
{
public:
    VulkanExecutor(RenderDevice* dev, vk::CommandBuffer cmdBuffer);

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

    virtual void bindVertexBuffer(const BufferView*, const size_t* offsets, const uint32_t) override;

    virtual void bindIndexBuffer(const BufferView&, const size_t offset) override;

    virtual void recordBarriers(BarrierRecorder&) override;

    virtual void startCommandPredication(const BufferView &buf, const uint32_t index) override;

    virtual void endCommandPredication() override;

    virtual void copyDataToBuffer(const void*, const size_t size, const size_t offset, Buffer&) override;

    virtual void blitImage(const ImageView& dst, const ImageView& src, const SamplerType) override;

    virtual void setGraphicsShaders(const GraphicsTask &task,
                                    const RenderGraph& graph,
                                    const Shader& vertexShader,
                                    const Shader* geometryShader,
                                    const Shader* tessControl,
                                    const Shader* tessEval,
                                    const Shader& fragmentShader) override;

    virtual void setComputeShader(const ComputeTask& task,
                                  const RenderGraph& graph,
                                  const Shader&) override;

    virtual uint32_t getRecordedCommandCount() override
    {
        return mRecordedCommands;
    }

    virtual void      resetRecordedCommandCount() override
    {
        mRecordedCommands = 0;
    }

    uint64_t getAndClearSemaphoreReadSignalValue()
    {
        const uint64_t value = mMaxSemaphoreReadSignal;
        mMaxSemaphoreReadSignal = ~0ULL;
        return value;
    }

    uint64_t getAndClearSemaphoreWriteSignalValue()
    {
        const uint64_t value = mMaxSemaphoreWriteSignal;
        mMaxSemaphoreWriteSignal = ~0ULL;
        return value;
    }

    void setPipelineLayout(const vk::PipelineLayout layout)
    { mPipelineLayout = layout; }

    vk::CommandBuffer getCommandBuffer()
    {
        return mCommandBuffer;
    }

private:

	vk::CommandBuffer mCommandBuffer;
    vk::PipelineLayout mPipelineLayout;

    PFN_vkCmdBeginConditionalRenderingEXT mBeginConditionalRenderingFPtr;
    PFN_vkCmdEndConditionalRenderingEXT   mEndConditionalRenderingFPtr;

    uint32_t mRecordedCommands;
    uint64_t mMaxSemaphoreReadSignal;
    uint64_t mMaxSemaphoreWriteSignal;
};

#endif
