
#include "RenderGraph/RenderGraph.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/BellLogging.hpp"
#include "Core/ConversionUtils.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <string>

#ifdef _MSC_VER
#undef max
#undef min
#endif


TaskID RenderGraph::addTask(const GraphicsTask& task)
{
    const uint32_t taskIndex = static_cast<uint32_t>(mGraphicsTasks.size());
    mGraphicsTasks.push_back(task);

    mTaskOrder.push_back({TaskType::Graphics, taskIndex});
    // Also add a vulkan resources and inputs/outputs for each task, zero initialised
    mInputResources.emplace_back();
    mOutputResources.emplace_back();
    mFrameBuffersNeedUpdating.push_back(false);
    mDescriptorsNeedUpdating.push_back(false);

	return taskIndex + 1;
}


TaskID RenderGraph::addTask(const ComputeTask& task)
{
	const uint32_t taskIndex = static_cast<uint32_t>(mComputeTasks.size());
	mComputeTasks.push_back(task);

    mTaskOrder.push_back({TaskType::Compute, taskIndex});

    // Also add a vulkan resources and inputs/outputs for each task, zero initialised
    mInputResources.emplace_back();
    mOutputResources.emplace_back();
    mFrameBuffersNeedUpdating.push_back(false);
    mDescriptorsNeedUpdating.push_back(false);

	return -static_cast<TaskID>(taskIndex) - 1;
}


RenderTask& RenderGraph::getTask(const TaskID id)
{
	BELL_ASSERT(id != 0, "0 is an invalid task ID");

	if (id > 0)
	{
		BELL_ASSERT(id - 1 < mGraphicsTasks.size(), "Invalid graphics task ID");
		return mGraphicsTasks[id - 1];
	}
	else
	{
		BELL_ASSERT(-id - 1 < mComputeTasks.size(), "Invalid compute task ID");
		return mComputeTasks[-id - 1];
	}
}


void RenderGraph::addDependancy(const RenderTask& dependancy, const RenderTask& dependant)
{
	addDependancy(dependancy.getName(), dependant.getName());
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


void RenderGraph::compile(RenderDevice* dev)
{
	compileDependancies();
	generateInternalResources(dev);
	reorderTasks();
	mergeTasks();
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
                const std::vector<RenderTask::InputAttachmentInfo>& inResources = innerTask.getInputAttachments();
                bool dependancyFound = false;

                for(size_t outerIndex = 0; outerIndex < outResources.size(); ++outerIndex)
                {
                    for(size_t innerIndex = 0; innerIndex < inResources.size(); ++innerIndex)
                    {						
						if(outResources[outerIndex].mName == inResources[innerIndex].mName &&
								!(outResources[outerIndex].mType == AttachmentType::Depth && !outerDepthWrite))
                        {
                            // Innertask reads from a
							dependancies.insert({i, j});

                            dependancyFound = true;
                            break;
                        }
                    }
                    if(dependancyFound)
                        break;
                }
            }

			// generate dependancies between descriptor -> descriptor resources e.g structured buffers/ImageStores.
            {
                const std::vector<RenderTask::InputAttachmentInfo>& outResources = outerTask.getInputAttachments();
				const std::vector<RenderTask::InputAttachmentInfo>& innerResources = innerTask.getInputAttachments();

                bool dependancyFound = false;
                for(size_t outerIndex = 0; outerIndex < outResources.size(); ++outerIndex)
                {
					for(size_t innerIndex = 0; innerIndex < innerResources.size(); ++innerIndex)
                    {
						if(outResources[outerIndex].mName == innerResources[innerIndex].mName &&
                                // Assumes ImageND are used as read write or read Only.
                                (outResources[outerIndex].mType == AttachmentType::Image1D ||
                                 outResources[outerIndex].mType == AttachmentType::Image2D ||
                                 outResources[outerIndex].mType == AttachmentType::Image3D ||
                                 // If the outer task writes to the buffer and the inner reads we need
                                 // to emit a dependancy to enforce writing before reading.
                                 ((outResources[outerIndex].mType == AttachmentType::DataBufferWO ||
                                  outResources[outerIndex].mType == AttachmentType::DataBufferRW) &&
                                  (innerResources[innerIndex].mType == AttachmentType::DataBufferRO ||
                                  innerResources[innerIndex].mType == AttachmentType::DataBufferRW ||
                                   innerResources[innerIndex].mType == AttachmentType::VertexBuffer ||
                                   innerResources[innerIndex].mType == AttachmentType::IndexBuffer)) ||
                                 // Indirect buffers are readOnly (written as (WO) data buffers)
                                 innerResources[innerIndex].mType == AttachmentType::IndirectBuffer))
                        {
                            // Innertask reads from a
							dependancies.insert({i, j});

                            dependancyFound = true;
                            break;
                        }
                    }
                    if(dependancyFound)
                        break;
                }
            }

			// generate dependancies for depth -> depth (output, output)
			{
				const std::vector<RenderTask::OutputAttachmentInfo>& outResources = outerTask.getOuputAttachments();
				const std::vector<RenderTask::OutputAttachmentInfo>& inResources = innerTask.getOuputAttachments();
                bool dependancyFound = false;

				for(size_t outerIndex = 0; outerIndex < outResources.size(); ++outerIndex)
				{
					for(size_t innerIndex = 0; innerIndex < inResources.size(); ++innerIndex)
					{
						if(outResources[outerIndex].mType == AttachmentType::Depth && inResources[innerIndex].mType == AttachmentType::Depth
								&& outResources[outerIndex].mName == inResources[innerIndex].mName
								&& outerDepthWrite)
						{
							dependancies.insert({i, j});

                            dependancyFound = true;
                            break;
						}
					}

                    if(dependancyFound)
                        break;
				}
			}
        }
    }

    mTaskDependancies.insert(mTaskDependancies.end(), dependancies.begin(), dependancies.end());
}


