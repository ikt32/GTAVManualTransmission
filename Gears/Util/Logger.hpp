#pragma once
#include <string>

#define LOGFILE "./Gears.log"

class Logger {
public:
	Logger(std::string fileName);
	void Clear() const;
	void Write(const std::string& text) const;

private:
	std::string file;
};
