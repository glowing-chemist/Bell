#ifndef BELL_LOGGING_HPP
#define BELL_LOGGING_HPP

#include <stdio.h>

// This file contains logging/assert macros that will print out in debug builds.

// BELL_LOG
#ifndef NDEBUG

#define BELL_LOG_ARGS(msg_format, ...) printf(msg_format "\n", __VA_ARGS__);
#define BELL_LOG(msg)		  printf(msg "\n");

#else

#define BELL_LOG_ARGS(msg_format, ...)
#define BELL_LOG(msg)

#endif


// BELL_ASSRT
#ifndef NDEBUG

#define BELL_ASSERT(condition, msg) if(!condition) printf("ASSERT FAILED: " ## msg ## #condition);

#else

#define BELL_ASSERT

#endif


#endif