void RenderGraph::bindResource(const std::string& name, const uint32_t index, const ResourceType resourcetype)
{
    uint32_t taskOrderIndex = 0;
	for(const auto& [taskType, taskIndex] : mTaskOrder)
    {
        const RenderTask& task = getTask(taskType, taskIndex);

        uint32_t inputAttachmentIndex = 0;
        for(const auto& input : task.getInputAttachments())
        {
            if(input.mName == name)
            {
                mInputResources[taskOrderIndex].push_back({name, resourcetype, index, inputAttachmentIndex});
                mDescriptorsNeedUpdating[taskOrderIndex] = true;
                break; // Assume a resource is only bound once per task.
            }

            // These don't take up slots so don't increment the input index.
            if(input.mType != AttachmentType::VertexBuffer && input.mType != AttachmentType::IndexBuffer)
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


void RenderGraph::bindVertexBuffer(const std::string& name, const BufferView& buffer)
{
    const uint32_t currentBufferIndex = static_cast<uint32_t>(mBufferViews.size());
    mBufferViews.emplace_back(name, buffer);

    bindResource(name, currentBufferIndex, ResourceType::VertexBuffer);
}


void RenderGraph::bindIndexBuffer(const std::string& name, const BufferView& buffer)
{
    const uint32_t currentBufferIndex = static_cast<uint32_t>(mBufferViews.size());
    mBufferViews.emplace_back(name, buffer);

    bindResource(name, currentBufferIndex, ResourceType::IndexBuffer);
}


void RenderGraph::bindSampler(const std::string& name, const Sampler& type)
{
    const uint32_t samplerIndex = static_cast<uint32_t>(mSamplers.size());
    mSamplers.emplace_back(name, type);

    bindResource(name, samplerIndex, ResourceType::Sampler);
}


void RenderGraph::bindShaderResourceSet(const std::string& name, const ShaderResourceSet& set)
{
	mSRS.emplace_back(name, set);
}


void RenderGraph::bindInternalResources()
{
	for (const auto& resourceEntry : mInternalResources)
	{
		bindImage(resourceEntry.mSlot, *resourceEntry.mResourceView);
	}
}


void RenderGraph::createTransientImage(RenderDevice* dev, const std::string& name, const Format format, const ImageUsage usage, const SizeClass size)
{
    const auto [width, height] = [=]() -> std::pair<uint32_t, uint32_t>
    {
        const uint32_t swapChainWidth = dev->getSwapChain()->getSwapChainImageWidth();
        const uint32_t swapChainHeight = dev->getSwapChain()->getSwapChainImageHeight();

        switch(size)
        {
            case SizeClass::Swapchain:
                return {swapChainWidth, swapChainHeight};

            case SizeClass::HalfSwapchain:
                return {swapChainWidth / 2, swapChainHeight / 2 };

            case SizeClass::QuarterSwapchain:
                return {swapChainWidth / 4, swapChainHeight / 4 };

			case SizeClass::DoubleSwapchain:
				return { swapChainWidth * 2, swapChainHeight * 2 };

			case SizeClass::QuadrupleSwapChain:
				return { swapChainWidth * 4, swapChainHeight * 4 };

            default:
                BELL_TRAP;
                return {swapChainWidth, swapChainHeight};
        }
    }();

    Image generatedImage(dev, format, usage | ImageUsage::Transient,
                         width, height, 1, 1, 1, 1, name);

    ImageView view{generatedImage, usage & ImageUsage::DepthStencil ?
                                        ImageViewType::Depth :  ImageViewType::Colour};

    mNonPersistentImages.emplace_back(generatedImage, view);

    bindImage(name, view);
}


void RenderGraph::createInternalResource(RenderDevice* dev, const std::string& name, const Format format, const ImageUsage usage, const SizeClass size)
{
	const auto [width, height] = [=]() -> std::pair<uint32_t, uint32_t>
	{
		const uint32_t swapChainWidth = dev->getSwapChain()->getSwapChainImageWidth();
		const uint32_t swapChainHeight = dev->getSwapChain()->getSwapChainImageHeight();

		switch (size)
		{
		case SizeClass::Swapchain:
			return { swapChainWidth, swapChainHeight };

		case SizeClass::HalfSwapchain:
			return { swapChainWidth / 2, swapChainHeight / 2 };

		case SizeClass::QuarterSwapchain:
			return { swapChainWidth / 4, swapChainHeight / 4 };

		case SizeClass::DoubleSwapchain:
			return { swapChainWidth * 2, swapChainHeight * 2 };

		case SizeClass::QuadrupleSwapChain:
			return { swapChainWidth * 4, swapChainHeight * 4 };

		default:
			BELL_TRAP;
			return { swapChainWidth, swapChainHeight };
		}
	}();

	PerFrameResource<Image> generatedImage(dev, format, usage,
		width, height, 1, 1, 1, 1, name);

	PerFrameResource<ImageView> view{ generatedImage, usage & ImageUsage::DepthStencil ?
										ImageViewType::Depth : ImageViewType::Colour };

	mInternalResources.emplace_back(name, generatedImage, view);
}


void RenderGraph::generateInternalResources(RenderDevice* dev)
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

            createInternalResource(dev,
                                 output.mName,
                                 output.mFormat,
                                 (output.mType == AttachmentType::Depth ? ImageUsage::DepthStencil :  ImageUsage::ColourAttachment) | ImageUsage::Sampled,
                                 output.mSize);
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
		
		mTaskDependancies.erase(std::remove_if(mTaskDependancies.begin(), mTaskDependancies.end(), [=](const auto& dependancy)
			{
				return dependancy.first == taskIndexToAdd;
			}), mTaskDependancies.end());
	}

	mTaskOrder.swap(newTaskOrder);
    mInputResources.swap(newInputBindings);
    mOutputResources.swap(newOutputBindings);
    mFrameBuffersNeedUpdating.swap(newFrameBuffersNeedUpdating);
    mDescriptorsNeedUpdating.swap(mDescriptorsNeedUpdating);

#if 0 // Enable to print out task submission order.

	BELL_LOG("Task submission order:");

	for (const auto& [type, index] : mTaskOrder)
	{
		const auto& task = getTask(type, index);
		BELL_LOG_ARGS("%s", task.getName().c_str());
	}

#endif
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

	BELL_ASSERT(maxElementIt != prospectiveTaskIndicies.end(), "Unable to find next task")

	return (*maxElementIt).first;
}


void RenderGraph::mergeTasks()
{
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
}


bool RenderGraph::areSupersets(const RenderTask& task1, const RenderTask& task2)
{
    if(task1.taskType() != task2.taskType())
        return false;

	auto outputPred = [](const RenderTask::OutputAttachmentInfo& lhs, const RenderTask::OutputAttachmentInfo& rhs)
	{
		return lhs.mName == rhs.mName; // Should be strong enough?
	};

	auto inputPred = [](const auto& lhs, const auto& rhs)
	{
		return lhs.mName == rhs.mName; // Should be strong enough?
	};

	bool isFrameBufferSubset = std::search(task1.getOuputAttachments().begin(), task1.getOuputAttachments().end(),
								  task2.getOuputAttachments().begin(), task2.getOuputAttachments().end(), outputPred) == task1.getOuputAttachments().begin();

	isFrameBufferSubset = isFrameBufferSubset || std::search(task2.getOuputAttachments().begin(), task2.getOuputAttachments().end(),
								  task1.getOuputAttachments().begin(), task1.getOuputAttachments().end(), outputPred) == task2.getOuputAttachments().begin();

	bool isDescriptorSubset = std::search(task1.getInputAttachments().begin(), task1.getInputAttachments().end(),
							  task2.getInputAttachments().begin(), task2.getInputAttachments().end(), inputPred) == task1.getInputAttachments().begin();

	isDescriptorSubset = isDescriptorSubset || std::search(task2.getInputAttachments().begin(), task2.getInputAttachments().end(),
							  task1.getInputAttachments().begin(), task1.getInputAttachments().end(), inputPred) == task2.getInputAttachments().begin();

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
	const auto it = std::find_if(mBufferViews.begin(), mBufferViews.end(), [&name](const std::pair<std::string, BufferView>& entry) { return name == entry.first; });

	BELL_ASSERT(it != mBufferViews.end(), "Buffer not found")

    return (*it).second;
}


const ImageView& RenderGraph::getBoundImageView(const std::string& name) const
{
    const auto it = std::find_if(mImageViews.begin(), mImageViews.end(), [&name](const std::pair<std::string, ImageView>& entry) { return name == entry.first; });

    BELL_ASSERT(it != mImageViews.end(), "Image not found")

    return (*it).second;
}


const ShaderResourceSet& RenderGraph::getBoundShaderResourceSet(const std::string& slot) const
{
	const auto it = std::find_if(mSRS.begin(), mSRS.end(), [&slot](const auto& s) {
		return s.first == slot;
		});

	BELL_ASSERT(it != mSRS.end(), "SRS not found")

	return (*it).second;
}


std::vector<BarrierRecorder> RenderGraph::generateBarriers(RenderDevice* dev)
{
	std::vector<BarrierRecorder> barriers{};
	barriers.reserve(mTaskOrder.size());

	struct ResourceUsageEntries
	{
		ImageView* mImage;
		BufferView* mBuffer;

		struct taskIndex
		{
			uint32_t mTaskIndex;
			AttachmentType mStateRequired;
		};
		std::vector<taskIndex> mRequiredStates;
	};

	std::map<uint32_t, ResourceUsageEntries> resourceEntries;
	std::unordered_map<std::string, uint32_t> resourceIndicies;
	uint32_t nextResourceIndex = 0;

	// Gather when all resources are used/how.
	for (uint32_t i = 0; i < mTaskOrder.size(); ++i)
	{
		barriers.emplace_back(dev);

		const auto& inputResources = mInputResources[i];
		const auto& outputResources = mOutputResources[i];
		const auto& [type, index] = mTaskOrder[i];
		const auto& task = getTask(type, index);

		for (auto j = 0u; j < std::max(inputResources.size(), outputResources.size()); ++j)
		{
			if (j < inputResources.size())
			{
				const auto& resource = inputResources[j];

				uint32_t resourceIndex = ~0u;

				if (resource.mResourcetype == RenderGraph::ResourceType::Image)
				{
					if (resourceIndicies.find(resource.mName) == resourceIndicies.end())
					{
						resourceIndex = nextResourceIndex++;
						resourceIndicies[resource.mName] = resourceIndex;
						ImageView& imageView = getImageView(resource.mResourceIndex);
						resourceEntries[resourceIndex].mImage = &imageView;
					}
					else
						resourceIndex = resourceIndicies[resource.mName];

				}
				else if (resource.mResourcetype == RenderGraph::ResourceType::Buffer)
				{
					if (resourceIndicies.find(resource.mName) == resourceIndicies.end())
					{
						resourceIndex = nextResourceIndex++;
						resourceIndicies[resource.mName] = resourceIndex;
						BufferView& bufView = getBuffer(resource.mResourceIndex);
						resourceEntries[resourceIndex].mBuffer = &bufView;
					}
					else
						resourceIndex = resourceIndicies[resource.mName];
				}

				resourceEntries[resourceIndex].mRequiredStates.push_back({ i, task.getInputAttachments()[resource.mResourceBinding].mType });
			}

			if (j < outputResources.size())
			{
				const auto& resource = outputResources[j];

				if (resource.mResourcetype == RenderGraph::ResourceType::Image)
				{
					uint32_t resourceIndex;

					if (resourceIndicies.find(resource.mName) == resourceIndicies.end())
					{
						resourceIndex = nextResourceIndex++;
						resourceIndicies[resource.mName] = resourceIndex;
						ImageView & imageView = getImageView(resource.mResourceIndex);
						resourceEntries[resourceIndex].mImage = &imageView;
					}
					else
						resourceIndex = resourceIndicies[resource.mName];

					resourceEntries[resourceIndex].mRequiredStates.push_back({ i, task.getOuputAttachments()[resource.mResourceBinding].mType });
				}
			}
		}
	}

// Enable for barrier summary
#if 0
	for (const auto& [name, index] : resourceIndicies)
	{
		BELL_LOG_ARGS("Resource %s", name.c_str());

		for (const auto& entry : resourceEntries[index].mRequiredStates)
		{
			BELL_LOG_ARGS("Needed as %s in task %d", getAttachmentName(entry.mStateRequired), entry.mTaskIndex);
		}
	}
#endif

	for (const auto& [index, entries] : resourceEntries)
	{
		// intended overflow.
		ResourceUsageEntries::taskIndex previous{~0u, AttachmentType::PushConstants};

		if (entries.mImage)
		{
			const auto layout = (*entries.mImage)->getImageLayout((*entries.mImage)->getBaseLevel(), (*entries.mImage)->getBaseMip());
			previous.mStateRequired = getAttachmentType(layout);
		}
		else
			previous.mStateRequired = entries.mRequiredStates[0].mStateRequired;

		for (const auto& entry : entries.mRequiredStates)
		{
			if (entry.mStateRequired != previous.mStateRequired)
			{
				if (entries.mBuffer)
				{
					Hazard hazard;
					switch (entry.mStateRequired)
					{
					case AttachmentType::DataBufferRO:
					case AttachmentType::DataBufferRW:
					case AttachmentType::IndirectBuffer:
						hazard = Hazard::ReadAfterWrite;
						break;

					default:
						hazard = Hazard::WriteAfterRead;
						break;
					}

					SyncPoint src;
					SyncPoint dst;

					if (previous.mTaskIndex != ~0u)
					{
						const auto& [prevType, prevIndex] = mTaskOrder[previous.mTaskIndex];

						if (prevType == TaskType::Compute)
							src = SyncPoint::ComputeShader;
						else
							src = SyncPoint::VertexShader;
					}
					else
						src = SyncPoint::TopOfPipe;

					const auto& [type, index] = mTaskOrder[entry.mTaskIndex];

					if (entry.mStateRequired == AttachmentType::IndirectBuffer)
						dst = SyncPoint::IndirectArgs;
					else if (type == TaskType::Compute)
						dst = SyncPoint::ComputeShader;
					else
						dst = SyncPoint::VertexShader;

					barriers[previous.mTaskIndex + 1]->memoryBarrier(*entries.mBuffer, hazard, src, dst);

				}

				if (entries.mImage)
				{
					Hazard hazard;
					switch (entry.mStateRequired)
					{
					case AttachmentType::Texture1D:
					case AttachmentType::Texture2D:
					case AttachmentType::Texture3D:
						hazard = Hazard::ReadAfterWrite;
						break;

					default:
						hazard = Hazard::WriteAfterRead;
						break;
					}

					const SyncPoint src = getSyncPoint(previous.mStateRequired);
					const SyncPoint dst = getSyncPoint(entry.mStateRequired);

					const ImageLayout layout = (*entries.mImage)->getImageUsage() & ImageUsage::DepthStencil ?
						entry.mStateRequired == AttachmentType::Depth ? ImageLayout::DepthStencil : ImageLayout::DepthStencilRO
						: getImageLayout(entry.mStateRequired);

					barriers[previous.mTaskIndex + 1]->transitionLayout(*entries.mImage, layout, hazard, src, dst);
				}
			}

			previous = entry;
		}
	}

	return barriers;
}


void RenderGraph::reset()
{
	resetBindings();

	mInternalResources.clear();

	mInputResources.clear();
	mOutputResources.clear();

	mFrameBuffersNeedUpdating.clear();
	mDescriptorsNeedUpdating.clear();

	// Clear all jobs
	mGraphicsTasks.clear();
	mComputeTasks.clear();
	mTaskOrder.clear();
	mTaskDependancies.clear();
}


void RenderGraph::resetBindings()
{
	// Clear all bound resources
	mImageViews.clear();
	mBufferViews.clear();
	mSamplers.clear();
	mSRS.clear();

	for (auto& resourceSet : mInputResources)
		resourceSet.clear();

	for (auto& resourceSet : mOutputResources)
		resourceSet.clear();

	// clear non-persistent resources
	mNonPersistentImages.clear();
}


const BufferView& RenderGraph::getVertexBuffer(const uint32_t taskIndex) const
{
    const auto input = std::find_if(mInputResources[taskIndex].begin(), mInputResources[taskIndex].end(), [](const auto& inputAttachment)
    {
        return inputAttachment.mResourcetype == ResourceType::VertexBuffer;
    });
    BELL_ASSERT(input != mInputResources[taskIndex].end(), "Task doesn't bind a vertex buffer")

    return mBufferViews[input->mResourceIndex].second;
}


const BufferView& RenderGraph::getIndexBuffer(const uint32_t taskIndex) const
{
    const auto input = std::find_if(mInputResources[taskIndex].begin(), mInputResources[taskIndex].end(), [](const auto& inputAttachment)
    {
        return inputAttachment.mResourcetype == ResourceType::IndexBuffer;
    });
    BELL_ASSERT(input != mInputResources[taskIndex].end(), "Task doesn't bind a index buffer")

    return mBufferViews[input->mResourceIndex].second;
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


const RenderTask& TaskIterator::operator*() const
{
	auto[taskType, taskIndex] = mGraph.mTaskOrder[mCurrentIndex];
	return mGraph.getTask(taskType, taskIndex);
}
