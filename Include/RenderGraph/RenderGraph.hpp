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

#include <iterator>
#include <string>
#include <optional>
#include <vector>
#include <tuple>


class DescriptorManager;
class TaskIterator;

enum class BindingIteratorType
{
	Input,
	Output
};

template<BindingIteratorType>
class BindingIterator;

class RenderGraph
{
	friend RenderDevice;
	friend TaskIterator;
	friend BindingIterator<BindingIteratorType::Input>;
	friend BindingIterator<BindingIteratorType::Output>;
public:

    RenderGraph() = default;

    void addTask(const GraphicsTask&);
    void addTask(const ComputeTask&);

    void addDependancy(const std::string& dependancy, const std::string& dependant);

    // compiles the dependancy graph based on slots (assuming resources are finished writing to by their first read from)
    void compileDependancies();

    void bindImage(const std::string&, const ImageView &);
	void bindImageArray(const std::string&, const ImageViewArray&);
	void bindBuffer(const std::string&, const BufferView &);
    void bindSampler(const std::string&, const Sampler&);
	void bindShaderResourceSet(const std::string&, const ShaderResourceSet&);

    // These are special because they are used by every task that has vertex attributes
    void bindVertexBuffer(const Buffer&);
    void bindIndexBuffer(const Buffer&);

    std::optional<Buffer>& getVertexBuffer() { return mVertexBuffer; }
    std::optional<Buffer>& getIndexBuffer() { return mIndexBuffer; }
	const std::optional<Buffer>& getVertexBuffer() const { return mVertexBuffer; }
	const std::optional<Buffer>& getIndexBuffer() const { return mIndexBuffer; }

	RenderTask& getTask(TaskType, uint32_t);
	const RenderTask& getTask(TaskType, uint32_t) const;

	const BufferView& getBoundBuffer(const std::string&) const;
    const ImageView&  getBoundImageView(const std::string&) const;
	const ShaderResourceSet& getBoundShaderResourceSet(const std::string& slot) const;

	void reset();

	uint64_t taskCount() const { return mTaskOrder.size(); }

	TaskIterator taskBegin();
	TaskIterator taskEnd();

	BindingIterator<BindingIteratorType::Input> inputBindingBegin();
	BindingIterator<BindingIteratorType::Input> inputBindingEnd();

	BindingIterator< BindingIteratorType::Output> outputBindingBegin();
	BindingIterator< BindingIteratorType::Output> outputBindingEnd();

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
		SRS
    };
	ImageView&		getImageView(const uint32_t index);
	ImageViewArray& getImageArrayViews(const uint32_t index);
	BufferView&		getBuffer(const uint32_t index);
	Sampler&		getSampler(const uint32_t index);


	struct ResourceBindingInfo
	{
        std::string mName;
		ResourceType mResourcetype;
		uint32_t mResourceIndex;
		uint32_t mResourceBinding;
	};

	void generateNonPersistentImages(RenderDevice*);

private:

    void reorderTasks();
	// Selecets the best task to execuet next based on some heuristics.
	uint32_t selectNextTask(const std::vector<uint8_t>& dependancies, const TaskType) const;

    void mergeTasks();
    bool areSupersets(const RenderTask&, const RenderTask&);
    void mergeTasks(RenderTask&, RenderTask&);

    void bindResource(const std::string&, const uint32_t, const ResourceType);

    std::vector<GraphicsTask> mGraphicsTasks;
	std::vector<ComputeTask>  mComputeTasks;

    std::optional<Buffer> mVertexBuffer;
    std::optional<Buffer> mIndexBuffer;

	// The index in to the coresponding task array
    std::vector<std::pair<TaskType, uint32_t>> mTaskOrder;
	std::vector<bool> mDescriptorsNeedUpdating;
	std::vector<bool> mFrameBuffersNeedUpdating;

	// Dependancy, Dependant
    std::vector<std::pair<uint32_t, uint32_t>> mTaskDependancies;

    std::vector<std::vector<ResourceBindingInfo>> mInputResources;
    std::vector<std::vector<ResourceBindingInfo>> mOutputResources;

    std::vector<std::pair<std::string, ImageView>> mImageViews;
	std::vector<std::pair<std::string, ImageViewArray>> mImageViewArrays;
	std::vector<std::pair<std::string, BufferView>> mBufferViews;
    std::vector<std::pair<std::string, Sampler>> mSamplers;
	std::vector<std::pair<std::string, ShaderResourceSet>> mSRS;

	// Store the images created per frame.
	struct NonPersistentImage
	{
		NonPersistentImage(const Image& i, const ImageView& v) :
			mImage{i},
			mImageView{v} {}

		Image mImage;
		ImageView mImageView;
	};
	std::vector<NonPersistentImage> mNonPersistentImages;
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
