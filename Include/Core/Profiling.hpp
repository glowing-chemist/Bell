#ifndef BELL_PROFILING
#define BELL_PROFILING

#define PROFILE 0

#define USE_OPTIK PROFILE

#include "optick.h"

#ifdef VULKAN
#include "Core/Vulkan/VulkanExecutor.hpp"
#else
#include "Core/DX_12/DX_12Executor.hpp"
#endif

#if PROFILE

#define PROFILER_START_FRAME(n) OPTICK_FRAME(n)

#define PROFILER_END_FRAME() OPTICK_FRAME_FLIP()

#define PROFILER_EVENT() OPTICK_EVENT()

#define PROFILER_THREAD(n) OPTICK_THREAD(n)

#define PROFILER_TAG(e, n) OPTICK_TAG(n, e)

#define PROFILER_INIT_VULKAN(dev, physDev, queues, queueFamilies, queueCount) OPTICK_GPU_INIT_VULKAN(dev, physDev, queues, queueFamilies, queueCount, nullptr)

#define PROFILER_GPU_FLIP(swapChain) OPTICK_GPU_FLIP(swapChain)

#ifdef VULKAN

#define PROFILER_GPU_TASK(exec) OPTICK_GPU_CONTEXT(static_cast<VulkanExecutor*>(exec)->getCommandBuffer())

#else

#define PROFILER_GPU_TASK(exec) OPTICK_GPU_CONTEXT(static_cast<DX12_Executor*>(exec)->getCommandList())

#endif

#define PROFILER_GPU_EVENT(n) OPTICK_GPU_EVENT(n)

#else

#define PROFILER_START_FRAME(n)

#define PROFILER_EVENT()

#define PROFILER_THREAD(n)

#define PROFILER_TAG(e, n)

#define PROFILER_INIT_VULKAN(dev, physDev, queues, queueFamilies, queueCount)

#define PROFILER_GPU_FLIP(swapChain)

#define PROFILER_GPU_TASK(exec)

#define PROFILER_GPU_EVENT(n)

#endif

#endif
