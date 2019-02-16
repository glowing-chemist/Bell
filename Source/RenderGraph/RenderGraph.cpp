
#include "RenderGraph/RenderGraph.hpp"

#include <algorithm>

void RenderGraph::addTask(const GraphicsTask& task)
{
    const uint32_t taskIndex = mGraphicsTasks.size();
    mGraphicsTasks.push_back(task);

    mTaskOrder.push_back({TaskType::Graphics, taskIndex});
    // Also add a vulkan resources and inputs/outputs for each task, zero initialised
    mVulkanResources.emplace_back();
    mInputResources.emplace_back();
    mOutputResources.emplace_back();
}


void RenderGraph::addTask(const ComputeTask& task)
{
    const uint32_t taskIndex = mComputeTask.size();
    mComputeTask.push_back(task);

    mTaskOrder.push_back({TaskType::Compute, taskIndex});

    // Also add a vulkan resources and inputs/outputs for each task, zero initialised
    mVulkanResources.emplace_back();
    mInputResources.emplace_back();
    mOutputResources.emplace_back();
}


void RenderGraph::addDependancy(const std::string& dependancy, const std::string& dependant)
{
    // Find the task index in mGraphicsTasks/mComputeTasks
    uint32_t dependancyTaskIndex = 0;
    uint32_t dependantTaskIndex = 0;
    {
    auto dependancyGraphicsIt = std::find_if(mGraphicsTasks.begin(), mGraphicsTasks.end(),
                                            [&dependancy](auto& task) {return task.getName() == dependancy;});


    if(dependancyGraphicsIt == mGraphicsTasks.end())
    {
        auto dependancyComputeIt = std::find_if(mComputeTask.begin(), mComputeTask.end(),
                                                    [&dependancy](const auto& task) {return task.getName() == dependancy;});

        const uint32_t computeTaskIndex = std::distance(mComputeTask.begin(), dependancyComputeIt);
        uint32_t taskCounter = 0;
        for(uint32_t i = 0; i < mTaskOrder.size(); ++i)
        {
            if(mTaskOrder[i].first == TaskType::Compute)
                ++taskCounter;
            if(taskCounter == computeTaskIndex)
                dependancyTaskIndex = i;
        }
    }
    else
    {
        const uint32_t graphicsTaskIndex = std::distance(mGraphicsTasks.begin(), dependancyGraphicsIt);
        uint32_t taskCounter = 0;
        for(uint32_t i = 0; i < mTaskOrder.size(); ++i)
        {
            if(mTaskOrder[i].first == TaskType::Graphics)
                ++taskCounter;
            if(taskCounter == graphicsTaskIndex)
                dependancyTaskIndex = i;
        }
    }
    }
    // Find the dependant taskIndex now
    {
    auto dependancyGraphicsIt = std::find_if(mGraphicsTasks.begin(), mGraphicsTasks.end(),
                                            [&dependant](auto& task) {return task.getName() == dependant;});


    if(dependancyGraphicsIt == mGraphicsTasks.end())
    {
        auto dependancyComputeIt = std::find_if(mComputeTask.begin(), mComputeTask.end(),
                                                    [&dependant](const auto& task) {return task.getName() == dependant;});

        const uint32_t computeTaskIndex = std::distance(mComputeTask.begin(), dependancyComputeIt);
        uint32_t taskCounter = 0;
        for(uint32_t i = 0; i < mTaskOrder.size(); ++i)
        {
            if(mTaskOrder[i].first == TaskType::Compute)
                ++taskCounter;
            if(taskCounter == computeTaskIndex)
                dependancyTaskIndex = i;
        }
    }
    else
    {
        const uint32_t graphicsTaskIndex = std::distance(mGraphicsTasks.begin(), dependancyGraphicsIt);
        uint32_t taskCounter = 0;
        for(uint32_t i = 0; i < mTaskOrder.size(); ++i)
        {
            if(mTaskOrder[i].first == TaskType::Graphics)
                ++taskCounter;
            if(taskCounter == graphicsTaskIndex)
                dependancyTaskIndex = i;
        }
    }
    }

    mTaskDependancies.push_back({dependancyTaskIndex, dependantTaskIndex});
}


