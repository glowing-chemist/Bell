#include "Engine/UtilityTasks.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

#include "Core/Executor.hpp"


TaskID addDeferredUpsampleTaskR8(const char* input, const char* output, const uint2 outputSize, Engine* eng, RenderGraph& graph)
{
    ComputePipelineDescription pipeline{eng->getShader("./Shaders/BilaterialUpsample.comp")};
    ComputeTask task{"Upsample", pipeline};
    task.addInput(input, AttachmentType::Texture2D);
    task.addInput(output, AttachmentType::Image2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kPointSampler, AttachmentType::Sampler);
    task.addInput(kLinearDepth, AttachmentType::Texture2D);
    task.addInput(kGBufferNormals, AttachmentType::Texture2D);

    task.setRecordCommandsCallback(
                [=](Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
                {
                    const float threadGroupWidth = outputSize.x;
                    const float threadGroupHeight = outputSize.y;

                    exec->dispatch(std::ceil(threadGroupWidth / 16.0f), std::ceil(threadGroupHeight / 16.0f), 1.0f);
                }
    );

    return graph.addTask(task);
}
