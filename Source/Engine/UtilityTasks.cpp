#include "Engine/UtilityTasks.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

#include "Core/Executor.hpp"


TaskID addDeferredUpsampleTaskR8(const char* name, const char* input, const char* output, const uint2 outputSize, RenderEngine* eng, RenderGraph& graph)
{
    ComputeTask task{ name };
    task.addInput(input, AttachmentType::Texture2D);
    task.addInput(output, AttachmentType::Image2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kPointSampler, AttachmentType::Sampler);
    task.addInput(kLinearDepth, AttachmentType::Texture2D);
    task.addInput(kGBufferNormals, AttachmentType::Texture2D);

    task.setRecordCommandsCallback(
                [=](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>&)
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


TaskID addDeferredUpsampleTaskRGBA8(const char* name, const char* input, const char* output, const uint2 outputSize, RenderEngine* eng, RenderGraph& graph)
{
    ComputeTask task{ name };
    task.addInput(input, AttachmentType::Texture2D);
    task.addInput(output, AttachmentType::Image2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kPointSampler, AttachmentType::Sampler);
    task.addInput(kLinearDepth, AttachmentType::Texture2D);
    task.addInput(kGBufferNormals, AttachmentType::Texture2D);

    task.setRecordCommandsCallback(
                [=](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>&)
                {
                    Shader upsampleShader = eng->getShader("./Shaders/BilaterialUpsampleRGBA8.comp");
                    const RenderTask& task = graph.getTask(taskIndex);
                    exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, upsampleShader);

                    const float threadGroupWidth = outputSize.x;
                    const float threadGroupHeight = outputSize.y;

                    exec->dispatch(std::ceil(threadGroupWidth / 16.0f), std::ceil(threadGroupHeight / 16.0f), 1.0f);
                }
    );

    return graph.addTask(task);
}


TaskID addBlurXTaskR8(const char* name, const char* input, const char* output, const uint2 outputSize, RenderEngine* eng, RenderGraph& graph)
{
    ComputeTask blurXTask{name};
    blurXTask.addInput(input, AttachmentType::Texture2D);
    blurXTask.addInput(output, AttachmentType::Image2D);
    blurXTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    blurXTask.setRecordCommandsCallback(
        [=](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>&)
        {
            Shader blurXShader = eng->getShader("./Shaders/blurXR8.comp");
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, blurXShader);

            exec->dispatch(std::ceil(outputSize.x / 256.0f), outputSize.y, 1.0f);
        }
    );

    return graph.addTask(blurXTask);
}


TaskID addBlurYTaskR8(const char* name, const char* input, const char* output, const uint2 outputSize, RenderEngine* eng, RenderGraph& graph)
{
    ComputeTask blurYTask{ name };
    blurYTask.addInput(input, AttachmentType::Texture2D);
    blurYTask.addInput(output, AttachmentType::Image2D);
    blurYTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    blurYTask.setRecordCommandsCallback(
        [=](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine*, const std::vector<const MeshInstance*>&)
        {
            Shader blurYShader = eng->getShader("./Shaders/blurYR8.comp");
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, blurYShader);

            exec->dispatch(outputSize.x, std::ceil(outputSize.y / 256.0f), 1.0f);
        }
    );

    return graph.addTask(blurYTask);
}


void compileShadeFlagsPipelines(std::unordered_map<uint64_t, uint64_t>& pipelineMap,
                                const std::string& vertexPath,
                                const std::string& fragmentPath,
                                RenderEngine* engine,
                                const RenderGraph& graph,
                                const TaskID id)
{
    const Scene* scene = engine->getScene();
    RenderDevice* device = engine->getDevice();
    const std::vector<Scene::Material>& materials = scene->getMaterialDescriptions();
    // compile pipelines for all material variants.
    const RenderTask& task = graph.getTask(id);
    for(const auto& material : materials)
    {
        for(uint8_t skinning = 0; skinning < 2; ++skinning)
        {
            const uint64_t shadeflags = material.mMaterialTypes | (skinning ? kShade_Skinning : 0u);
            if (pipelineMap.find(shadeflags) == pipelineMap.end())
            {
                ShaderDefine fragmentShadeDefines(L"SHADE_FLAGS", shadeflags);
                Shader fragmentShader = engine->getShader(fragmentPath, fragmentShadeDefines);
                ShaderDefine vertexShadeDefine(L"SHADE_FLAGS", (skinning ? kShade_Skinning : 0u));
                Shader vertexShader = engine->getShader(vertexPath, vertexShadeDefine);

                const auto& graphicsTask = static_cast<const GraphicsTask &>(task);
                const PipelineHandle pipeline = device->compileGraphicsPipeline(graphicsTask,
                                                                                graph,
                                                                                graphicsTask.getVertexAttributes() | (skinning ? (VertexAttributes::BoneWeights | VertexAttributes::BoneIndices) : 0u),
                                                                                vertexShader, nullptr,
                                                                                nullptr, nullptr, fragmentShader);

                pipelineMap.insert({shadeflags, pipeline});
            }
        }
    }
}

void compileSkinnedPipelineVariants(PipelineHandle* array,
                                    const std::string& vertexPath,
                                    const std::string& fragmentPath,
                                    RenderEngine* engine,
                                    const RenderGraph& graph,
                                    const TaskID id)
{
    RenderDevice* device = engine->getDevice();
    const RenderTask& task = graph.getTask(id);
    Shader fragmentShader = engine->getShader(fragmentPath);
    for(uint8_t skinning = 0; skinning < 2; ++skinning)
    {
        ShaderDefine vertexShadeDefine(L"SHADE_FLAGS", (skinning ? kShade_Skinning : 0u));
        Shader vertexShader = engine->getShader(vertexPath, vertexShadeDefine);

        const auto& graphicsTask = static_cast<const GraphicsTask &>(task);
        const PipelineHandle pipeline = device->compileGraphicsPipeline(graphicsTask,
                                                                        graph,
                                                                        graphicsTask.getVertexAttributes() |
                                                                        (skinning ? (VertexAttributes::BoneWeights |
                                                                                         VertexAttributes::BoneIndices)
                                                                                      : 0),
                                                                        vertexShader, nullptr,
                                                                        nullptr, nullptr, fragmentShader);

        array[skinning] = pipeline;
    }
}
