#include "General.h"

static HMODULE ourModule;

const std::string Util::GetRunningExecutableFolder() {

	char fileName[MAX_PATH];
	GetModuleFileNameA(NULL, fileName, MAX_PATH);

	std::string currentPath = fileName;
	return currentPath.substr(0, currentPath.find_last_of("\\"));
}

const std::string Util::GetRunningExecutableName() {

	char fileName[MAX_PATH];
	GetModuleFileNameA(NULL, fileName, MAX_PATH);

	std::string fullPath = fileName;

	size_t lastIndex = fullPath.find_last_of("\\") + 1;
	return fullPath.substr(lastIndex, fullPath.length() - lastIndex);
}

const std::string Util::GetRunningExecutableNameWithoutExtension() {
	const std::string fileNameWithExtension = GetRunningExecutableName();

	size_t lastIndex = fileNameWithExtension.find_last_of(".");
	if (lastIndex == -1) {
		return fileNameWithExtension;
	}

	return fileNameWithExtension.substr(0, lastIndex);
}

const std::string Util::GetModuleFolder(const HMODULE module) {

	char fileName[MAX_PATH];
	GetModuleFileNameA(module, fileName, MAX_PATH);

	std::string currentPath = fileName;
	return currentPath.substr(0, currentPath.find_last_of("\\"));
}

const std::string Util::GetModuleName(const HMODULE module) {

	char fileName[MAX_PATH];
	GetModuleFileNameA(module, fileName, MAX_PATH);

	std::string fullPath = fileName;

	size_t lastIndex = fullPath.find_last_of("\\") + 1;
	return fullPath.substr(lastIndex, fullPath.length() - lastIndex);
}

const std::string Util::GetModuleNameWithoutExtension(const HMODULE module) {

	const std::string fileNameWithExtension = GetModuleName(module);

	size_t lastIndex = fileNameWithExtension.find_last_of(".");
	if (lastIndex == -1) {
		return fileNameWithExtension;
	}

	return fileNameWithExtension.substr(0, lastIndex);
}


void Util::SetOurModuleHandle(const HMODULE module) {

	ourModule = module;
}

const HMODULE Util::GetOurModuleHandle() {

	return ourModule;
}