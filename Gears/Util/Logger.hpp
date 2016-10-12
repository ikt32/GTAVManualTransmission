#pragma once
#include <string>

#define LOGFILE "./Gears.log"

class Logger {
public:
	explicit Logger(char* fileName);
	void Clear() const;
	void Write(const std::string& text) const;

private:
	char* file;
};
