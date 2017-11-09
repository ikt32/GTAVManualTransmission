#pragma once
#include <string>
#include <fstream>

class Logger {
public:
    Logger();
    void SetFile(const std::string &fileName);
    void Clear();
    void Write(const std::string& text);
    int Writef(char *fmt, ...);

private:
    std::string file = "";
    std::ofstream logFile;
};

extern Logger logger;
