#ifndef VULKAN_COMMAND_CONTEXT_HPP
#define VULKAN_COMMAND_CONTEXT_HPP

#include "CommandPool.h"
#include "DescriptorManager.hpp"
#include "Core/CommandContext.hpp"


class VulkanCommandContext : public CommandContextBase
{
public:

    VulkanCommandContext(RenderDevice* dev);
    ~VulkanCommandContext() = default;

    virtual void      setupState(const RenderGraph&, uint32_t taskIndex, Executor*, const uint64_t prefixHash) override final;

    virtual Executor* allocateExecutor() override final;
    virtual void      freeExecutor(Executor*) override final;

    virtual void      reset() override final;

    vk::CommandBuffer getPrefixCommandBuffer();

private:
    CommandPool mCommandPool;
    DescriptorManager mDescriptorManager;

    vk::RenderPass mActiveRenderPass;
};

#endif
