
#include "RenderGraph/RenderGraph.hpp"

#include <algorithm>

void RenderGraph::addTask(const GraphicsTask& task)
{
    const uint32_t taskIndex = mGraphicsTasks.size();
    mGraphicsTasks.push_back(task);

    mTaskOrder.push_back({TaskType::Graphics, taskIndex});
    // Also add a vulkan resources and inputs/outputs for each task, zero initialised
    mInputResources.emplace_back();
    mOutputResources.emplace_back();

    // This makes sure that the frameBuffer attahcment is registered properly.
    auto& outputResources = mOutputResources.back();
    for(const auto& resource : task.getOuputAttachments())
    {
        ResourceBindingInfo info{};
        info.mName = resource.first;

        outputResources.push_back(info);
    }
}


void RenderGraph::addTask(const ComputeTask& task)
{
    const uint32_t taskIndex = mComputeTask.size();
    mComputeTask.push_back(task);

    mTaskOrder.push_back({TaskType::Compute, taskIndex});

    // Also add a vulkan resources and inputs/outputs for each task, zero initialised
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
            if(taskCounter == computeTaskIndex)
                dependancyTaskIndex = i;
            if(mTaskOrder[i].first == TaskType::Compute)
                ++taskCounter;
        }
    }
    else
    {
        const uint32_t graphicsTaskIndex = std::distance(mGraphicsTasks.begin(), dependancyGraphicsIt);
        uint32_t taskCounter = 0;
        for(uint32_t i = 0; i < mTaskOrder.size(); ++i)
        {
            if(taskCounter == graphicsTaskIndex)
                dependancyTaskIndex = i;
            if(mTaskOrder[i].first == TaskType::Graphics)
                ++taskCounter;
        }
    }
    }
    // Find the dependant taskIndex now
    {
    auto dependantGraphicsIt = std::find_if(mGraphicsTasks.begin(), mGraphicsTasks.end(),
                                            [&dependant](auto& task) {return task.getName() == dependant;});


    if(dependantGraphicsIt == mGraphicsTasks.end())
    {
        auto dependantComputeIt = std::find_if(mComputeTask.begin(), mComputeTask.end(),
                                                    [&dependant](const auto& task) {return task.getName() == dependant;});

        const uint32_t computeTaskIndex = std::distance(mComputeTask.begin(), dependantComputeIt);
        uint32_t taskCounter = 0;
        for(uint32_t i = 0; i < mTaskOrder.size(); ++i)
        {
            if(taskCounter == computeTaskIndex)
                dependantTaskIndex = i;
            if(mTaskOrder[i].first == TaskType::Compute)
                ++taskCounter;
        }
    }
    else
    {
        const uint32_t graphicsTaskIndex = std::distance(mGraphicsTasks.begin(), dependantGraphicsIt);
        uint32_t taskCounter = 0;
        for(uint32_t i = 0; i < mTaskOrder.size(); ++i)
        {
            if(taskCounter == graphicsTaskIndex)
                dependantTaskIndex = i;
            if(mTaskOrder[i].first == TaskType::Graphics)
                ++taskCounter;
        }
    }
    }

    mTaskDependancies.push_back({dependancyTaskIndex, dependantTaskIndex});
}


void RenderGraph::bindResource(const std::string& name, const uint32_t index, const ResourceType resourcetype)
{
    uint32_t taskOrderIndex = 0;
	mDescriptorsNeedUpdating.resize(mTaskOrder.size());
	mFrameBuffersNeedUpdating.resize(mTaskOrder.size());
    for(const auto [taskType, taskIndex] : mTaskOrder)
    {
        RenderTask& task = getTask(taskType, taskIndex);

        uint32_t inputAttachmentIndex = 0;
        for(const auto& input : task.getInputAttachments())
        {
            if(input.first == name)
            {
                mInputResources[taskOrderIndex][inputAttachmentIndex] = {name, resourcetype, index, inputAttachmentIndex};
                mDescriptorsNeedUpdating[taskOrderIndex] = true;
                break; // Assume a resource is only bound once per task.
            }
            ++inputAttachmentIndex;
        }

        uint32_t outputAttachmentIndex = 0;
        for(const auto& input : task.getOuputAttachments())
        {
            if(input.first == name)
            {
                mOutputResources[taskOrderIndex][outputAttachmentIndex] = {name, resourcetype, index, outputAttachmentIndex};
                mFrameBuffersNeedUpdating[taskOrderIndex] = true;
                break;
            }
            ++outputAttachmentIndex;
        }

        ++taskOrderIndex;
    }
}


