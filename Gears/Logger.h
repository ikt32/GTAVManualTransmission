#pragma once

#include <Windows.h>
#include <string>
#include <fstream>
#include <iomanip>

class Logger {
public:
	Logger(char *fileName);
	void Clear();
	void Write(const std::string &text);

private:
	char *file;
};