#include "logger.h"

#include <cstdio>

bool Logger::debugOutput = false;
bool Logger::infoOutput = true;
bool Logger::warnOutput = true;

/**
 * @brief 是否开启调试输出
 *
 * @param flag
 */
void Logger::setDebugOutput(bool flag) { debugOutput = flag; }

/**
 * @brief 是否开启信息输出
 *
 * @param flag
 */
void Logger::setInfoOutput(bool flag) { infoOutput = flag; }

/**
 * @brief 是否开启警告输出
 *
 * @param flag
 */
void Logger::setWarnOutput(bool flag) { warnOutput = flag; }

/**
 * @brief 用于调试输出，格式与printf一样
 *
 * @param format
 * @param ...
 */
void Logger::Debug(const char *format, ...) {
    if (debugOutput) {
        fprintf(stderr, "[  DEBUG  ] ");
        va_list args;
        va_start(args, format);
        [[maybe_unused]] auto ret = vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

/**
 * @brief 用于执行信息输出，格式与printf一样
 *
 * @param format
 * @param ...
 */
void Logger::Info(const char *format, ...) {
    if (infoOutput) {
        fprintf(stderr, "[  INFO   ] ");
        va_list args;
        va_start(args, format);
        [[maybe_unused]] auto ret = vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

/**
 * @brief 用于警告输出，格式与printf一样
 *
 * @param format
 * @param ...
 */
void Logger::Warn(const char *format, ...) {
    if (warnOutput) {
        fprintf(stderr, "[ WARNING ] ");
        va_list args;
        va_start(args, format);
        [[maybe_unused]] auto ret = vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

/**
 * @brief 用于错误输出，格式与printf一样
 *
 * @param format
 * @param ...
 */
void Logger::Error(const char *format, ...) {
    fprintf(stderr, "[  ERROR  ] ");
    va_list args;
    va_start(args, format);
    [[maybe_unused]] auto ret = vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
}
