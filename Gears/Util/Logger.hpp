#pragma once
#include <string>

class Logger {
public:
    Logger();
    void SetFile(const std::string &fileName);
    void Clear() const;
    void Write(const std::string& text) const;
    int Writef(char *fmt, ...);

private:
    std::string file = "";
};

extern Logger logger;
