#include "Paths.h"

static HMODULE ourModule;

std::string Paths::GetRunningExecutableFolder() {
    char fileName[MAX_PATH];
    GetModuleFileNameA(nullptr, fileName, MAX_PATH);

    std::string currentPath = fileName;
    return currentPath.substr(0, currentPath.find_last_of("\\"));
}

std::string Paths::GetRunningExecutableName() {
    char fileName[MAX_PATH];
    GetModuleFileNameA(nullptr, fileName, MAX_PATH);

    std::string fullPath = fileName;

    size_t lastIndex = fullPath.find_last_of("\\") + 1;
    return fullPath.substr(lastIndex, fullPath.length() - lastIndex);
}

std::string Paths::GetRunningExecutableNameWithoutExtension() {
    std::string fileNameWithExtension = GetRunningExecutableName();
    size_t lastIndex = fileNameWithExtension.find_last_of(".");
    if (lastIndex == -1) {
        return fileNameWithExtension;
    }

    return fileNameWithExtension.substr(0, lastIndex);
}

std::string Paths::GetModuleFolder(const HMODULE module) {
    char fileName[MAX_PATH];
    GetModuleFileNameA(module, fileName, MAX_PATH);

    std::string currentPath = fileName;
    return currentPath.substr(0, currentPath.find_last_of("\\"));
}

std::string Paths::GetModuleName(const HMODULE module) {
    char fileName[MAX_PATH];
    GetModuleFileNameA(module, fileName, MAX_PATH);

    std::string fullPath = fileName;

    size_t lastIndex = fullPath.find_last_of("\\") + 1;
    return fullPath.substr(lastIndex, fullPath.length() - lastIndex);
}

std::string Paths::GetModuleNameWithoutExtension(const HMODULE module) {
    const std::string fileNameWithExtension = GetModuleName(module);

    size_t lastIndex = fileNameWithExtension.find_last_of(".");
    if (lastIndex == -1) {
        return fileNameWithExtension;
    }

    return fileNameWithExtension.substr(0, lastIndex);
}


void Paths::SetOurModuleHandle(const HMODULE module) {
    ourModule = module;
}

HMODULE Paths::GetOurModuleHandle() {
    return ourModule;
}