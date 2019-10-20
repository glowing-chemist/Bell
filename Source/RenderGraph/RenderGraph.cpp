
#include "RenderGraph/RenderGraph.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/BellLogging.hpp"

#include <algorithm>
#include <set>


void RenderGraph::addTask(const GraphicsTask& task)
{
    const uint32_t taskIndex = static_cast<uint32_t>(mGraphicsTasks.size());
    mGraphicsTasks.push_back(task);

    mTaskOrder.push_back({TaskType::Graphics, taskIndex});
    // Also add a vulkan resources and inputs/outputs for each task, zero initialised
    mInputResources.emplace_back();
    mOutputResources.emplace_back();
    mFrameBuffersNeedUpdating.emplace_back();
    mDescriptorsNeedUpdating.emplace_back();
}


void RenderGraph::addTask(const ComputeTask& task)
{
	const uint32_t taskIndex = static_cast<uint32_t>(mComputeTasks.size());
	mComputeTasks.push_back(task);

    mTaskOrder.push_back({TaskType::Compute, taskIndex});

    // Also add a vulkan resources and inputs/outputs for each task, zero initialised
    mInputResources.emplace_back();
    mOutputResources.emplace_back();
    mFrameBuffersNeedUpdating.emplace_back();
    mDescriptorsNeedUpdating.emplace_back();
}


