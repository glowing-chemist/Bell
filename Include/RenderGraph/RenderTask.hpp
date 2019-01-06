#ifndef RENDERTASK_HPP
#define RENDERTASK_HPP

#include <map>
#include <string>
#include <vector>
#include <map>
#include <tuple>

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
	RenderTask() = default;
	virtual ~RenderTask() = default;

    void addInput(const std::string& name, const AttachmentType attachmentType)
        { mOutputAttachments.push_back({name, attachmentType}); }

    void addOutput(const std::string& name, const AttachmentType attachmentType)
        { mInputAttachments.push_back({name, attachmentType}); }

    const std::vector<std::pair<std::string, AttachmentType>>& getInputAttachments() const
        { return mInputAttachments; }

    const std::vector<std::pair<std::string, AttachmentType>>& getOuputAttachments() const
        { return mOutputAttachments; }

	virtual void clearCalls() = 0;


protected:
    std::vector<std::pair<std::string, AttachmentType>> mOutputAttachments;
    std::vector<std::pair<std::string, AttachmentType>> mInputAttachments;
};

#endif
