
#include "RenderGraph/RenderGraph.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/BellLogging.hpp"
#include "Core/ConversionUtils.hpp"
#include "Core/Profiling.hpp"

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
    mFrameBuffersNeedUpdating.push_back(false);
    mDescriptorsNeedUpdating.push_back(false);

    return { taskIndex, TaskType::Graphics };
}


TaskID RenderGraph::addTask(const ComputeTask& task)
{
	const uint32_t taskIndex = static_cast<uint32_t>(mComputeTasks.size());
	mComputeTasks.push_back(task);

    mTaskOrder.push_back({TaskType::Compute, taskIndex});

    // Also add a vulkan resources and inputs/outputs for each task, zero initialised
    mFrameBuffersNeedUpdating.push_back(false);
    mDescriptorsNeedUpdating.push_back(false);

    return { taskIndex, TaskType::Compute };
}


TaskID RenderGraph::addTask(const AsyncComputeTask& task)
{
    const uint32_t taskIndex = static_cast<uint32_t>(mAsyncComputeTasks.size());
    mAsyncComputeTasks.push_back(task);

    mTaskOrder.push_back({ TaskType::AsyncCompute, taskIndex });

    // Also add a vulkan resources and inputs/outputs for each task, zero initialised
    mFrameBuffersNeedUpdating.push_back(false);
    mDescriptorsNeedUpdating.push_back(false);

    return { taskIndex, TaskType::AsyncCompute };
}


RenderTask& RenderGraph::getTask(const TaskID id)
{
    switch (id.mTaskType)
    {
    case TaskType::Graphics:
        return mGraphicsTasks[id.mTaskIndex];

    case TaskType::Compute:
        return mComputeTasks[id.mTaskIndex];

    case TaskType::AsyncCompute:
        return mAsyncComputeTasks[id.mTaskIndex];
    }

    BELL_TRAP;
}


const RenderTask& RenderGraph::getTask(const TaskID id) const
{
    switch (id.mTaskType)
    {
    case TaskType::Graphics:
        return mGraphicsTasks[id.mTaskIndex];

    case TaskType::Compute:
        return mComputeTasks[id.mTaskIndex];

    case TaskType::AsyncCompute:
        return mAsyncComputeTasks[id.mTaskIndex];
    }

    BELL_TRAP;
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
    PROFILER_EVENT();

	compileDependancies();
	generateInternalResources(dev);
	reorderTasks();
    bindInternalResources();
}


