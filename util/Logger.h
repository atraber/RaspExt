#ifndef LOGGER_H
#define LOGGER_H

class Logger
{
public:
    enum Facility
    {
        Misc = 0,
        I2C,
        BT,
        Script,
        UI
    };

    static void debug(Facility facility, const char* msg, const char* file, const char* function, int line);
    static void debugf(Facility facility, const char* file, const char* function, int line, const char *format, ...);
private:
    Logger();
    ~Logger();
    static Logger* getInst();

    void print(const char* msg);
};

#define LOG_DEBUG(facility, msg)                    Logger::debug(facility, msg, __FILE__, __FUNCTION__, __LINE__)
#define LOG_DEBUGF(facility, ...)           Logger::debugf(facility, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#endif // LOGGER_H
