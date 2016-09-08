#pragma once

#include <iomanip>

#define LOGFILE "./Gears.log"

class Logger {
public:
	Logger(char *fileName);
	void Clear();
	void Write(const std::string &text);

private:
	char *file;
};