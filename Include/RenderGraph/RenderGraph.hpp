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
#include "Core/PerFrameResource.hpp"

#include <iterator>
#include <string>
#include <optional>
#include <vector>
#include <tuple>


class DescriptorManager;
class TaskIterator;
class VulkanRenderDevice;

enum class BindingIteratorType
{
	Input,
	Output
};

template<BindingIteratorType>
class BindingIterator;

using TaskID = int32_t;

class RenderGraph
{
	friend VulkanRenderDevice;
	friend TaskIterator;
	friend BindingIterator<BindingIteratorType::Input>;
	friend BindingIterator<BindingIteratorType::Output>;
public:

    RenderGraph() = default;

	TaskID addTask(const GraphicsTask&);
	TaskID addTask(const ComputeTask&);
	RenderTask& getTask(const TaskID);

	void addDependancy(const RenderTask& dependancy, const RenderTask& dependant);
    void addDependancy(const std::string& dependancy, const std::string& dependant);

	void compile(RenderDevice* dev);

    // Bind persistent resourcers.
    void bindImage(const std::string&, const ImageView &);
	void bindImageArray(const std::string&, const ImageViewArray&);
	void bindBuffer(const std::string&, const BufferView &);
    void bindVertexBuffer(const std::string&, const BufferView &);
    void bindIndexBuffer(const std::string&, const BufferView &);
    void bindSampler(const std::string&, const Sampler&);
	void bindShaderResourceSet(const std::string&, const ShaderResourceSet&);

	void bindInternalResources();

    RenderTask& getTask(const uint32_t);
    const RenderTask& getTask(const uint32_t) const;
	RenderTask& getTask(TaskType, uint32_t);
	const RenderTask& getTask(TaskType, uint32_t) const;

	const BufferView& getBoundBuffer(const std::string&) const;
    const ImageView&  getBoundImageView(const std::string&) const;
	const ShaderResourceSet& getBoundShaderResourceSet(const std::string& slot) const;

	std::vector<BarrierRecorder> generateBarriers(RenderDevice*);

	void reset();
	void resetBindings();

	uint64_t taskCount() const { return mTaskOrder.size(); }
    uint64_t graphicsTaskCount() const { return mGraphicsTasks.size(); }
    uint64_t computeTaskCount() const { return mComputeTasks.size(); }

    const BufferView &getVertexBuffer(const uint32_t taskIndex) const;
    const BufferView& getIndexBuffer(const uint32_t taskIndex) const;

	TaskIterator taskBegin();
	TaskIterator taskEnd();

    BindingIterator<BindingIteratorType::Input> inputBindingBegin() const;
    BindingIterator<BindingIteratorType::Input> inputBindingEnd() const;

    BindingIterator< BindingIteratorType::Output> outputBindingBegin() const;
    BindingIterator< BindingIteratorType::Output> outputBindingEnd() const;

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
		SRS
    };
	ImageView&		getImageView(const uint32_t index);
	ImageViewArray& getImageArrayViews(const uint32_t index);
	BufferView&		getBuffer(const uint32_t index);
	Sampler&		getSampler(const uint32_t index);

    const ImageView&		getImageView(const uint32_t index) const;
    const ImageViewArray& getImageArrayViews(const uint32_t index) const;
    const BufferView&		getBuffer(const uint32_t index) const;
    const Sampler&		getSampler(const uint32_t index) const;


	struct ResourceBindingInfo
	{
        std::string mName;
		ResourceType mResourcetype;
		uint32_t mResourceIndex;
		uint32_t mResourceBinding;
	};

