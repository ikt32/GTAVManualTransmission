#pragma once
#include <Windows.h>
#include <filesystem>
#include <string>

namespace Paths {
std::filesystem::path GetLocalAppDataPath();

void SetModPath(std::string path);
std::string GetModPath();
std::string GetInitialModPath();

void SetModPathChanged();
bool GetModPathChanged();

std::string GetRunningExecutablePath();
std::string	GetRunningExecutableFolder();
std::string	GetRunningExecutableName();
std::string	GetRunningExecutableNameWithoutExtension();

std::string	GetModuleFolder(HMODULE module);
std::string	GetModuleName(HMODULE module);
std::string	GetModuleNameWithoutExtension(HMODULE module);

void SetOurModuleHandle(HMODULE module);
HMODULE GetOurModuleHandle();

bool FileExists(const std::string& name);

std::wstring GetDocumentsFolder();
}
