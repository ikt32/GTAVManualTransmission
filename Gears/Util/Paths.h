#pragma once
#include <Windows.h>
#include <string>

namespace Paths {
std::string GetRunningExecutablePath();
std::string	GetRunningExecutableFolder();
std::string	GetRunningExecutableName();
std::string	GetRunningExecutableNameWithoutExtension();

std::string	GetModuleFolder(HMODULE module);
std::string	GetModuleName(HMODULE module);
std::string	GetModuleNameWithoutExtension(HMODULE module);

void SetOurModuleHandle(HMODULE module);
HMODULE GetOurModuleHandle();
}
