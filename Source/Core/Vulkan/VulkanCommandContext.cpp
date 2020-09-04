#include "VulkanCommandContext.hpp"
#include "VulkanExecutor.hpp"
#include "VulkanRenderDevice.hpp"


VulkanCommandContext::VulkanCommandContext(RenderDevice* dev, const QueueType queue) :
    CommandContextBase(dev, queue),
    mCommandPool(dev, queue),
    mDescriptorManager(dev),
    mTimeStampPool(nullptr),
    mTimeStamps{},
    mActiveRenderPass(nullptr)
{
    VulkanRenderDevice* vkDev = static_cast<VulkanRenderDevice*>(getDevice());
    mTimeStampPool = vkDev->createTimeStampPool(50);
}


VulkanCommandContext::~VulkanCommandContext()
{
    static_cast<VulkanRenderDevice*>(getDevice())->destroyTimeStampPool(mTimeStampPool);
}


void VulkanCommandContext::setupState(const RenderGraph& graph, uint32_t taskIndex, Executor* exec, const uint64_t prefixHash)
{
    VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    vulkanResources resources = device->getTaskResources(graph, taskIndex, prefixHash);

    VulkanExecutor* VKExec = static_cast<VulkanExecutor*>(exec);
    VKExec->setPipelineLayout(resources.mPipelineTemplate->getLayoutHandle());
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
    }

    //allocate and write descriptor sets.
    std::vector<vk::DescriptorSet> descriptorSets = mDescriptorManager.getDescriptors(graph, taskIndex, resources.mDescSetLayout[0]);
    mDescriptorManager.writeDescriptors(graph, taskIndex, descriptorSets[0]);

    cmdBuffer.bindDescriptorSets(bindPoint, resources.mPipelineTemplate->getLayoutHandle(), 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
}


Executor* VulkanCommandContext::allocateExecutor(const bool timeStamp)
{
    mActiveQuery = timeStamp;
    Executor* exec = nullptr;

    if(!mFreeExecutors.empty())
    {
        exec = mFreeExecutors.back();
        mFreeExecutors.pop_back();
    }
    else
    {
        vk::CommandBuffer cmdBuffer = mCommandPool.getBufferForQueue();

        exec = new VulkanExecutor(getDevice(), cmdBuffer);
    }

    if(mActiveQuery)
    {
        // Write start timestamp.
        vk::CommandBuffer cmdBuffer = static_cast<VulkanExecutor*>(exec)->getCommandBuffer();
        cmdBuffer.resetQueryPool(mTimeStampPool, mTimeStamps.size(), 2);
        cmdBuffer.writeTimestamp(vk::PipelineStageFlagBits::eAllCommands, mTimeStampPool, mTimeStamps.size());
        mTimeStamps.emplace_back();
    }

    return exec;
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

    if(mActiveQuery)
    {
        // write finish timestamp.
        vk::CommandBuffer cmdBuffer = static_cast<VulkanExecutor*>(exec)->getCommandBuffer();
        cmdBuffer.writeTimestamp(vk::PipelineStageFlagBits::eAllCommands, mTimeStampPool, mTimeStamps.size());
        mTimeStamps.emplace_back();
    }

    mFreeExecutors.push_back(exec);
}


const std::vector<uint64_t>& VulkanCommandContext::getTimestamps()
{
    static_cast<VulkanRenderDevice*>(getDevice())->writeTimeStampResults(mTimeStampPool, mTimeStamps.data(), mTimeStamps.size());
    return mTimeStamps;
}


void VulkanCommandContext::reset()
{
    mCommandPool.reset();
    mDescriptorManager.reset();

    mTimeStamps.clear();
}


vk::CommandBuffer VulkanCommandContext::getPrefixCommandBuffer()
{
    return mCommandPool.getBufferForQueue();
}
