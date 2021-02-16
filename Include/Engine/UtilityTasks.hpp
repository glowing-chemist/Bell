#ifndef UTILITY_TASKS_HPP
#define UTILITY_TASKS_HPP


#include "RenderGraph/RenderGraph.hpp"

TaskID addDeferredUpsampleTaskR8(const char* name, const char* input, const char* output, const uint2 outputSize, RenderEngine*, RenderGraph&);

TaskID addDeferredUpsampleTaskRGBA8(const char* name, const char* input, const char* output, const uint2 outputSize, RenderEngine*, RenderGraph&);

TaskID addBlurXTaskR8(const char* name, const char* input, const char* output, const uint2 outputSize, RenderEngine*, RenderGraph&);

TaskID addBlurYTaskR8(const char* name, const char* input, const char* output, const uint2 outputSize, RenderEngine*, RenderGraph&);

#endif
