#ifndef LOGGER_H
#define LOGGER_H

#include <string>

class Logger
{
public:
    enum Facility
    {
        Misc = 0,
        I2C,
        BT,
        Script,
        UI,
        N_FACILITIES // must be last
    };

    static void debugf(Facility facility, const char* file, const char* function, int line, const char *format, ...);
    static void warnf(Facility facility, const char* file, const char* function, int line, const char *format, ...);
    static void errorf(Facility facility, const char* file, const char* function, int line, const char *format, ...);

    static std::string FacilityToString(Facility facility);
    static Facility StringToFacility(std::string str);
private:
    Logger();
    ~Logger();
    static Logger* getInst();

    void print(const char* msg);
    bool isFacilityLogged(Facility facility);

    bool m_bLogFacility[N_FACILITIES];
};

#define LOG_DEBUG(facility, ...)           Logger::debugf(facility, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_WARN(facility, ...)            Logger::warnf(facility, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(facility, ...)           Logger::errorf(facility, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#endif // LOGGER_H
