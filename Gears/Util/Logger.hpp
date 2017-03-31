#pragma once
#include <string>

/*
 * Due to how I use the logger in Gears, this little thing should only be used
 * when in project Gears in the Memory classes, main.cpp, and script.cpp.
 */
#define GEARSLOGPATH "./Gears.log"

class Logger {
public:
	Logger(const std::string &fileName);
	void Clear() const;
	void Write(const std::string& text) const;

private:
	std::string file;
};
