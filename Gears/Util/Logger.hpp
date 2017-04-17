#pragma once
#include <string>

class Logger {
public:
	Logger();
	void Clear() const;
	void Write(const std::string& text) const;
	void SetFile(const std::string &fileName);

private:
	std::string file = "";
	std::string bakFile = "./GearsERR.log";
};

extern Logger logger;