private:

	// compiles the dependancy graph based on slots (assuming resources are finished writing to by their first read from)
	void compileDependancies();
	void generateInternalResources(RenderDevice*);
    void compileBarrierInfo();

	void reorderTasks();

    void sortResourceBindings();

	// Selecets the best task to execuet next based on some heuristics.
	uint32_t selectNextTask(const std::vector<uint8_t>& dependancies, const TaskType) const;

    void bindResource(const std::string&, const uint32_t, const ResourceType);

	void createInternalResource(RenderDevice*, const std::string& name, const Format, const ImageUsage, const SizeClass);

    std::vector<GraphicsTask> mGraphicsTasks;
	std::vector<ComputeTask>  mComputeTasks;

	// The index in to the coresponding task array
    std::vector<std::pair<TaskType, uint32_t>> mTaskOrder;
	std::vector<bool> mDescriptorsNeedUpdating;
	std::vector<bool> mFrameBuffersNeedUpdating;

    // Hazard tracking info.
    struct HazardTrackingInfo
    {
        uint32_t mResourceBinding;
        Hazard mHazard;
        SyncPoint mSrc;
        SyncPoint mDst;
        AttachmentType mNeededtype;
        bool mImagetransition;
        bool mInputResource;
    };
    std::vector<std::vector<HazardTrackingInfo>> mHazardInfo;

	// Dependancy, Dependant
    std::vector<std::pair<uint32_t, uint32_t>> mTaskDependancies;

    std::vector<std::vector<ResourceBindingInfo>> mInputResources;
    std::vector<std::vector<ResourceBindingInfo>> mOutputResources;

    std::vector<std::pair<std::string, ImageView>> mImageViews;
	std::vector<std::pair<std::string, ImageViewArray>> mImageViewArrays;
	std::vector<std::pair<std::string, BufferView>> mBufferViews;
    std::vector<std::pair<std::string, Sampler>> mSamplers;
	std::vector<std::pair<std::string, ShaderResourceSet>> mSRS;

	struct InternalResourceEntry
	{
		InternalResourceEntry(const std::string slot, PerFrameResource<Image>& image, PerFrameResource<ImageView>& view) :
			mSlot{ slot }, mResource{ image }, mResourceView{ view } {}

		std::string mSlot;
		PerFrameResource<Image> mResource;
		PerFrameResource<ImageView> mResourceView;
	};
	std::vector<InternalResourceEntry> mInternalResources;
};




class TaskIterator : public std::iterator<std::forward_iterator_tag,
	RenderTask>
{
	uint64_t mCurrentIndex;
	const RenderGraph& mGraph;

public:

	TaskIterator(RenderGraph& graph, uint64_t startingIndex = 0) : mCurrentIndex{ startingIndex }, mGraph{ graph } {}

	const RenderTask& operator*() const;
    TaskIterator& operator++() { ++mCurrentIndex; return *this; }
	bool operator==(const TaskIterator& rhs) { return mCurrentIndex == rhs.mCurrentIndex; }
	bool operator!=(const TaskIterator& rhs) { return !(*this == rhs); }
};


template<BindingIteratorType>
class BindingIterator : public std::iterator<std::forward_iterator_tag,
	std::vector<RenderGraph::ResourceBindingInfo>>
{
    const std::vector<std::vector<RenderGraph::ResourceBindingInfo>>& mBindings;
	uint64_t mCurrentIndex;
	const RenderGraph& mGraph;

public:

    BindingIterator(const std::vector<std::vector<RenderGraph::ResourceBindingInfo>>& bindings, const RenderGraph& graph, uint64_t startingIndex = 0) : mBindings{ bindings }, mCurrentIndex{ startingIndex }, mGraph{ graph } {}

    const std::vector<RenderGraph::ResourceBindingInfo>& operator*() const  { return mBindings[mCurrentIndex];  }
	BindingIterator& operator++() { ++mCurrentIndex; return *this; }
    BindingIterator  operator+(const uint32_t i) {mCurrentIndex += i; return *this; }
	bool operator==(const BindingIterator& rhs) { return mCurrentIndex == rhs.mCurrentIndex; }
	bool operator!=(const BindingIterator& rhs) { return !(*this == rhs); }
};

#endif
