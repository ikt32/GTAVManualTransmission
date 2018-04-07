#pragma once
#include <Windows.h>
#include <string>

namespace Paths {
std::string	GetRunningExecutableFolder();
std::string	GetRunningExecutableName();
std::string	GetRunningExecutableNameWithoutExtension();

std::string	GetModuleFolder(const HMODULE module);
std::string	GetModuleName(const HMODULE module);
std::string	GetModuleNameWithoutExtension(const HMODULE module);

    void				SetOurModuleHandle(const HMODULE module);
HMODULE		GetOurModuleHandle();
}
