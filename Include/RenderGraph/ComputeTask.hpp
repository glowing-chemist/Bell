#ifndef COMPUTETASK_HPP
#define COMPUTETASK_HPP

#include "RenderTask.hpp"
#include "Core/Shader.hpp"
#include "Core/Pipeline.hpp"

#include <string>
#include <vector>
#include <cstdint>

enum class DispatchType
{
	Standard,
	Indirect
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

    void addDispatch(const uint32_t x, const uint32_t y, const uint32_t z) { mComputeCalls.push_back({DispatchType::Standard, x, y, z, ""}); }
    void addIndirectDispatch(const uint32_t x, const uint32_t y, const uint32_t z) { mComputeCalls.push_back({DispatchType::Indirect, x, y, z, ""}); }

    void addOutput(const std::string& name, const AttachmentType attachmentType, const Format, const SizeClass = SizeClass::Custom, const LoadOp = LoadOp::Preserve) override final
    {
        // All outputs needs to be part of the descriptor set for compute pipelies
        // as compuite shaders writes don't go to the framebuffer.
        mInputAttachments.push_back({name, attachmentType});
    }

	void mergeWith(const RenderTask&) override final;

    // Needs to take the graph to be able to lookup indirect buffers tha are bound to the graph.
    void recordCommands(vk::CommandBuffer, const RenderGraph&, const vulkanResources&) const override final;

	void clearCalls() override final { mComputeCalls.clear(); }

    uint32_t recordedCommandCount() const override final { return static_cast<uint32_t>(mComputeCalls.size()); }

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
