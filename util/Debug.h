#ifndef DEBUG_H
#define DEBUG_H

// This #define ensures that debugging functions are used
// remove it if not desired
#ifndef DEBUG
#define DEBUG
#endif

#include "util/Logger.h"

#ifdef DEBUG
#define pi_assert(exp)				if(!(exp)) LOG_ERROR(Logger::Misc, "Assertion-Warning")
#define pi_assert2(exp, msg)        if(!(exp)) LOG_ERROR(Logger::Misc, "Assertion-Warning %s", msg)
#else
#define pi_assert(exp)
#define pi_assert2(exp, msg)
#endif

#endif // DEBUG_H
