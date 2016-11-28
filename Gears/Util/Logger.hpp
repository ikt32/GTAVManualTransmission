#pragma once
#include <string>

static const std::string LOGFILE = "./Gears.log";

class Logger {
public:
	Logger();
	Logger(std::string fileName);
	void Clear() const;
	void Write(const std::string& text) const;

private:
	std::string file;
};