void RenderGraph::addDependancy(const std::string& dependancy, const std::string& dependant)
{
	// Find the task index in mGraphicsTasks/mComputeTaskss
    uint32_t dependancyTaskIndex = 0;
    uint32_t dependantTaskIndex = 0;
    {
    auto dependancyGraphicsIt = std::find_if(mGraphicsTasks.begin(), mGraphicsTasks.end(),
                                            [&dependancy](auto& task) {return task.getName() == dependancy;});


    if(dependancyGraphicsIt == mGraphicsTasks.end())
    {
		auto dependancyComputeIt = std::find_if(mComputeTasks.begin(), mComputeTasks.end(),
                                                    [&dependancy](const auto& task) {return task.getName() == dependancy;});

		const uint32_t computeTaskIndex = static_cast<uint32_t>(std::distance(mComputeTasks.begin(), dependancyComputeIt));
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
        const uint32_t graphicsTaskIndex = static_cast<uint32_t>(std::distance(mGraphicsTasks.begin(), dependancyGraphicsIt));
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
		auto dependantComputeIt = std::find_if(mComputeTasks.begin(), mComputeTasks.end(),
                                                    [&dependant](const auto& task) {return task.getName() == dependant;});

		const uint32_t computeTaskIndex = static_cast<uint32_t>(std::distance(mComputeTasks.begin(), dependantComputeIt));
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
        const uint32_t graphicsTaskIndex = static_cast<uint32_t>(std::distance(mGraphicsTasks.begin(), dependantGraphicsIt));
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


void RenderGraph::compileDependancies()
{
	std::set<std::pair<uint32_t, uint32_t>> dependancies;

    for(size_t i = 0; i < mTaskOrder.size(); ++i)
    {
		const RenderTask& outerTask = getTask(mTaskOrder[i].first, mTaskOrder[i].second);

		bool outerDepthWrite = false;
		{
			if(outerTask.taskType() == TaskType::Graphics)
			{
				const auto& outerGraphicsTask = static_cast<const GraphicsTask&>(outerTask);

				outerDepthWrite = outerGraphicsTask.getPipelineDescription().mDepthWrite;
			}
		}

        for(size_t j = 0; j < mTaskOrder.size(); ++j)
        {
            // Don't generate dependancies between the same task (shouldn't be possible anyway).
            if(i == j)
                continue;

			const RenderTask& innerTask = getTask(mTaskOrder[j].first, mTaskOrder[j].second);

            // generate dependancies between framebuffer writes and "descriptor" reads.
            {
                const std::vector<RenderTask::OutputAttachmentInfo>& outResources = outerTask.getOuputAttachments();
                const std::vector<std::pair<std::string, AttachmentType>>& inResources = innerTask.getInputAttachments();

                for(size_t outerIndex = 0; outerIndex < outResources.size(); ++outerIndex)
                {
                    for(size_t innerIndex = 0; innerIndex < inResources.size(); ++innerIndex)
                    {						
						if(outResources[outerIndex].mName == inResources[innerIndex].first &&
								!(outResources[outerIndex].mType == AttachmentType::Depth && !outerDepthWrite))
                        {
                            // Innertask reads from a
							dependancies.insert({i, j});
                        }
                    }
                }
            }

			// generate dependancies between descriptor -> descriptor resources e.g structured buffers/ImageStores.
            {
                const std::vector<std::pair<std::string, AttachmentType>>& outResources = outerTask.getInputAttachments();
				const std::vector<std::pair<std::string, AttachmentType>>& innerResources = innerTask.getInputAttachments();

                for(size_t outerIndex = 0; outerIndex < outResources.size(); ++outerIndex)
                {
					for(size_t innerIndex = 0; innerIndex < innerResources.size(); ++innerIndex)
                    {
						if(outResources[outerIndex].first == innerResources[innerIndex].first &&
                                (outResources[outerIndex].second == AttachmentType::Image1D ||
                                 outResources[outerIndex].second == AttachmentType::Image2D ||
                                 outResources[outerIndex].second == AttachmentType::Image3D ||
                                 outResources[outerIndex].second == AttachmentType::DataBuffer ||
                                 outResources[outerIndex].second == AttachmentType::IndirectBuffer))
                        {
                            // Innertask reads from a
							dependancies.insert({i, j});
                        }
                    }
                }
            }

			// generate dependancies for depth -> depth (output, output)
			{
				const std::vector<RenderTask::OutputAttachmentInfo>& outResources = outerTask.getOuputAttachments();
				const std::vector<RenderTask::OutputAttachmentInfo>& inResources = innerTask.getOuputAttachments();

				for(size_t outerIndex = 0; outerIndex < outResources.size(); ++outerIndex)
				{
					for(size_t innerIndex = 0; innerIndex < inResources.size(); ++innerIndex)
					{
						if(outResources[outerIndex].mType == AttachmentType::Depth && inResources[innerIndex].mType == AttachmentType::Depth
								&& outResources[outerIndex].mName == inResources[innerIndex].mName
								&& outerDepthWrite)
						{
							dependancies.insert({i, j});
						}
					}
				}
			}
        }
    }

	mTaskDependancies.insert(mTaskDependancies.begin(), dependancies.begin(), dependancies.end());
}


void RenderGraph::bindResource(const std::string& name, const uint32_t index, const ResourceType resourcetype)
{
    uint32_t taskOrderIndex = 0;
	for(const auto& [taskType, taskIndex] : mTaskOrder)
    {
        RenderTask& task = getTask(taskType, taskIndex);

        uint32_t inputAttachmentIndex = 0;
        for(const auto& input : task.getInputAttachments())
        {
            if(input.first == name)
            {
                mInputResources[taskOrderIndex].push_back({name, resourcetype, index, inputAttachmentIndex});
                mDescriptorsNeedUpdating[taskOrderIndex] = true;
                break; // Assume a resource is only bound once per task.
            }
            ++inputAttachmentIndex;
        }

        uint32_t outputAttachmentIndex = 0;
        for(const auto& input : task.getOuputAttachments())
        {
            if(input.mName == name)
            {
                mOutputResources[taskOrderIndex].push_back({name, resourcetype, index, outputAttachmentIndex});
                mFrameBuffersNeedUpdating[taskOrderIndex] = true;
                break;
            }
            ++outputAttachmentIndex;
        }

        ++taskOrderIndex;
    }
}


void RenderGraph::bindImage(const std::string& name, const ImageView &image)
{
    const uint32_t currentImageIndex = static_cast<uint32_t>(mImageViews.size());
    mImageViews.emplace_back(name, image);

    bindResource(name, currentImageIndex, ResourceType::Image);
}


void RenderGraph::bindImageArray(const std::string& name, const ImageViewArray& imageArray)
{
	const uint32_t currentImageArrayIndex = static_cast<uint32_t>(mImageViewArrays.size());
	mImageViewArrays.emplace_back(name, imageArray);	

	bindResource(name, currentImageArrayIndex, ResourceType::ImageArray);
}


void RenderGraph::bindBuffer(const std::string& name , const BufferView& buffer)
{
	const uint32_t currentBufferIndex = static_cast<uint32_t>(mBufferViews.size());
	mBufferViews.emplace_back(name, buffer);

    bindResource(name, currentBufferIndex, ResourceType::Buffer);
}


void RenderGraph::bindSampler(const std::string& name, const Sampler& type)
{
    const uint32_t samplerIndex = static_cast<uint32_t>(mSamplers.size());
    mSamplers.emplace_back(name, type);

    bindResource(name, samplerIndex, ResourceType::Sampler);
}


void RenderGraph::bindVertexBuffer(const Buffer& buffer)
{
	mVertexBuffer.reset();
	mVertexBuffer = buffer;
}


void RenderGraph::bindIndexBuffer(const Buffer& buffer)
{
	mIndexBuffer.reset();
    mIndexBuffer = buffer;
}


void RenderGraph::generateNonPersistentImages(RenderDevice* dev)
{
	// Currently (and probably will only ever) support creating non persistent resources for graphics tasks.
	for(const auto& task : mGraphicsTasks)
	{
		const auto& outputs = task.getOuputAttachments();

		for(const auto& output : outputs)
		{
			// Means it is a persistent resource.
			if(output.mSize == SizeClass::Custom)
				continue;

			const auto [width, height] = [=]() -> std::pair<uint32_t, uint32_t>
			{
				const uint32_t swapChainWidth = dev->getSwapChain()->getSwapChainImageWidth();
				const uint32_t swapChainHeight = dev->getSwapChain()->getSwapChainImageHeight();

				switch(output.mSize)
				{
					case SizeClass::Swapchain:
						return {swapChainWidth, swapChainHeight};

					case SizeClass::HalfSwapchain:
						return {swapChainWidth / 2, swapChainHeight / 2 };

					case SizeClass::QuarterSwapchain:
						return {swapChainWidth / 4, swapChainHeight / 4 };

					default:
						BELL_TRAP;
						return {swapChainWidth, swapChainHeight};
				}
			}();

			Image generatedImage(dev, output.mFormat, (output.mType == AttachmentType::Depth ?
									 ImageUsage::DepthStencil :  ImageUsage::ColourAttachment) | ImageUsage::Sampled,
								 width, height, 1, 1, 1, 1, output.mName);

			ImageView view{generatedImage, output.mType == AttachmentType::Depth ?
												ImageViewType::Depth :  ImageViewType::Colour};

			mNonPersistentImages.emplace_back(generatedImage, view);

			bindImage(output.mName, view);
		}
	}
}


void RenderGraph::reorderTasks()
{
	// Indicates the graph has already been reordered
	if(mTaskDependancies.empty())
		return;

	std::vector<std::pair<TaskType, uint32_t>> newTaskOrder{};
    std::vector<std::vector<ResourceBindingInfo>> newInputBindings{};
    std::vector<std::vector<ResourceBindingInfo>> newOutputBindings{};
    std::vector<bool> newFrameBuffersNeedUpdating{};
    std::vector<bool> newDescriptorsNeedUpdating{};
	TaskType previousTaskType = TaskType::Compute;

    newTaskOrder.reserve(mTaskOrder.size());
    newInputBindings.reserve(mTaskOrder.size());
    newOutputBindings.reserve(mTaskOrder.size());

    const uint32_t taskCount = static_cast<uint32_t>(mTaskOrder.size());

	// keep track of tasks that have already been added to meet other tasks dependancies.
	// This stops us readding moved from tasks.
	std::vector<uint8_t> usedDependants(taskCount);
	std::vector<uint8_t> dependancyBitset(taskCount);

	for (uint32_t i = 0; i < taskCount; ++i)
	{
		// clear the dependancy flags
		std::memcpy(dependancyBitset.data(), usedDependants.data(), dependancyBitset.size());

		for (const auto& dependancy : mTaskDependancies)
		{
			dependancyBitset[dependancy.second] = 1;
		}

		const uint32_t taskIndexToAdd = selectNextTask(dependancyBitset, previousTaskType);

		// Keep track of the previous task type.
		previousTaskType = mTaskOrder[taskIndexToAdd].first;

		usedDependants[taskIndexToAdd] = 1;

		newTaskOrder.push_back(mTaskOrder[taskIndexToAdd]);
        newInputBindings.push_back(std::move(mInputResources[taskIndexToAdd]));
        newOutputBindings.push_back((std::move(mOutputResources[taskIndexToAdd])));
        newFrameBuffersNeedUpdating.push_back(mFrameBuffersNeedUpdating[taskIndexToAdd]);
        newDescriptorsNeedUpdating.push_back(mDescriptorsNeedUpdating[taskIndexToAdd]);

		uint32_t dependancyIndex = 0;
		for (const auto& dependancy : mTaskDependancies)
		{
			if (dependancy.first == taskIndexToAdd)
			{
				mTaskDependancies.erase(mTaskDependancies.begin() + dependancyIndex);
			}
			++dependancyIndex;
		}
	}

	mTaskOrder.swap(newTaskOrder);
    mInputResources.swap(newInputBindings);
    mOutputResources.swap(newOutputBindings);
    mFrameBuffersNeedUpdating.swap(newFrameBuffersNeedUpdating);
    mDescriptorsNeedUpdating.swap(mDescriptorsNeedUpdating);
}


uint32_t RenderGraph::selectNextTask(const std::vector<uint8_t>& dependancies, const TaskType taskType) const
{
	// maps from index to score
	std::vector<std::pair<uint32_t, uint32_t>> prospectiveTaskIndicies;
	prospectiveTaskIndicies.reserve(dependancies.size());

	for(uint8_t index = 0; index < dependancies.size(); ++index)
	{
		// This task has no unmet dependancies so calculate its score
		if(dependancies[index] == 0)
		{
			uint8_t taskCompatibilityScore = 0;

			const auto& task = getTask(mTaskOrder[index].first, mTaskOrder[index].second);

			// Won't require swapping fom compute -> graphics or vice versa
			if(task.taskType() == taskType)
				taskCompatibilityScore += 30;

			// Take in to account how much work each task will do
			taskCompatibilityScore += task.recordedCommandCount();

			// Calculate how many other tasks depend on this one, prioritise tasks that have a larger
			// number of dependants.
			uint32_t dependantTasks = 0;
			for(const auto& dependancy : mTaskDependancies)
			{
				if(dependancy.first == index)
					dependantTasks++;
			}
			// Value it highly that tasks with lots of dependants are scheduled early.
			taskCompatibilityScore += 10 * dependantTasks;

			prospectiveTaskIndicies.push_back({index, taskCompatibilityScore});
		}
	}

	const auto maxElementIt = std::max_element(prospectiveTaskIndicies.begin(), prospectiveTaskIndicies.end(),
														[](const auto& p1, const auto& p2)
	{
		return p1.second < p2.second;
	});

	return (*maxElementIt).first;
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
				mComputeTasks.erase(std::remove(mComputeTasks.begin(), mComputeTasks.end(), static_cast<ComputeTask&>(task2)), mComputeTasks.end());
            }

            mTaskOrder.erase(mTaskOrder.begin() + i + 1);
            mInputResources.erase(mInputResources.begin() + i + 1);
            mOutputResources.erase(mOutputResources.begin() + i + 1);
        }
    }

    hasMerged = true;
}


