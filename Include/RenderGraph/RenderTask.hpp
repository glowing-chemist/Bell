#ifndef RENDERTASK_HPP
#define RENDERTASK_HPP

#include <map>
#include <string>
#include <vector>
#include <tuple>
#include <functional>

#include "Engine/PassTypes.hpp"


class RenderGraph;
class Executor;
class RenderEngine;
class RenderTask;
class MeshInstance;

using CommandCallbackFunc = std::function<void(const RenderGraph& graph, const uint32_t taskIndex, Executor*, RenderEngine*, const std::vector<const MeshInstance*>&)>;

enum class LoadOp
{
    Preserve,
	Nothing,
    Clear_White,
    Clear_Black,
    Clear_ColourBlack_AlphaWhite,
    Clear_Float_Max
};

enum class StoreOp
{
    Store,
    Discard
};

enum class SizeClass
{
    QuadrupleSwapChain,
    DoubleSwapchain,
	Swapchain,
	HalfSwapchain,
	QuarterSwapchain,
	Custom
};

enum class TaskType
{
	Graphics,
	Compute,
    AsyncCompute,
	All
};


// This class describes any task that can be used for rendering
// from graphics render passes to async compute
class RenderTask 
{
public:
    RenderTask(const std::string& name) : mName{name} {}
	virtual ~RenderTask() = default;

    virtual void addInput(const char* name, const AttachmentType attachmentType, const size_t arraySize = 0)
    {
       mInputAttachments.push_back({name, attachmentType, arraySize});
    }

    // Loadop has no effect on ComputeTasks
    virtual void addOutput( const char* name, 
                            const AttachmentType attachmentType, 
                            const Format format, 
                            const LoadOp loadOp = LoadOp::Preserve, 
                            const StoreOp storeOp = StoreOp::Store)
    {
       mOutputAttachments.push_back({name, attachmentType, format, SizeClass::Custom, loadOp, storeOp, ImageUsage::ColourAttachment});
    }

    virtual void addManagedOutput(const char* name,
        const AttachmentType attachmentType,
        const Format format,
        const SizeClass size,
        const LoadOp loadOp = LoadOp::Preserve,
        const StoreOp storeOp = StoreOp::Store,
        const ImageUsage usage = ImageUsage::ColourAttachment | ImageUsage::Sampled)
    {
        mOutputAttachments.push_back({ name, attachmentType, format, size, loadOp, storeOp, usage });
    }

    struct OutputAttachmentInfo
    {
        const char*		mName;
		AttachmentType	mType;
		Format			mFormat;
		SizeClass		mSize;
		LoadOp			mLoadOp;
        StoreOp         mStoreOp;
        ImageUsage      mUsage;
    };

	struct InputAttachmentInfo
	{
        const char* mName;
		AttachmentType mType;
        size_t mArraySize;
	};

    const std::vector<InputAttachmentInfo>& getInputAttachments() const
        { return mInputAttachments; }

    const std::vector<OutputAttachmentInfo>& getOuputAttachments() const
        { return mOutputAttachments; }

	virtual TaskType taskType() const = 0;

    const std::string& getName() const
        { return mName; }

    void setRecordCommandsCallback(const CommandCallbackFunc& callback)
    {
        mRecordCommandsCallback = callback;
    }

    void executeRecordCommandsCallback(const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes) const
    {
        mRecordCommandsCallback(graph, taskIndex, exec, eng, meshes);
    }

protected:

    std::string mName;

    std::vector<OutputAttachmentInfo> mOutputAttachments;
    std::vector<InputAttachmentInfo> mInputAttachments;

    CommandCallbackFunc mRecordCommandsCallback;
};


#endif
