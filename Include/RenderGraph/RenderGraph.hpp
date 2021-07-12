#ifndef RENDERGRAPH_HPP
#define RENDERGRAPH_HPP

#include "GraphicsTask.hpp"
#include "ComputeTask.hpp"

#include "Core/Image.hpp"
#include "Core/ImageView.hpp"
#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"
#include "Core/Sampler.hpp"
#include "Core/ShaderResourceSet.hpp"
#include "Core/AccelerationStructures.hpp"

#include <iterator>
#include <string>
#include <optional>
#include <vector>
#include <tuple>


class DescriptorManager;
class TaskIterator;
class VulkanRenderDevice;

enum BindingFlags : uint32_t
{
    ManualBarriers = 1
};

struct TaskID
{
    uint32_t mTaskIndex;
    TaskType mTaskType;
};

class RenderGraph
{
    friend VulkanRenderDevice;
    friend TaskIterator;
public:

    RenderGraph() = default;

    TaskID addTask(const GraphicsTask&);
    TaskID addTask(const ComputeTask&);
    TaskID addTask(const AsyncComputeTask&);

    void addDependancy(const RenderTask& dependancy, const RenderTask& dependant);
    void addDependancy(const std::string& dependancy, const std::string& dependant);

    void compile(RenderDevice* dev);

    // Bind persistent resourcers.
    void bindImage(const char* name, const ImageView&, const uint32_t flags = 0);
    void bindImageArray(const char* name, const ImageViewArray&, const uint32_t flags = 0);
    void bindBuffer(const char* name, const BufferView&, const uint32_t flags = 0);
    void bindBufferArray(const char*, const BufferViewArray&, const uint32_t flags = 0);
    void bindSampler(const char* name, const Sampler&);
    void bindShaderResourceSet(const char* name, const ShaderResourceSet&);
    void bindAccelerationStructure(const char* name, const TopLevelAccelerationStructure&);
    bool isResourceSlotBound(const char* name) const;

    //get Task by index.
    RenderTask& getTask(const uint32_t);
    const RenderTask& getTask(const uint32_t) const;

    // Get Task by ID.
    RenderTask& getTask(const TaskID);
    const RenderTask& getTask(const TaskID) const;
    const ShaderResourceSet& getShaderResourceSet(const char*) const;

    std::vector<BarrierRecorder> generateBarriers(RenderDevice *dev);

	void reset();
	void resetBindings();

	uint64_t taskCount() const { return mTaskOrder.size(); }
    uint64_t graphicsTaskCount() const { return mGraphicsTasks.size(); }
    uint64_t computeTaskCount() const { return mComputeTasks.size(); }

    TaskIterator taskBegin() const;
    TaskIterator taskEnd() const;

	const std::vector<bool>& getDescriptorsNeedUpdating() const
	{
		return mDescriptorsNeedUpdating; 
	}


	const std::vector<bool>& getFrameBuffersNeedUpdating() const
	{
		return mFrameBuffersNeedUpdating;
	}

    // resources tracking
    enum class ResourceType
    {
        Image,
		ImageArray,
        Sampler,
        Buffer,
        VertexBuffer,
        IndexBuffer,
		SRS,
		AccelerationStructure
    };
    ImageView&		getImageView(const char* name);
    ImageViewArray& getImageArrayViews(const char* mName);
    BufferView&		getBuffer(const char* mName);
    BufferViewArray&		getBufferArrayViews(const char* name);
    Sampler&		getSampler(const char* mName);
    TopLevelAccelerationStructure& getAccelerationStructure(const char*);

    const ImageView&		getImageView(const char* name) const;
    const ImageViewArray& getImageArrayViews(const char* name) const;
    const BufferView&		getBuffer(const char* name) const;
    const BufferViewArray&		getBufferArrayViews(const char* name) const;
    const Sampler&		getSampler(const char* name) const;
    const TopLevelAccelerationStructure& getAccelerationStructure(const char*) const;

    struct ResourceInfo
    {
        AttachmentType mType;
        uint32_t mTaskIndex;
    };

private:

    // compiles the dependancy graph based on slots (assuming resources are finished writing to by their first read from)
    void compileDependancies();
    void generateInternalResources(RenderDevice*);

    void reorderTasks();
    void bindInternalResources();

    RenderTask& getTask(TaskType, uint32_t);
    const RenderTask& getTask(TaskType, uint32_t) const;

    // Selecets the best task to execuet next based on some heuristics.
    uint32_t selectNextTask(const std::vector<uint8_t>& dependancies, const TaskType) const;

    void bindResource(const char* name, const uint32_t flags);

    void createInternalResource(RenderDevice*, const char *name, const Format, const ImageUsage, const SizeClass);

    void verifyDependencies();

    std::vector<GraphicsTask> mGraphicsTasks;
    std::vector<ComputeTask>  mComputeTasks;
    std::vector<AsyncComputeTask> mAsyncComputeTasks;

    // The index in to the coresponding task array
    std::vector<std::pair<TaskType, uint32_t>> mTaskOrder;
    std::vector<bool> mDescriptorsNeedUpdating;
    std::vector<bool> mFrameBuffersNeedUpdating;

    // Dependancy, Dependant
    std::vector<std::pair<uint32_t, uint32_t>> mTaskDependancies;

    struct ResourceUsageEntries
    {
        uint32_t mFlags;
        std::vector<ResourceInfo> mUsages;
    };
    std::unordered_map<const char*, ResourceUsageEntries> mResourceInfo;

    std::unordered_map<const char*, ImageView> mImageViews;
    std::unordered_map<const char*, ImageViewArray> mImageViewArrays;
    std::unordered_map<const char*, BufferView> mBufferViews;
    std::unordered_map<const char*, BufferViewArray> mBufferViewArrays;
    std::unordered_map<const char*, Sampler> mSamplers;
    std::unordered_map<const char*, ShaderResourceSet> mSRS;
    std::unordered_map<const char*, TopLevelAccelerationStructure> mAccelerationStructures;

	struct InternalResourceEntry
	{
        InternalResourceEntry(const char* slot, Image& image, ImageView& view) :
			mSlot{ slot }, mResource{ image }, mResourceView{ view } {}

        const char* mSlot;
        Image mResource;
        ImageView mResourceView;
	};
	std::vector<InternalResourceEntry> mInternalResources;
};




class TaskIterator : public std::iterator<std::forward_iterator_tag,
	RenderTask>
{
	uint64_t mCurrentIndex;
	const RenderGraph& mGraph;

public:

    TaskIterator(const RenderGraph& graph, uint64_t startingIndex = 0) : mCurrentIndex{ startingIndex }, mGraph{ graph } {}

	const RenderTask& operator*() const;
    TaskIterator& operator++() { ++mCurrentIndex; return *this; }
	bool operator==(const TaskIterator& rhs) { return mCurrentIndex == rhs.mCurrentIndex; }
	bool operator!=(const TaskIterator& rhs) { return !(*this == rhs); }
};

#endif