bool RenderGraph::areSupersets(const RenderTask& task1, const RenderTask& task2)
{
    if(task1.taskType() != task2.taskType())
        return false;

    bool isFrameBufferSubset = std::includes(task1.getOuputAttachments().begin(), task1.getOuputAttachments().end(),
                                  task2.getOuputAttachments().begin(), task2.getOuputAttachments().end());

    isFrameBufferSubset |= std::includes(task2.getOuputAttachments().begin(), task2.getOuputAttachments().end(),
                                  task1.getOuputAttachments().begin(), task1.getOuputAttachments().end());

    bool isDescriptorSubset = std::includes(task1.getInputAttachments().begin(), task1.getInputAttachments().end(),
                              task2.getInputAttachments().begin(), task2.getInputAttachments().end());

    isDescriptorSubset |= std::includes(task2.getInputAttachments().begin(), task2.getInputAttachments().end(),
                              task1.getInputAttachments().begin(), task1.getInputAttachments().end());

    return isFrameBufferSubset && isDescriptorSubset;
}


void RenderGraph::mergeTasks(RenderTask& task1, RenderTask& task2)
{
	task1.mergeWith(task2);

    task2.clearCalls();
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
				return mComputeTasks[taskIndex];
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
				return mComputeTasks[taskIndex];
        }
    }();

    return task;
}


