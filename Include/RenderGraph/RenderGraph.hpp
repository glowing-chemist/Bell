#ifndef RENDERGRAPH_HPP
#define RENDERGRAPH_HPP

#include "GraphicsTask.hpp"
#include "ComputeTask.hpp"

#include "Core/Image.hpp"
#include "Core/Buffer.hpp"

#include <iterator>
#include <string>
#include <map>
#include <optional>
#include <vector>
#include <tuple>


class DescriptorManager;
class TaskIterator;
class ResourceIterator;

enum class BindingIteratorType
{
	Input,
	Output
};

template<BindingIteratorType>
class BindingIterator;

class RenderGraph
{
	friend TaskIterator;
	friend ResourceIterator;
	friend BindingIterator<BindingIteratorType::Input>;
	friend BindingIterator<BindingIteratorType::Output>;
public:

    RenderGraph() = default;

    void addTask(const GraphicsTask&);
    void addTask(const ComputeTask&);

    void addDependancy(const std::string&, const std::string&);

    void bindImage(const std::string&, Image&);
    void bindBuffer(const std::string&, Buffer&);

    // These are special because they are used by every task that has vertex attributes
    void bindVertexBuffer(const Buffer&);
    void bindIndexBuffer(const Buffer&);

	RenderTask& getTask(TaskType, uint32_t);
	const RenderTask& getTask(TaskType, uint32_t) const;

	uint64_t taskCount() const { return mTaskOrder.size(); }

	TaskIterator taskBegin();
	TaskIterator taskEnd();

	ResourceIterator resourceBegin();
	ResourceIterator resourceEnd();

	BindingIterator<BindingIteratorType::Input> inputBindingBegin();
	BindingIterator<BindingIteratorType::Input> inputBindingEnd();

	BindingIterator< BindingIteratorType::Output> outputBindingBegin();
	BindingIterator< BindingIteratorType::Output> outputBindingEnd();

    // resources tracking
    enum class ResourceType
    {
        Image,
        Buffer
    };
	GPUResource& getResource(const ResourceType, const uint32_t);

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

	struct ResourceBindingInfo
	{
		ResourceType mResourcetype;
		uint32_t mResourceIndex;
		uint32_t mResourceBinding;
	};

private:

    void reorderTasks();
    void mergeTasks();

    void bindResource(const std::string&, const uint32_t, const ResourceType);

    std::vector<GraphicsTask> mGraphicsTasks;
    std::vector<ComputeTask>  mComputeTask;

    Buffer mVertexBuffer;
    Buffer mIndexBuffer;

    std::vector<std::pair<TaskType, uint32_t>> mTaskOrder;
    std::vector<vulkanResources> mVulkanResources;

    std::vector<std::pair<uint32_t, uint32_t>> mTaskDependancies;

    std::vector<std::vector<ResourceBindingInfo>> mInputResources;
    std::vector<std::vector<ResourceBindingInfo>> mOutputResources;

    std::vector<std::pair<std::string, Image>> mImages;
    std::vector<std::pair<std::string, Buffer>> mBuffers;
};




class TaskIterator : public std::iterator<std::forward_iterator_tag,
	RenderTask>
{
	uint64_t mCurrentIndex;
	const RenderGraph& mGraph;

public:

	TaskIterator(RenderGraph& graph, uint64_t startingIndex = 0) : mCurrentIndex{ startingIndex }, mGraph{ graph } {}

	const RenderTask& operator*() const;
	TaskIterator& operator++();
	bool operator==(const TaskIterator& rhs) { return mCurrentIndex == rhs.mCurrentIndex; }
	bool operator!=(const TaskIterator& rhs) { return !(*this == rhs); }
};


class ResourceIterator : public std::iterator<std::forward_iterator_tag,
	RenderGraph::vulkanResources>
{
	std::vector<RenderGraph::vulkanResources>& mResources;
	uint64_t mCurrentIndex;
	const RenderGraph& mGraph;

public:

	ResourceIterator(std::vector<RenderGraph::vulkanResources>& resources, RenderGraph& graph, uint64_t startingIndex = 0) : mResources{ resources }, mCurrentIndex{ startingIndex }, mGraph{ graph } {}

	RenderGraph::vulkanResources& operator*() { return mResources[mCurrentIndex]; }
	ResourceIterator& operator++() { ++mCurrentIndex; return *this; }
	bool operator==(const ResourceIterator& rhs) { mCurrentIndex == rhs.mCurrentIndex; }
	bool operator!=(const ResourceIterator& rhs) { return !(*this == rhs); }
};


template<BindingIteratorType>
class BindingIterator : public std::iterator<std::forward_iterator_tag,
	std::vector<RenderGraph::ResourceBindingInfo>>
{
	std::vector<std::vector<RenderGraph::ResourceBindingInfo>>& mBindings;
	uint64_t mCurrentIndex;
	const RenderGraph& mGraph;

public:

	BindingIterator(std::vector<std::vector<RenderGraph::ResourceBindingInfo>>& bindings, RenderGraph& graph, uint64_t startingIndex = 0) : mBindings{ bindings }, mCurrentIndex{ startingIndex }, mGraph{ graph } {}

	std::vector<RenderGraph::ResourceBindingInfo>& operator*() { return mBindings[mCurrentIndex];  }
	BindingIterator& operator++() { ++mCurrentIndex; return *this; }
	bool operator==(const BindingIterator& rhs) { return mCurrentIndex == rhs.mCurrentIndex; }
	bool operator!=(const BindingIterator& rhs) { return !(*this == rhs); }
};

#endif
