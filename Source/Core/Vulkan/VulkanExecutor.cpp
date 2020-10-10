#include "VulkanExecutor.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanBufferView.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanRenderDevice.hpp"
#include "VulkanBarrierManager.hpp"
#include "Core/ConversionUtils.hpp"


VulkanExecutor::VulkanExecutor(RenderDevice *dev, vk::CommandBuffer cmdBuffer) :
    Executor(dev),
    mCommandBuffer{ cmdBuffer },
    mRecordedCommands{0},
    mMaxSemaphoreReadSignal{~0ULL},
    mMaxSemaphoreWriteSignal{~0ULL}
{
    vk::Instance inst = static_cast<VulkanRenderDevice*>(getDevice())->getParentInstance();
    mBeginConditionalRenderingFPtr = reinterpret_cast<PFN_vkCmdBeginConditionalRenderingEXT>(inst.getProcAddr("vkCmdBeginConditionalRenderingEXT"));
    mEndConditionalRenderingFPtr = reinterpret_cast<PFN_vkCmdEndConditionalRenderingEXT>(inst.getProcAddr("vkCmdEndConditionalRenderingEXT"));
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
        const auto& semaphoreSignals = VKRecorder.getSemaphoreSignalInfo();

        if(bufferBarriers.empty() && imageBarriers.empty() && memoryBarriers.empty() && semaphoreSignals.empty())
          return;

        uint64_t maxSemaphoreReadValue = 0;
        uint64_t maxSemaphoreWriteValue = 0;
        for(auto& semaphoreSignal : semaphoreSignals)
        {
            if(semaphoreSignal.mWriteSemaphore)
                maxSemaphoreWriteValue = std::max(maxSemaphoreWriteValue, semaphoreSignal.mValue);
            else
                maxSemaphoreReadValue = std::max(maxSemaphoreReadValue, semaphoreSignal.mValue);
        }
        if(!semaphoreSignals.empty())
        {
            mMaxSemaphoreReadSignal = maxSemaphoreReadValue;
            mMaxSemaphoreWriteSignal = maxSemaphoreWriteValue;
        }

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
    VkConditionalRenderingBeginInfoEXT beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
    beginInfo.buffer = static_cast<const VulkanBufferView&>(*buffer.getBase()).getBuffer();
    beginInfo.offset = index * sizeof(uint32_t);

    BELL_ASSERT(mBeginConditionalRenderingFPtr, "Unable to find function ptr")
    mBeginConditionalRenderingFPtr(mCommandBuffer, &beginInfo);
}


void VulkanExecutor::endCommandPredication()
{
    BELL_ASSERT(mEndConditionalRenderingFPtr, "Unable to find function ptr")
    mEndConditionalRenderingFPtr(mCommandBuffer);
}


void VulkanExecutor::copyDataToBuffer(const void* data, const size_t size, const size_t offset, Buffer& buf)
{
    VulkanBuffer* VKBuf = static_cast<VulkanBuffer*>(buf.getBase());
    VKBuf->setContents(mCommandBuffer, data, size, offset);
}


void VulkanExecutor::setGraphicsShaders(const GraphicsTask& task,
                                        const RenderGraph& graph,
                                        const Shader& vertexShader,
                                        const Shader* geometryShader,
                                        const Shader* tessControl,
                                        const Shader* tessEval,
                                        const Shader& fragmentShader)
{
    BELL_ASSERT(vertexShader->getPrefixHash() == fragmentShader->getPrefixHash(), "Shaders compiled with different prefix hashes")

    VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());
    vulkanResources handles = device->getTaskResources(graph, task, vertexShader->getPrefixHash());

    std::shared_ptr<Pipeline> pipeline = handles.mPipelineTemplate->instanciateGraphicsPipeline(task, vertexShader->getPrefixHash() ^ vertexShader->getCompiledDefinesHash() ^ fragmentShader->getCompiledDefinesHash(), *handles.mRenderPass, vertexShader, geometryShader, tessControl, tessEval, fragmentShader);

    mCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->getHandle());
}


void VulkanExecutor::setComputeShader(const ComputeTask& task,
                                      const RenderGraph& graph,
                                      const Shader& computeShader)
{
    VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());
    vulkanResources handles = device->getTaskResources(graph, task, computeShader->getPrefixHash());

    std::shared_ptr<Pipeline> pipeline = handles.mPipelineTemplate->instanciateComputePipeline(task, computeShader->getPrefixHash() ^ computeShader->getCompiledDefinesHash(), computeShader);

    mCommandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline->getHandle());
}