Sampler& RenderGraph::getSampler(const uint32_t index)
{
    BELL_ASSERT(index < mSamplers.size(), " Attempting to fetch non sampler resource")

    return mSamplers[index].second;
}


ImageView& RenderGraph::getImageView(const uint32_t index)
{
    BELL_ASSERT(index < mImageViews.size(), " Attempting to fetch non imageView resource")

    return mImageViews[index].second;
}


ImageViewArray& RenderGraph::getImageArrayViews(const uint32_t index)
{
	BELL_ASSERT(index < mImageViewArrays.size(), "Attempting to fetch non imageViewArray resource")

	return mImageViewArrays[index].second;
}


BufferView& RenderGraph::getBuffer(const uint32_t index)
{
	BELL_ASSERT(index < mBufferViews.size(), " Attempting to fetch non buffer resource")

	return mBufferViews[index].second;
}


const BufferView& RenderGraph::getBoundBuffer(const std::string& name) const
{
	auto itr = std::find_if(mBufferViews.begin(), mBufferViews.end(), [&name](const std::pair<std::string, BufferView>& entry) { return name == entry.first; });

	BELL_ASSERT(itr != mBufferViews.end(), "Buffer not found")

    return (*itr).second;
}


const ImageView& RenderGraph::getBoundImageView(const std::string& name) const
{
    auto itr = std::find_if(mImageViews.begin(), mImageViews.end(), [&name](const std::pair<std::string, ImageView>& entry) { return name == entry.first; });

    BELL_ASSERT(itr != mImageViews.end(), "Image not found")

    return (*itr).second;
}


void RenderGraph::reset()
{
	// Clear all bound resources
    mImageViews.clear();
	mBufferViews.clear();
	mSamplers.clear();

	mInputResources.clear();
	mOutputResources.clear();

	// clear non-persistent resources
	mNonPersistentImages.clear();

	// Clear all jobs
	mGraphicsTasks.clear();
	mComputeTasks.clear();
	mTaskOrder.clear();
	mTaskDependancies.clear();
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
