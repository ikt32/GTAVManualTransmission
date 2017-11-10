#pragma once
#include <string>
#include <vector>

enum LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

class Logger {

public:
    Logger();
    void SetFile(const std::string &fileName);
    void SetMinLevel(LogLevel level);
    void Clear();
    void Write(LogLevel level, const std::string& text);
    void Write(LogLevel level, const char *fmt, ...);

private:
    std::string file = "";
    std::string levelText(LogLevel level);
    LogLevel minLevel = INFO;
    const std::vector<std::string> levelStrings{
        "DEBUG", 
        "INFO ",
        "WARN ",
        "ERROR",
        "FATAL",
    };
};

extern Logger logger;
