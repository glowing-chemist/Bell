#ifndef COMPUTETASK_HPP
#define COMPUTETASK_HPP

#include "RenderTask.hpp"
#include "Core/Shader.hpp"

#include <string>
#include <vector>
#include <cstdint>

enum class DispatchType
{
	Standard,
	Indirect
};

struct ComputePipelineDescription
{
	Shader mComputeShader;
};

// needed in order to use unordered_map
namespace std
{
	template<>
	struct hash<ComputePipelineDescription>
	{
		size_t operator()(const ComputePipelineDescription&) const noexcept;
	};
}


// This class describes a compute task (sync and async)
class ComputeTask : public RenderTask 
{
public:
    ComputeTask(const std::string& name, ComputePipelineDescription desc) : RenderTask{ name }, mPipelineDescription{ desc } {}

    ComputePipelineDescription getPipelineDescription() const
        { return mPipelineDescription; }

    void addOutput(const std::string& name, const AttachmentType attachmentType, const Format, const SizeClass = SizeClass::Custom, const LoadOp = LoadOp::Preserve) override final
    {
        // All outputs needs to be part of the descriptor set for compute pipelies
        // as compuite shaders writes don't go to the framebuffer.
        mInputAttachments.push_back({name, attachmentType, 0});
    }

	TaskType taskType() const override final { return TaskType::Compute; }

    friend bool operator==(const ComputeTask& lhs, const ComputeTask& rhs);

private:

	struct thunkedCompute
	{
		DispatchType mDispatchType;
		uint32_t x, y, z;
        std::string mIndirectBuffer;
	};
	std::vector<thunkedCompute> mComputeCalls;

	ComputePipelineDescription mPipelineDescription;

};


bool operator==(const ComputePipelineDescription& lhs, const ComputePipelineDescription& rhs);

#endif
