#pragma once
#include <Windows.h>
#include <string>

namespace Paths {
	const std::string	GetRunningExecutableFolder();
	const std::string	GetRunningExecutableName();
	const std::string	GetRunningExecutableNameWithoutExtension();

	const std::string	GetModuleFolder(const HMODULE module);
	const std::string	GetModuleName(const HMODULE module);
	const std::string	GetModuleNameWithoutExtension(const HMODULE module);

	void				SetOurModuleHandle(const HMODULE module);
	const HMODULE		GetOurModuleHandle();
}
