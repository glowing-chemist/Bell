#include "VulkanCommandContext.hpp"
#include "VulkanExecutor.hpp"
#include "VulkanRenderDevice.hpp"


VulkanCommandContext::VulkanCommandContext(RenderDevice* dev) :
    CommandContextBase(dev),
    mCommandPool(dev, QueueType::Graphics),
    mDescriptorManager(dev),
    mActiveRenderPass(nullptr)
{
}


void VulkanCommandContext::setupState(const RenderGraph& graph, uint32_t taskIndex, Executor* exec, const uint64_t prefixHash)
{
    VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    vulkanResources resources = device->getTaskResources(graph, taskIndex, prefixHash);

    VulkanExecutor* VKExec = static_cast<VulkanExecutor*>(exec);
    VKExec->setPipelineLayout(resources.mPipeline->getLayoutHandle());
    vk::CommandBuffer cmdBuffer = VKExec->getCommandBuffer();

    vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eCompute;
    const RenderTask& task = graph.getTask(taskIndex);

    if(resources.mRenderPass)
    {
        mActiveRenderPass = *resources.mRenderPass;

        const auto& graphicsTask = static_cast<const GraphicsTask&>(task);
        const auto& pipelineDesc = graphicsTask.getPipelineDescription();
        vk::Rect2D renderArea{ {0, 0}, {pipelineDesc.mViewport.x, pipelineDesc.mViewport.y} };

        const std::vector<ClearValues> clearValues = graphicsTask.getClearValues();
        std::vector<vk::ClearValue> vkClearValues{};
        for (const auto& value : clearValues)
        {
            vk::ClearValue val{};
            if (value.mType == AttachmentType::Depth)
                val.setDepthStencil({ value.r, static_cast<uint32_t>(value.r) });
            else
                val.setColor({ std::array<float, 4>{value.r, value.g, value.b, value.a} });

            vkClearValues.push_back(val);
        }

        vk::Framebuffer frameBuffer = device->createFrameBuffer(graph, taskIndex, *resources.mRenderPass);

        vk::RenderPassBeginInfo passBegin{};
        passBegin.setRenderPass(*resources.mRenderPass);
        passBegin.setFramebuffer(frameBuffer);
        passBegin.setRenderArea(renderArea);
        if (!clearValues.empty())
        {
            passBegin.setClearValueCount(static_cast<uint32_t>(vkClearValues.size()));
            passBegin.setPClearValues(vkClearValues.data());
        }

        cmdBuffer.beginRenderPass(passBegin, vk::SubpassContents::eInline);

        bindPoint = vk::PipelineBindPoint::eGraphics;

        // Destruction on the device is deferred.
        device->destroyFrameBuffer(frameBuffer, device->getCurrentSubmissionIndex());
    }

    //allocate and write descriptor sets.
    std::vector<vk::DescriptorSet> descriptorSets = mDescriptorManager.getDescriptors(graph, taskIndex, resources.mDescSetLayout[0]);
    mDescriptorManager.writeDescriptors(graph, taskIndex, descriptorSets[0]);

    cmdBuffer.bindPipeline(bindPoint, resources.mPipeline->getHandle());

    cmdBuffer.bindDescriptorSets(bindPoint, resources.mPipeline->getLayoutHandle(), 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
}


Executor* VulkanCommandContext::allocateExecutor()
{
    if(!mFreeExecutors.empty())
    {
        Executor* exec = mFreeExecutors.back();
        mFreeExecutors.pop_back();

        return exec;
    }
    else
    {
        const uint32_t neededIndex = mCommandPool.getNumberOfBuffersForQueue();
        vk::CommandBuffer cmdBuffer = mCommandPool.getBufferForQueue(neededIndex);

        return new VulkanExecutor(cmdBuffer);
    }
}


void VulkanCommandContext::freeExecutor(Executor* exec)
{
    if(mActiveRenderPass != vk::RenderPass{nullptr})
    {
        VulkanExecutor* VKexec = static_cast<VulkanExecutor*>(exec);
        vk::CommandBuffer cmdBuffer = VKexec->getCommandBuffer();
        cmdBuffer.endRenderPass();

        mActiveRenderPass = vk::RenderPass{nullptr};
    }

    mFreeExecutors.push_back(exec);
}


void VulkanCommandContext::reset()
{
    mCommandPool.reset();
    mDescriptorManager.reset();
}


vk::CommandBuffer VulkanCommandContext::getPrefixCommandBuffer()
{
    VulkanExecutor* exec = static_cast<VulkanExecutor*>(allocateExecutor());
    vk::CommandBuffer cmdBuffer = exec->getCommandBuffer();
    freeExecutor(exec);

    return cmdBuffer;

}
