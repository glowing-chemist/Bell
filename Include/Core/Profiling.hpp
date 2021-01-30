#ifndef BELL_PROFILING
#define BELL_PROFILING

#define PROFILE 1

#if PROFILE
#define USE_OPTIK 1
#endif

#include "optick.h"

#ifdef PROFILE

#define PROFILER_TICK(n) OPTICK_FRAME(n)

#define PROFILER_EVENT() OPTIK_EVENT()

#define PROFILER_THREAD(n) OPTIK_THREAD(n)

#define PROFILER_TAG(e, n) OPTIK_TAG(n, e)

#define PROFILER_INIT_VULKAN(dev, physDev, queues, queueFamilies, queueCount) OPTICK_GPU_INIT_VULKAN(dev, physDev, queues, queueFamilies, queueCount, nullptr)

#define PROFILER_GPU_FLIP(swapChain) OPTICK_GPU_FLIP(swapChain)

#define PROFILER_GPU_SCOPE(cmdList) OPTICK_GPU_CONTEXT(cmdList)

#define PROFILER_GPU_EVENT(n) OPTICK_GPU_EVENT(n)

#else

#define PROFILER_TICK(n) (void)n

#define PROFILER_EVENT()

#define PROFILER_THREAD(n)

#define PROFILER_TAG(e, n)

#define PROFILER_INIT_VULKAN(dev, physDev, queues, queueFamilies, queueCount)

PROFILER_GPU_FLIP(swapChain)

#define PROFILER_GPU_SCOPE(cmdList)

#define PROFILER_GPU_EVENT(n)

#endif

#endif
