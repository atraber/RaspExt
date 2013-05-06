
#include "util/Logger.h"

#include <stdio.h>
#include <stdarg.h>

Logger::Logger()
{

}

Logger::~Logger()
{

}

Logger*
Logger::getInst()
{
    static Logger L;
    return &L;
}

void
Logger::debugf(Facility facility, const char* file, const char* function, int line, const char *format, ...)
{
    char str[255];
    unsigned int len;

    va_list args;

    va_start(args, format);

    len = vsnprintf(str, 255, format, args);

    va_end(args);

    snprintf(str + len, 255 - len, "\nFile: %s\nFunction: %s\nLine: %i\n", file, function, line);

    Logger::getInst()->print(str);
}

void
Logger::warnf(Facility facility, const char* file, const char* function, int line, const char *format, ...)
{
    char str[255];
    unsigned int len;

    va_list args;

    va_start(args, format);

    len = vsnprintf(str, 255, format, args);

    va_end(args);

    snprintf(str + len, 255 - len, "\nFile: %s\nFunction: %s\nLine: %i\n", file, function, line);

    Logger::getInst()->print(str);
}

void
Logger::errorf(Facility facility, const char* file, const char* function, int line, const char *format, ...)
{
    char str[255];
    unsigned int len;

    va_list args;

    va_start(args, format);

    len = vsnprintf(str, 255, format, args);

    va_end(args);

    snprintf(str + len, 255 - len, "\nFile: %s\nFunction: %s\nLine: %i\n", file, function, line);

    Logger::getInst()->print(str);
}

void
Logger::print(const char *msg)
{
    printf(msg);
}