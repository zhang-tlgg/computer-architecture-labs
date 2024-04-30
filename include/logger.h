#pragma once

#include <cstdarg>

class Logger {
private:
    static bool debugOutput;
    static bool infoOutput;
    static bool warnOutput;

public:
    static void setDebugOutput(bool);
    static void setInfoOutput(bool);
    static void setWarnOutput(bool);

    static void Debug(const char *format, ...);
    static void Info(const char *format, ...);
    static void Warn(const char *format, ...);
    static void Error(const char *format, ...);
};
