#include <Windows.h>
#include <fstream>
#include <iomanip>

#include "Logger.hpp"

Logger::Logger() {}

void Logger::Clear() const {
	if (file == "") {
		std::ofstream logFile(bakFile, std::ios_base::out | std::ios_base::app);
		SYSTEMTIME currTimeLog;
		GetLocalTime(&currTimeLog);
		logFile << "[" <<
			std::setw(2) << std::setfill('0') << currTimeLog.wHour << ":" <<
			std::setw(2) << std::setfill('0') << currTimeLog.wMinute << ":" <<
			std::setw(2) << std::setfill('0') << currTimeLog.wSecond << "." <<
			std::setw(3) << std::setfill('0') << currTimeLog.wMilliseconds << "] " <<
			"ERROR: Clear on invalid log file" << "\n";
	}
	std::ofstream logFile;
	logFile.open(file, std::ofstream::out | std::ofstream::trunc);
	logFile.close();
}

void Logger::Write(const std::string& text) const {
	if (file == "") {
		std::ofstream logFile(bakFile, std::ios_base::out | std::ios_base::app);
		SYSTEMTIME currTimeLog;
		GetLocalTime(&currTimeLog);
		// [HH:MM:SS.mmm] 
		logFile << "[" <<
			std::setw(2) << std::setfill('0') << currTimeLog.wHour << ":" <<
			std::setw(2) << std::setfill('0') << currTimeLog.wMinute << ":" <<
			std::setw(2) << std::setfill('0') << currTimeLog.wSecond << "." <<
			std::setw(3) << std::setfill('0') << currTimeLog.wMilliseconds << "] " <<
			"ERROR: Write to invalid log file: \n" << 
			"               " << text << "\n";
	}
	std::ofstream logFile(file, std::ios_base::out | std::ios_base::app);
	SYSTEMTIME currTimeLog;
	GetLocalTime(&currTimeLog);
	logFile << "[" <<
	           std::setw(2) << std::setfill('0') << currTimeLog.wHour << ":" <<
	           std::setw(2) << std::setfill('0') << currTimeLog.wMinute << ":" <<
	           std::setw(2) << std::setfill('0') << currTimeLog.wSecond << "." <<
	           std::setw(3) << std::setfill('0') << currTimeLog.wMilliseconds << "] " <<
	           text << "\n";
}

void Logger::SetFile(const std::string &fileName) {
	file = fileName;
}

// Everything's gonna use this instance.
Logger logger;
