
#include "util/Logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <strings.h>

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

Logger::Facility
Logger::StringToFacility(std::string str)
{
    const char* cstr = str.c_str();

    if( strcasecmp("misc", cstr) == 0)
        return Misc;
    else if(strcasecmp("i2c", cstr) == 0)
        return I2C;
    else if(strcasecmp("bt", cstr) == 0)
        return BT;
    else if(strcasecmp("script", cstr) == 0)
        return Script;
    else if(strcasecmp("ui", cstr) == 0)
        return UI;
    else
        return N_FACILITIES;
}

std::string
Logger::FacilityToString(Facility facility)
{
    switch(facility)
    {
    case Misc:
        return "Misc";
    case I2C:
        return "I2C";
    case BT:
        return "BT";
    case Script:
        return "Script";
    case UI:
        return "UI";
    default:
        return "INVALID";
    }
}
