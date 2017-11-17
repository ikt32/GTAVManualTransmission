#include "Logger.hpp"

#include <iomanip>
#include <Windows.h>
#include <fstream>

Logger::Logger() {
}

void Logger::SetFile(const std::string &fileName) {
    file = fileName;
}

void Logger::SetMinLevel(LogLevel level) {
    minLevel = level;
}

void Logger::Clear() const {
    std::ofstream logFile(file, std::ofstream::out | std::ofstream::trunc);
}

void Logger::Write(LogLevel level, const std::string& text) const {
#ifndef _DEBUG
    if (level < minLevel) return;
#endif
    std::ofstream logFile(file, std::ios_base::out | std::ios_base::app);
    SYSTEMTIME currTimeLog;
    GetLocalTime(&currTimeLog);
    logFile << "[" <<
        std::setw(2) << std::setfill('0') << currTimeLog.wHour << ":" <<
        std::setw(2) << std::setfill('0') << currTimeLog.wMinute << ":" <<
        std::setw(2) << std::setfill('0') << currTimeLog.wSecond << "." <<
        std::setw(3) << std::setfill('0') << currTimeLog.wMilliseconds << "] " <<
        "[" << levelText(level) << "] " <<
        text << "\n";
}

void Logger::Write(LogLevel level, const char *fmt, ...) const {
    const int size = 1024;
    char buff[size];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buff, size, fmt, args);
    va_end(args);
    Write(level, std::string(buff));
}

std::string Logger::levelText(LogLevel level) const {
    return levelStrings[level];
}

// Everything's gonna use this instance.
Logger logger;
