#pragma once

#include <iomanip>

#define LOGFILE "./Gears.log"

class Logger {
public:
	Logger(char *fileName);
	void Clear() const;
	void Write(const std::string &text) const;

private:
	char *file;
};