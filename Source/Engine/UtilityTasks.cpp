#include "Engine/UtilityTasks.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

#include "Core/Executor.hpp"


TaskID addDeferredUpsampleTaskR8(const char* name, const char* input, const char* output, const uint2 outputSize, Engine* eng, RenderGraph& graph)
{
    ComputeTask task{ name };
    task.addInput(input, AttachmentType::Texture2D);
    task.addInput(output, AttachmentType::Image2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kPointSampler, AttachmentType::Sampler);
    task.addInput(kLinearDepth, AttachmentType::Texture2D);
    task.addInput(kGBufferNormals, AttachmentType::Texture2D);

    task.setRecordCommandsCallback(
                [=](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    Shader upsampleShader = eng->getShader("./Shaders/BilaterialUpsample.comp");
                    const RenderTask& task = graph.getTask(taskIndex);
                    exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, upsampleShader);

                    const float threadGroupWidth = outputSize.x;
                    const float threadGroupHeight = outputSize.y;

                    exec->dispatch(std::ceil(threadGroupWidth / 16.0f), std::ceil(threadGroupHeight / 16.0f), 1.0f);
                }
    );

    return graph.addTask(task);
}


TaskID addBlurXTaskR8(const char* name, const char* input, const char* output, const uint2 outputSize, Engine* eng, RenderGraph& graph)
{
    ComputeTask blurXTask{name};
    blurXTask.addInput(input, AttachmentType::Texture2D);
    blurXTask.addInput(output, AttachmentType::Image2D);
    blurXTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    blurXTask.setRecordCommandsCallback(
        [=](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            Shader blurXShader = eng->getShader("./Shaders/blurXR8.comp");
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, blurXShader);

            exec->dispatch(std::ceil(outputSize.x / 256.0f), outputSize.y, 1.0f);
        }
    );

    return graph.addTask(blurXTask);
}


TaskID addBlurYTaskR8(const char* name, const char* input, const char* output, const uint2 outputSize, Engine* eng, RenderGraph& graph)
{
    ComputeTask blurYTask{ name };
    blurYTask.addInput(input, AttachmentType::Texture2D);
    blurYTask.addInput(output, AttachmentType::Image2D);
    blurYTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    blurYTask.setRecordCommandsCallback(
        [=](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
        {
            Shader blurYShader = eng->getShader("./Shaders/blurYR8.comp");
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, blurYShader);

            exec->dispatch(outputSize.x, std::ceil(outputSize.y / 256.0f), 1.0f);
        }
    );

    return graph.addTask(blurYTask);
}
