#ifndef RENDERTASK_HPP
#define RENDERTASK_HPP

#include <map>
#include <string>
#include <vector>
#include <map>
#include <tuple>

#include "Engine/PassTypes.hpp"

#include <vulkan/vulkan.hpp>

class RenderGraph;

enum class LoadOp
{
    Preserve,
    Clear_White,
    Clear_Black
};

enum class TaskType
{
	Graphics,
	Compute
};


// This class describes any task that can be used for rendering
// from graphics render passes to async compute
class RenderTask 
{
public:
    RenderTask(const std::string& name) : mName{name} {}
	virtual ~RenderTask() = default;

    virtual void addInput(const std::string& name, const AttachmentType attachmentType)
    {
       mInputAttachments.push_back({name, attachmentType});
    }

    // Loadop has no effect on ComputeTasks
	virtual void addOutput(const std::string& name, const AttachmentType attachmentType, const Format format, const LoadOp op = LoadOp::Preserve)
    {
	   mOutputAttachments.push_back({name, attachmentType, format, op});
    }


    virtual void recordCommands(vk::CommandBuffer, const RenderGraph&) const = 0;

    struct OutputAttachmentInfo
    {
        std::string mName;
        AttachmentType mType;
		Format			mFormat;
        LoadOp         mLoadOp;
    };

    const std::vector<std::pair<std::string, AttachmentType>>& getInputAttachments() const
        { return mInputAttachments; }

    const std::vector<OutputAttachmentInfo>& getOuputAttachments() const
        { return mOutputAttachments; }

	virtual void clearCalls() = 0;

	virtual TaskType taskType() const = 0;

    const std::string& getName() const
        { return mName; }


protected:

    std::string mName;

    std::vector<OutputAttachmentInfo> mOutputAttachments;
    std::vector<std::pair<std::string, AttachmentType>> mInputAttachments;
};


inline bool operator<(const RenderTask::OutputAttachmentInfo& lhs, const RenderTask::OutputAttachmentInfo& rhs)
{
    return lhs.mName < rhs.mName && lhs.mType < rhs.mType && lhs.mLoadOp < rhs.mLoadOp;
}

#endif
