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
class Engine;
struct MeshInstance;

using CommandCallbackFunc = std::function<void(Executor*, Engine*, const std::vector<const MeshInstance*>&)>;

enum class LoadOp
{
    Preserve,
	Nothing,
    Clear_White,
    Clear_Black
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
	All
};


// This class describes any task that can be used for rendering
// from graphics render passes to async compute
class RenderTask 
{
public:
    RenderTask(const std::string& name) : mName{name} {}
	virtual ~RenderTask() = default;

    virtual void addInput(const std::string& name, const AttachmentType attachmentType, const size_t arraySize = 0)
    {
       mInputAttachments.push_back({name, attachmentType, arraySize});
    }

    // Loadop has no effect on ComputeTasks
	virtual void addOutput(const std::string& name, const AttachmentType attachmentType, const Format format, const SizeClass size = SizeClass::Custom, const LoadOp op = LoadOp::Preserve)
    {
	   mOutputAttachments.push_back({name, attachmentType, format, size, op});
    }

    struct OutputAttachmentInfo
    {
		std::string		mName;
		AttachmentType	mType;
		Format			mFormat;
		SizeClass		mSize;
		LoadOp			mLoadOp;
    };

	struct InputAttachmentInfo
	{
		std::string mName;
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

    void executeRecordCommandsCallback(Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes) const
    {
        mRecordCommandsCallback(exec, eng, meshes);
    }

protected:

    std::string mName;

    std::vector<OutputAttachmentInfo> mOutputAttachments;
    std::vector<InputAttachmentInfo> mInputAttachments;

    CommandCallbackFunc mRecordCommandsCallback;
};


inline bool operator<(const RenderTask::OutputAttachmentInfo& lhs, const RenderTask::OutputAttachmentInfo& rhs)
{
    return lhs.mName < rhs.mName && lhs.mType < rhs.mType && lhs.mLoadOp < rhs.mLoadOp;
}

#endif