void RenderGraph::bindImage(const std::string& name, Image& image)
{
    const uint32_t currentImageIndex = mImages.size();
    mImages.push_back({name, image});

    bindResource(name, currentImageIndex, ResourceType::Image);
}


void RenderGraph::bindBuffer(const std::string& name , Buffer& buffer)
{
    const uint32_t currentBufferIndex = mBuffers.size();
    mBuffers.push_back({name, buffer});

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
    static bool hasReordered = false;

    if(hasReordered || mTaskDependancies.empty())
        return;

	std::vector<std::pair<TaskType, uint32_t>> newTaskOrder{};
    std::vector<std::vector<ResourceBindingInfo>> newInputBindings{};
    std::vector<std::vector<ResourceBindingInfo>> newOutputBindings{};

    newTaskOrder.reserve(mTaskOrder.size());
    newInputBindings.reserve(mTaskOrder.size());
    newOutputBindings.reserve(mTaskOrder.size());

	const uint32_t taskCount = mTaskOrder.size();

	for (uint32_t i = 0; i < taskCount; ++i)
	{
		std::vector<uint8_t> dependancyBitset(mTaskDependancies.size());

		for (uint32_t vertexIndex = 0; vertexIndex < mTaskDependancies.size(); ++vertexIndex)
		{
			dependancyBitset[mTaskDependancies[vertexIndex].second] = 1;
		}

		const uint32_t taskIndexToAdd = std::distance(dependancyBitset.begin(), std::find(dependancyBitset.begin(), dependancyBitset.end(), 0));

		newTaskOrder.push_back(mTaskOrder[taskIndexToAdd]);
        newInputBindings.push_back(std::move(mInputResources[taskIndexToAdd]));
        newOutputBindings.push_back((std::move(mOutputResources[taskIndexToAdd])));

		mTaskOrder.erase(mTaskOrder.begin() + taskIndexToAdd);

		for (uint32_t i = 0; i < mTaskDependancies.size(); ++i)
		{
			if (mTaskDependancies[i].first == taskIndexToAdd)
				mTaskDependancies.erase(mTaskDependancies.begin() + i);
		}
	}

	mTaskOrder.swap(newTaskOrder);
    mInputResources.swap(newInputBindings);
    mOutputResources.swap(newOutputBindings);

    hasReordered = true;
}


void RenderGraph::mergeTasks()
{
    static bool hasMerged = false;

    if(hasMerged)
        return;

    for(uint32_t i = 0; i < taskCount() - 1; ++i)
    {
        const auto [type, index] = mTaskOrder[i];
        const auto [secondType, secondIndex] = mTaskOrder[i + 1];

        auto& task1 = getTask(type, index);
        auto& task2 = getTask(secondType, secondIndex);

        if(areSupersets(task1, task2))
        {
            mergeTasks(task1, task2);

            // We souldn't need to worry about cleaning up any other tracking state as we should have already serialised by now.
            if(task2.taskType() == TaskType::Graphics)
            {
                mGraphicsTasks.erase(std::remove(mGraphicsTasks.begin(), mGraphicsTasks.end(), static_cast<GraphicsTask&>(task2)), mGraphicsTasks.end());
            } else
            {
                mComputeTask.erase(std::remove(mComputeTask.begin(), mComputeTask.end(), static_cast<ComputeTask&>(task2)), mComputeTask.end());
            }

            mTaskOrder.erase(mTaskOrder.begin() + i + 1);
        }
    }

    hasMerged = true;
}


bool RenderGraph::areSupersets(const RenderTask& task1, const RenderTask& task2)
{
    if(task1.taskType() != task2.taskType())
        return false;

    if(task1.getOuputAttachments() != task2.getOuputAttachments())
        return false;

    if(task1.getInputAttachments() != task2.getInputAttachments())
        return false;


    return true;
}


void RenderGraph::mergeTasks(RenderTask& task1, RenderTask& task2)
{
    if(task1.taskType() == TaskType::Graphics)
    {
        static_cast<GraphicsTask&>(task1).mergeDraws(static_cast<GraphicsTask&>(task2));
    } else
    {
        static_cast<ComputeTask&>(task1).mergeDispatches(static_cast<ComputeTask&>(task2));
    }

    task2.clearCalls();
}


void RenderGraph::recordBarriers()
{

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
	return TaskIterator{*this};
}


TaskIterator RenderGraph::taskEnd()
{
	return TaskIterator{*this, mTaskOrder.size()};
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
    return BindingIterator<BindingIteratorType::Output>{mOutputResources, *this};
}


BindingIterator< BindingIteratorType::Output> RenderGraph::outputBindingEnd()
{
    return BindingIterator<BindingIteratorType::Output>{mOutputResources, *this, mOutputResources.size()};
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
