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

    void addInput(const std::string& name, std::pair<uint32_t, AttachmentType> bindingInfo)
        { mOutputAttachments[name] = bindingInfo; }

    void addOutput(const std::string& name, std::pair<uint32_t, AttachmentType> bindingInfo)
        { mInputAttachments[name] = bindingInfo; }

    const std::map<std::string, std::pair<uint32_t, AttachmentType>>& getInputAttachments() const
        { return mInputAttachments; }

    const std::map<std::string, std::pair<uint32_t, AttachmentType>>& getOuputAttachments() const
        { return mOutputAttachments; }

	virtual void clearCalls() = 0;


protected:
    std::map<std::string, std::pair<uint32_t, AttachmentType>> mOutputAttachments;
    std::map<std::string, std::pair<uint32_t, AttachmentType>> mInputAttachments;
};

#endif
