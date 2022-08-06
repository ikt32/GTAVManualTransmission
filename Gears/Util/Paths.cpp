#include "Paths.h"

#include <ShlObj.h>

namespace {
    HMODULE ourModule;
    std::string modPath;
    std::string initialModPath;
    bool modPathChangedThisRun = false;
}

std::filesystem::path Paths::GetLocalAppDataPath() {
    std::filesystem::path fsPath;
    PWSTR path = NULL;
    HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);
    
    if (path != NULL) {
        fsPath = std::filesystem::path(path);
    }

    CoTaskMemFree(path);

    return fsPath;
}

void Paths::SetModPath(std::string path) {
    if (initialModPath.empty()) {
        initialModPath = path;
    }
    modPath = path;
}

std::string Paths::GetModPath() {
    return modPath;
}

std::string Paths::GetInitialModPath() {
    return initialModPath;
}

void Paths::SetModPathChanged() {
    modPathChangedThisRun = true;
}

bool Paths::GetModPathChanged() {
    return modPathChangedThisRun;
}

std::string Paths::GetRunningExecutablePath() {
    char fileName[MAX_PATH];
    GetModuleFileNameA(nullptr, fileName, MAX_PATH);
    return fileName;
}

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

bool Paths::FileExists(const std::string& name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

std::wstring Paths::GetDocumentsFolder() {
    PWSTR path;
    SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, NULL, &path);
    std::wstring strPath(path);
    CoTaskMemFree(path);

    return strPath;
}