void RenderGraph::compileDependancies()
{
    PROFILER_EVENT();

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
                                (((outResources[outerIndex].mType == AttachmentType::Image1D ||
                                 outResources[outerIndex].mType == AttachmentType::Image2D ||
                                 outResources[outerIndex].mType == AttachmentType::Image3D ||
                                 outResources[outerIndex].mType == AttachmentType::TransferDestination) &&
                                  (innerResources[innerIndex].mType == AttachmentType::Texture1D ||
                                   innerResources[innerIndex].mType == AttachmentType::Texture2D ||
                                   innerResources[innerIndex].mType == AttachmentType::Texture3D ||
                                   innerResources[innerIndex].mType == AttachmentType::CubeMap ||
                                   innerResources[innerIndex].mType == AttachmentType::TransferSource)
                                  ) ||
                                 // If the outer task writes to the buffer and the inner reads we need
                                 // to emit a dependancy to enforce writing before reading.
                                 ((outResources[outerIndex].mType == AttachmentType::DataBufferWO ||
                                  outResources[outerIndex].mType == AttachmentType::DataBufferRW) &&
                                  (innerResources[innerIndex].mType == AttachmentType::DataBufferRO ||
                                  innerResources[innerIndex].mType == AttachmentType::DataBufferRW ||
                                   innerResources[innerIndex].mType == AttachmentType::VertexBuffer ||
                                   innerResources[innerIndex].mType == AttachmentType::IndexBuffer ||
                                   innerResources[innerIndex].mType == AttachmentType::CommandPredicationBuffer )) ||
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

            // generate dependancies for framebuffer -> framebuffer e.g depth -> depth
			{
				const std::vector<RenderTask::OutputAttachmentInfo>& outResources = outerTask.getOuputAttachments();
				const std::vector<RenderTask::OutputAttachmentInfo>& inResources = innerTask.getOuputAttachments();
                bool dependancyFound = false;

				for(size_t outerIndex = 0; outerIndex < outResources.size(); ++outerIndex)
				{
					for(size_t innerIndex = 0; innerIndex < inResources.size(); ++innerIndex)
					{
                        if(outResources[outerIndex].mName == inResources[innerIndex].mName &&
                                (((outResources[outerIndex].mType == AttachmentType::Depth && inResources[innerIndex].mType == AttachmentType::Depth) ||
                                (outResources[outerIndex].mType == AttachmentType::RenderTarget2D && inResources[innerIndex].mType == AttachmentType::RenderTarget2D)) &&
                                (outResources[outerIndex].mSize != SizeClass::Custom && inResources[innerIndex].mSize == SizeClass::Custom)))
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

#ifndef NDEBUG
    verifyDependencies();
#endif
}


void RenderGraph::bindResource(const char* name, const uint32_t flags)
{
    PROFILER_EVENT();

    uint32_t taskOrderIndex = 0;
	for(const auto& [taskType, taskIndex] : mTaskOrder)
    {
        const RenderTask& task = getTask(taskType, taskIndex);

        uint32_t inputAttachmentIndex = 0;
        for(const auto& input : task.getInputAttachments())
        {
            if(input.mName == name)
            {
                mResourceInfo[name].mFlags = flags;
                mResourceInfo[name].mUsages.push_back({input.mType, taskOrderIndex});
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
                mResourceInfo[name].mFlags = flags;
                mResourceInfo[name].mUsages.push_back({input.mType, taskOrderIndex});
                mFrameBuffersNeedUpdating[taskOrderIndex] = true;
                break;
            }
            ++outputAttachmentIndex;
        }

        ++taskOrderIndex;
    }
}


void RenderGraph::bindImage(const char *name, const ImageView &image, const uint32_t flags)
{
    mImageViews.insert_or_assign(name, image);
    mResourceInfo[name].mUsages.clear();

    bindResource(name, flags);
}


void RenderGraph::bindImageArray(const char *name, const ImageViewArray& imageArray, const uint32_t flags)
{
    mImageViewArrays.insert_or_assign(name, imageArray);
    mResourceInfo[name].mUsages.clear();

    bindResource(name, flags);
}


void RenderGraph::bindBuffer(const char *name , const BufferView& buffer, const uint32_t flags)
{
    mBufferViews.insert_or_assign(name, buffer);
    mResourceInfo[name].mUsages.clear();

    bindResource(name, flags);
}


void RenderGraph::bindVertexBuffer(const char *name, const BufferView& buffer, const uint32_t flags)
{
    mBufferViews.insert_or_assign(name, buffer);
    mResourceInfo[name].mUsages.clear();

    bindResource(name, flags);
}


void RenderGraph::bindIndexBuffer(const char *name, const BufferView& buffer, const uint32_t flags)
{
    mBufferViews.insert_or_assign(name, buffer);
    mResourceInfo[name].mUsages.clear();

    bindResource(name, flags);
}


void RenderGraph::bindSampler(const char *name, const Sampler& sampler)
{
    mSamplers.insert_or_assign(name, sampler);
    mResourceInfo[name].mUsages.clear();

    bindResource(name, 0);
}


void RenderGraph::bindShaderResourceSet(const char *name, const ShaderResourceSet& set)
{
    mSRS.insert_or_assign(name, set);
    mResourceInfo[name].mUsages.clear();
}


bool RenderGraph::isResourceSlotBound(const char* name) const
{
    return  mImageViews.find(name) != mImageViews.end() ||
            mBufferViews.find(name) != mBufferViews.end() ||
            mImageViewArrays.find(name) != mImageViewArrays.end() ||
            mSamplers.find(name) != mSamplers.end() ||
            mSRS.find(name) != mSRS.end();
}


void RenderGraph::bindInternalResources()
{
	for (const auto& resourceEntry : mInternalResources)
	{
        bindImage(resourceEntry.mSlot, resourceEntry.mResourceView);
	}
}


void RenderGraph::createInternalResource(RenderDevice* dev, const char* name, const Format format, const ImageUsage usage, const SizeClass size)
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

    Image generatedImage(dev, format, usage,
		width, height, 1, 1, 1, 1, name);

    ImageView view{ generatedImage, usage & ImageUsage::DepthStencil ?
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
                                 output.mUsage,
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
    std::vector<bool> newFrameBuffersNeedUpdating{};
    std::vector<bool> newDescriptorsNeedUpdating{};
	TaskType previousTaskType = TaskType::Compute;

    newTaskOrder.reserve(mTaskOrder.size());

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
        newFrameBuffersNeedUpdating.push_back(mFrameBuffersNeedUpdating[taskIndexToAdd]);
        newDescriptorsNeedUpdating.push_back(mDescriptorsNeedUpdating[taskIndexToAdd]);
		
		mTaskDependancies.erase(std::remove_if(mTaskDependancies.begin(), mTaskDependancies.end(), [=](const auto& dependancy)
			{
				return dependancy.first == taskIndexToAdd;
			}), mTaskDependancies.end());
	}

	mTaskOrder.swap(newTaskOrder);
    mFrameBuffersNeedUpdating.swap(newFrameBuffersNeedUpdating);
    mDescriptorsNeedUpdating.swap(newDescriptorsNeedUpdating);

#ifndef NDEBUG // Enable to print out task submission order.

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


RenderTask& RenderGraph::getTask(const uint32_t index)
{
    BELL_ASSERT(index < mTaskOrder.size(), "invalid task index")
    auto [type, i] = mTaskOrder[index];

    return getTask(type, i);
}


const RenderTask& RenderGraph::getTask(const uint32_t index) const
{
    BELL_ASSERT(index < mTaskOrder.size(), "invalid task index")
    auto [type, i] = mTaskOrder[index];

    return getTask(type, i);
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
            case TaskType::AsyncCompute:
                return mAsyncComputeTasks[taskIndex];
            case TaskType::All:
                return mGraphicsTasks[taskIndex];
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
            case TaskType::AsyncCompute:
                return mAsyncComputeTasks[taskIndex];
            case TaskType::All:
                return mGraphicsTasks[taskIndex];
        }
    }();

    return task;
}


Sampler& RenderGraph::getSampler(const char* name)
{
    BELL_ASSERT(mSamplers.find(name) != mSamplers.end(), " Attempting to fetch non sampler resource")

    return mSamplers.find(name)->second;
}


ImageView& RenderGraph::getImageView(const char *name)
{
    BELL_ASSERT(mImageViews.find(name) != mImageViews.end(), " Attempting to fetch non imageView resource")

    return mImageViews.find(name)->second;
}


ImageViewArray& RenderGraph::getImageArrayViews(const char* name)
{
    BELL_ASSERT(mImageViewArrays.find(name) != mImageViewArrays.end(), "Attempting to fetch non imageViewArray resource")

    return mImageViewArrays.find(name)->second;
}


BufferView& RenderGraph::getBuffer(const char* name)
{
    BELL_ASSERT(mBufferViews.find(name) != mBufferViews.end(), " Attempting to fetch non buffer resource")

    return mBufferViews.find(name)->second;
}


const Sampler& RenderGraph::getSampler(const char* name) const
{
    BELL_ASSERT(mSamplers.find(name) != mSamplers.end(), " Attempting to fetch non sampler resource")

    return mSamplers.find(name)->second;
}


const ImageView& RenderGraph::getImageView(const char* name) const
{
    BELL_ASSERT(mImageViews.find(name) != mImageViews.end(), " Attempting to fetch non imageView resource")

    return mImageViews.find(name)->second;
}


const ImageViewArray& RenderGraph::getImageArrayViews(const char* name) const
{
    BELL_ASSERT(mImageViewArrays.find(name) != mImageViewArrays.end(), "Attempting to fetch non imageViewArray resource")

    return mImageViewArrays.find(name)->second;
}


const BufferView& RenderGraph::getBuffer(const char* name) const
{
    BELL_ASSERT(mBufferViews.find(name) != mBufferViews.end(), " Attempting to fetch non buffer resource")

    return mBufferViews.find(name)->second;
}


const ShaderResourceSet& RenderGraph::getShaderResourceSet(const char* name) const
{
    BELL_ASSERT(mSRS.find(name) != mSRS.end(), "Attempting to fetch non imageViewArray resource")

    return mSRS.find(name)->second;
}


std::vector<BarrierRecorder> RenderGraph::generateBarriers(RenderDevice* dev)
{    
    PROFILER_EVENT();

	std::vector<BarrierRecorder> barriers{};
    for(uint32_t i = 0; i < mTaskOrder.size(); ++i)
        barriers.emplace_back(dev);

    auto isImage = [](const AttachmentType type) -> bool
    {
        switch(type)
        {
        case AttachmentType::RenderTarget1D:
        case AttachmentType::RenderTarget2D:
        case AttachmentType::Image1D:
        case AttachmentType::Image2D:
        case AttachmentType::Image3D:
        case AttachmentType::Texture1D:
        case AttachmentType::Texture2D:
        case AttachmentType::Texture3D:
        case AttachmentType::CubeMap:
        case AttachmentType::Depth:
        case AttachmentType::TransferDestination:
        case AttachmentType::TransferSource:
            return true;

        default:
            return false;
        }
    };

    auto isBuffer = [](const AttachmentType type) -> bool
    {
        switch(type)
        {
        case AttachmentType::IndexBuffer:
        case AttachmentType::VertexBuffer:
        case AttachmentType::UniformBuffer:
        case AttachmentType::DataBufferRO:
        case AttachmentType::DataBufferWO:
        case AttachmentType::DataBufferRW:
        case AttachmentType::IndirectBuffer:
            return true;

        default:
            return false;
        }
    };

    for (const auto& [name, usageInfo] : mResourceInfo)
	{
        const auto& entries = usageInfo.mUsages;

        // Print all resource transitions.
#if 0
        BELL_LOG_ARGS("\nResource Name: %s", name);
        for (const auto& entry : entries)
        {
            BELL_LOG_ARGS("Used in task %s as a %s", getTask(entry.mTaskIndex).getName().c_str(), getAttachmentName(entry.mType));
        }
        fflush(stdout);
#endif

        if(entries.empty() || (usageInfo.mFlags & BindingFlags::ManualBarriers)) // Resource isn't used by any tasks or is a SRS.
            continue;

		// intended overflow.
        ResourceInfo previous{AttachmentType::PushConstants, ~0u};

        if (isImage(entries[0].mType))
		{
            const ImageView& view = getImageView(name);
            const auto layout = view->getImageLayout(view->getBaseLevel(), view->getBaseMip());
            previous.mType = getAttachmentType(layout);
		}
		else
            previous.mType = entries[0].mType;

        for (const auto& entry : entries)
		{
            if (entry.mType != previous.mType)
			{
                if (isBuffer(entry.mType))
				{
					Hazard hazard;
                    switch (entry.mType)
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

                    if (entry.mType == AttachmentType::IndirectBuffer)
						dst = SyncPoint::IndirectArgs;
					else if (type == TaskType::Compute)
						dst = SyncPoint::ComputeShader;
					else
						dst = SyncPoint::VertexShader;

                    barriers[previous.mTaskIndex + 1]->memoryBarrier(getBuffer(name), hazard, src, dst);

				}

                if (isImage(entry.mType))
				{
					Hazard hazard;
                    switch (entry.mType)
					{
					case AttachmentType::Texture1D:
					case AttachmentType::Texture2D:
					case AttachmentType::Texture3D:
                    case AttachmentType::CubeMap:
                    case AttachmentType::TransferSource:
						hazard = Hazard::ReadAfterWrite;
						break;

					default:
						hazard = Hazard::WriteAfterRead;
						break;
					}

                    const SyncPoint src = getSyncPoint(previous.mType);
                    const SyncPoint dst = getSyncPoint(entry.mType);

                    ImageView& view = getImageView(name);
                    const ImageLayout layout = view->getImageUsage() & ImageUsage::DepthStencil ?
                        entry.mType == AttachmentType::Depth ? ImageLayout::DepthStencil : ImageLayout::DepthStencilRO
                        : getImageLayout(entry.mType);

                    barriers[previous.mTaskIndex + 1]->transitionLayout(view, layout, hazard, src, dst);
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

    mResourceInfo.clear();

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
    mImageViewArrays.clear();
	mBufferViews.clear();
	mSamplers.clear();
	mSRS.clear();
}


void RenderGraph::verifyDependencies()
{
    // TODO make more reobust, only checks direct circular links currently.
    for(const auto& [dependantOuter, dependancyOuter] : mTaskDependancies)
    {
        for(const auto& [dependantInner, dependancyInner] : mTaskDependancies)
        {
            if(dependantOuter == dependancyInner && dependancyOuter == dependantInner)
            {
                const RenderTask& task1 = getTask(dependantOuter);
                const RenderTask& task2 = getTask(dependancyOuter);
                BELL_LOG_ARGS("Circular dependancy between %s and %s", task1.getName().c_str(), task2.getName().c_str());
                fflush(stdout);
                BELL_TRAP;
            }
        }
    }
}


TaskIterator RenderGraph::taskBegin() const
{
	return TaskIterator{*this};
}


TaskIterator RenderGraph::taskEnd() const
{
	return TaskIterator{*this, mTaskOrder.size()};
}


const RenderTask& TaskIterator::operator*() const
{
	auto[taskType, taskIndex] = mGraph.mTaskOrder[mCurrentIndex];
	return mGraph.getTask(taskType, taskIndex);
}
