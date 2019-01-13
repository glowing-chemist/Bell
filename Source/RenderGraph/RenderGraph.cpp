
#include "RenderGraph/RenderGraph.hpp"

#include <algorithm>

void RenderGraph::addTask(const GraphicsTask& task)
{
    const uint32_t taskIndex = mGraphicsTasks.size();
    mGraphicsTasks.push_back(task);

    mTaskOrder.push_back({TaskType::Graphics, taskIndex});
}


void RenderGraph::addTask(const ComputeTask& task)
{
    const uint32_t taskIndex = mComputeTask.size();
    mComputeTask.push_back(task);

    mTaskOrder.push_back({TaskType::Compute, taskIndex});
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

        uint32_t inputAttachmentIndex = 0;
        for(const auto& input : task.getInputAttachments())
        {
            if(input.first == name)
            {
                mInputResources[taskOrderIndex][inputAttachmentIndex] = {resourcetype, index};
                break; // Assume a resource is only bound once.
            }
            ++inputAttachmentIndex;
        }

        uint32_t outputAttachmentIndex = 0;
        for(const auto& input : task.getOuputAttachments())
        {
            if(input.first == name)
            {
                mOutputResources[taskOrderIndex][outputAttachmentIndex] = {resourcetype, index};
                break;
            }
            ++outputAttachmentIndex;
        }

        ++taskOrderIndex;
    }
}


void RenderGraph::bindImage(const std::string& name, Buffer& buffer)
{
    const uint32_t currentBufferIndex = mBuffers.size();
    mBuffers.push_back({name, buffer});

    bindResource(name, currentBufferIndex, ResourceType::Image);
}

void RenderGraph::bindBuffer(const std::string& name , Image& image)
{
    const uint32_t currentImageIndex = mImages.size();
    mImages.push_back({name, image});

    bindResource(name, currentImageIndex, ResourceType::Image);
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