void RenderGraph::bindResource(const std::string& name, const uint32_t index, const ResourceType resourcetype)
{
    uint32_t taskOrderIndex = 0;
    for(const auto [taskType, taskIndex] : mTaskOrder)
    {
        RenderTask& task = getTask(taskType, taskIndex);

        uint32_t inputAttachmentIndex = 0;
        for(const auto& input : task.getInputAttachments())
        {
            if(input.first == name)
            {
                mInputResources[taskOrderIndex][inputAttachmentIndex] = {resourcetype, index, inputAttachmentIndex};
                mVulkanResources[taskOrderIndex].mDescSetNeedsUpdating = true;
                break; // Assume a resource is only bound once per task.
            }
            ++inputAttachmentIndex;
        }

        uint32_t outputAttachmentIndex = 0;
        for(const auto& input : task.getOuputAttachments())
        {
            if(input.first == name)
            {
                mOutputResources[taskOrderIndex][outputAttachmentIndex] = {resourcetype, index, outputAttachmentIndex};
                mVulkanResources[taskOrderIndex].mFrameBufferNeedsUpdating = true;
                break;
            }
            ++outputAttachmentIndex;
        }

        ++taskOrderIndex;
    }
}


void RenderGraph::bindImage(const std::string& name, Image& buffer)
{
    const uint32_t currentImageIndex = mImages.size();
    mImages.push_back({name, buffer});

    bindResource(name, currentImageIndex, ResourceType::Image);
}


void RenderGraph::bindBuffer(const std::string& name , Buffer& image)
{
    const uint32_t currentBufferIndex = mBuffers.size();
    mBuffers.push_back({name, image});

    bindResource(name, currentBufferIndex, ResourceType::Buffer);
}


void RenderGraph::bindVertexBuffer(const Buffer& buffer)
{
    mVertexBuffer = buffer;
}


void RenderGraph::bindIndexBuffer(const Buffer& buffer)
{
    mIndexBuffer = buffer;
}


void RenderGraph::reorderTasks()
{
    // TODO
}


void RenderGraph::mergeTasks()
{
    // TODO
}


RenderTask& RenderGraph::getTask(TaskType taskType, uint32_t taskIndex)
{
    RenderTask& task = [taskType, taskIndex, this]() -> RenderTask&
    {
        switch(taskType)
        {
            case TaskType::Graphics:
                return mGraphicsTasks[taskIndex];
            case TaskType::Compute:
                return mComputeTask[taskIndex];
        }
    }();

    return task;
}


const RenderTask& RenderGraph::getTask(TaskType taskType, uint32_t taskIndex) const
{
    const RenderTask& task = [taskType, taskIndex, this]() -> const RenderTask&
    {
        switch(taskType)
        {
            case TaskType::Graphics:
                return mGraphicsTasks[taskIndex];
            case TaskType::Compute:
                return mComputeTask[taskIndex];
        }
    }();

    return task;
}


GPUResource& RenderGraph::getResource(const ResourceType resourceType, const uint32_t resourceIndex)
{
    GPUResource& resource = [resourceType, resourceIndex, this]() -> GPUResource&
    {
        switch(resourceType)
        {
            case ResourceType::Image:
                return mImages[resourceIndex].second;

            case ResourceType::Buffer:
                return mBuffers[resourceIndex].second;
        }
    }();

    return resource;
}


TaskIterator RenderGraph::taskBegin()
{
	auto[taskType, taskIndex] = mTaskOrder[0];
	RenderTask& task = getTask(taskType, taskIndex);

	return TaskIterator{*this};
}


TaskIterator RenderGraph::taskEnd()
{
	auto[taskType, taskIndex] = mTaskOrder[0];
	RenderTask& task = getTask(taskType, taskIndex);

	return TaskIterator{*this, mTaskOrder.size()};
}


ResourceIterator RenderGraph::resourceBegin()
{
	return ResourceIterator{mVulkanResources, *this};
}


ResourceIterator RenderGraph::resourceEnd()
{
	return ResourceIterator{ mVulkanResources, *this, mVulkanResources.size()};
}


BindingIterator<BindingIteratorType::Input> RenderGraph::inputBindingBegin()
{
	return BindingIterator<BindingIteratorType::Input>{mInputResources, *this};
}


BindingIterator<BindingIteratorType::Input> RenderGraph::inputBindingEnd()
{
	return BindingIterator<BindingIteratorType::Input>{mInputResources, *this, mInputResources.size()};
}


BindingIterator< BindingIteratorType::Output> RenderGraph::outputBindingBegin()
{
	return BindingIterator<BindingIteratorType::Output>{mInputResources, *this};
}


BindingIterator< BindingIteratorType::Output> RenderGraph::outputBindingEnd()
{
	return BindingIterator<BindingIteratorType::Output>{mInputResources, *this, mOutputResources.size()};
}


TaskIterator& TaskIterator::operator++()
{
	++mCurrentIndex;

	return *this;
}


const RenderTask& TaskIterator::operator*() const
{
	auto[taskType, taskIndex] = mGraph.mTaskOrder[mCurrentIndex];
	return mGraph.getTask(taskType, taskIndex);
}
