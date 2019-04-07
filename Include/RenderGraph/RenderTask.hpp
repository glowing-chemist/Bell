#ifndef RENDERTASK_HPP
#define RENDERTASK_HPP

#include <map>
#include <string>
#include <vector>
#include <map>
#include <tuple>

#include <vulkan/vulkan.hpp>

class RenderGraph;

enum class AttachmentType
{
    Texture1D,
    Texture2D,
    Texture3D,
	RenderTarget1D,
	RenderTarget2D,
	RenderTarget3D,
    SwapChain,
	Depth,
    UniformBuffer,
    DataBuffer,
    IndirectBuffer,
    PushConstants
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

    virtual void addOutput(const std::string& name, const AttachmentType attachmentType)
    {
       mOutputAttachments.push_back({name, attachmentType});
    }


    virtual void recordCommands(vk::CommandBuffer, const RenderGraph&) const = 0;


    const std::vector<std::pair<std::string, AttachmentType>>& getInputAttachments() const
        { return mInputAttachments; }

    const std::vector<std::pair<std::string, AttachmentType>>& getOuputAttachments() const
        { return mOutputAttachments; }

	virtual void clearCalls() = 0;

	virtual TaskType taskType() const = 0;

    const std::string& getName() const
        { return mName; }


protected:

    std::string mName;

    std::vector<std::pair<std::string, AttachmentType>> mOutputAttachments;
    std::vector<std::pair<std::string, AttachmentType>> mInputAttachments;
};

#endif
