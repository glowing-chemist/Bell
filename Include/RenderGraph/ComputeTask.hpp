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

// This class describes a compute task (sync)
class ComputeTask : public RenderTask 
{
public:
    ComputeTask(const char* name) : RenderTask{ name } {}

    void addOutput(const char* name, const AttachmentType attachmentType, const Format, const SizeClass = SizeClass::Custom, const LoadOp = LoadOp::Preserve, const StoreOp = StoreOp::Store) override final
    {
        // All outputs needs to be part of the descriptor set for compute pipelies
        // as compuite shaders writes don't go to the framebuffer.
        mInputAttachments.push_back({name, attachmentType, 0});
    }

	TaskType taskType() const override final { return TaskType::Compute; }
};


// This class describes a compute task (async)
class AsyncComputeTask : public RenderTask
{
public:
    AsyncComputeTask(const char* name) : RenderTask{ name } {}

    void addOutput(const char* name, const AttachmentType attachmentType, const Format, const SizeClass = SizeClass::Custom, const LoadOp = LoadOp::Preserve, const StoreOp = StoreOp::Store) override final
    {
        // All outputs needs to be part of the descriptor set for compute pipelies
        // as compuite shaders writes don't go to the framebuffer.
        mInputAttachments.push_back({ name, attachmentType, 0 });
    }

    TaskType taskType() const override final { return TaskType::AsyncCompute; }
};


#endif
