#ifndef DEBUG_H
#define DEBUG_H

// This #define ensures that debugging functions are used
// remove it if not desired
#ifndef DEBUG
#define DEBUG
#endif

//#define DEBUG_I2C

void pi_error2(const char* str, ...);
void pi_message(const char* str, ...);

#define pi_error(msg)				pi_error2("Error: %s\nFile: %s\nLine: %i\n", msg, __FILE__, __LINE__)

#ifdef DEBUG
#define pi_assert(exp)				if(!(exp)) pi_error2("Assertion-Warning:\nFile: %s\nLine: %i\n", __FILE__, __LINE__)
#define pi_assert2(exp, msg)        if(!(exp)) pi_error2("Assertion-Warning: %s\nFile: %s\nLine: %i\n", msg, __FILE__, __LINE__)
#define pi_warn(msg)				pi_message("Warning: %s\r\nFile: %s\r\nLine: %i\n", msg, __FILE__, __LINE__)
#else
#define pi_assert(exp)
#define pi_assert2(exp, msg)
#define pi_warn(msg)				pi_message("Warning: %s\r\nFile: %s\r\nLine: %i\n", msg, __FILE__, __LINE__)
#endif


#ifdef DEBUG_I2C
#define I2C_warn(msg)               pi_warn(msg)
#else
#define I2C_warn(msg)
#endif

#endif // DEBUG_H
