#ifndef RENDERTASK_HPP
#define RENDERTASK_HPP

#include <map>
#include <string>
#include <vector>
#include <map>
#include <tuple>

enum class AttatchmentType
{
    Texture1D,
    Texture2D,
    Texture3D,
	RenderTarget1D,
	RenderTarget2D,
	RenderTarget3D,
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

	void addInput(const std::string& name, std::tuple<uint32_t, AttatchmentType> bindingInfo)
		{ mAttatchments[name] = bindingInfo; }

	void addOutput(const std::string& name, std::tuple<uint32_t, AttatchmentType> bindingInfo)
		{ mInputs[name] = bindingInfo; }

	virtual void clearCalls() = 0;


protected:
	std::map<std::string, std::tuple<uint32_t, AttatchmentType>> mAttatchments;
	std::map<std::string, std::tuple<uint32_t, AttatchmentType>> mInputs;
};

#endif
