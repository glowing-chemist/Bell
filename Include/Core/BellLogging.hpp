#ifndef BELL_LOGGING_HPP
#define BELL_LOGGING_HPP

#include <stdio.h>

#ifndef NDEBUG
#define ENABLE_LOGGING 1
#else
#define ENABLE_LOGGING 0
#endif

// This file contains logging/assert macros that will print out in debug builds.
#define BELL_ENABLE_LOGGING ENABLE_LOGGING

// BELL_LOG
#if BELL_ENABLE_LOGGING

#define BELL_LOG_ARGS(msg_format, ...) printf(msg_format "\n", __VA_ARGS__); \
                                        fflush(stdout);
#define BELL_LOG(msg)		  printf(msg "\n");

#else

#define BELL_LOG_ARGS(msg_format, ...)
#define BELL_LOG(msg)

#endif

// BELL_TRAP
#if BELL_ENABLE_LOGGING

#ifdef _MSC_VER

#define BELL_TRAP __debugbreak()

#else
#define BELL_TRAP __asm("int3")

#endif

#else

#define BELL_TRAP

#endif


// BELL_ASSRT
#if BELL_ENABLE_LOGGING

#define BELL_ASSERT(condition, msg) if(!(condition)) { printf(msg #condition "\n"); BELL_TRAP; }

#else

#define BELL_ASSERT(condition, msg)

#endif

#endif
