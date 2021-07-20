#ifndef UTILITY_TASKS_HPP
#define UTILITY_TASKS_HPP

#include <unordered_map>

#include "RenderGraph/RenderGraph.hpp"
#include "Scene.h"

TaskID addDeferredUpsampleTaskR8(const char* name, const char* input, const char* output, const uint2 outputSize, RenderEngine*, RenderGraph&);

TaskID addDeferredUpsampleTaskRGBA8(const char* name, const char* input, const char* output, const uint2 outputSize, RenderEngine*, RenderGraph&);

TaskID addBlurXTaskR8(const char* name, const char* input, const char* output, const uint2 outputSize, RenderEngine*, RenderGraph&);

TaskID addBlurYTaskR8(const char* name, const char* input, const char* output, const uint2 outputSize, RenderEngine*, RenderGraph&);

void compileShadeFlagsPipelines(std::unordered_map<uint64_t, uint64_t>& pipelineMap,
                                const std::string& vertexPath,
                                const std::string& fragmentPath,
                                RenderEngine*,
                                const RenderGraph&,
                                const TaskID id);

void compileSkinnedPipelineVariants(uint64_t*,
                                    const std::string& vertexPath,
                                    const std::string& fragmentPath,
                                    RenderEngine*,
                                    const RenderGraph&,
                                    const TaskID id);

#endif
