#ifndef RENDERGRAPH_HPP
#define RENDERGRAPH_HPP

#include "GraphicsTask.hpp"
#include "ComputeTask.hpp"

#include "Core/Image.hpp"
#include "Core/Buffer.hpp"

#include <string>
#include <map>
#include <optional>
#include <vector>
#include <tuple>


class DescriptorManager;


class RenderGraph
{
    friend RenderDevice;
    friend DescriptorManager;
public:

    RenderGraph() = default;

    void addTask(const GraphicsTask&);
    void addTask(const ComputeTask&);

    void addDependancy(const std::string&, const std::string&);

    void bindImage(const std::string&, Buffer&);
    void bindBuffer(const std::string&, Image&);

    // These are special because they are used by every task that has vertex attributes
    void bindVertexBuffer(const Buffer&);
    void bindIndexBuffer(const Buffer&);

private:
    // resources tracking
    enum class ResourceType
    {
        Image,
        Buffer
    };

    enum class TaskType
    {
        Graphics,
        Compute
    };

    struct vulkanResources
    {
        vk::Pipeline mPipeline;
        vk::PipelineLayout mPipelineLayout;
        vk::DescriptorSetLayout mDescSetLayout;
        // Only needed for graphics tasks
        std::optional<vk::RenderPass> mRenderPass;
        std::optional<vk::VertexInputBindingDescription> mVertexBindingDescription;
        std::optional<std::vector<vk::VertexInputAttributeDescription>> mVertexAttributeDescription;

        std::optional<vk::Framebuffer> mFrameBuffer;
        bool mFrameBufferNeedsUpdating;
        vk::DescriptorSet mDescSet;
        bool mDescSetNeedsUpdating;

    };

    void reorderTasks();
    void mergeTasks();

    RenderTask& getTask(TaskType, uint32_t);
    const RenderTask& getTask(TaskType, uint32_t) const;


    GPUResource& getResource(const ResourceType, const uint32_t);

    void bindResource(const std::string&, const uint32_t, const ResourceType);

    std::vector<GraphicsTask> mGraphicsTasks;
    std::vector<ComputeTask>  mComputeTask;

    Buffer mVertexBuffer;
    Buffer mIndexBuffer;

    std::vector<std::pair<TaskType, uint32_t>> mTaskOrder;
    std::vector<vulkanResources> mVulkanResources;

    std::vector<std::pair<uint32_t, uint32_t>> mTaskDependancies;

    struct ResourceBindingInfo
    {
        ResourceType mResourcetype;
        uint32_t mResourceIndex;
        uint32_t mResourceBinding;
    };

    std::vector<std::vector<ResourceBindingInfo>> mInputResources;
    std::vector<std::vector<ResourceBindingInfo>> mOutputResources;

    std::vector<std::pair<std::string, Image>> mImages;
    std::vector<std::pair<std::string, Buffer>> mBuffers;
};


#endif
