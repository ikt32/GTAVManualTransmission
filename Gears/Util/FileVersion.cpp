#include "FileVersion.h"

#include <filesystem>
#include <Windows.h>
#include <Psapi.h>

#include "../Util/Logger.hpp"
#include "../Util/Paths.h"

#pragma comment(lib, "Version.lib")

namespace fs = std::filesystem;

bool operator==(const SVersion& a, const SVersion& b) {
    return a.Minor == b.Minor && a.Build == b.Build;
}

bool operator<=(const SVersion& a, const SVersion& b) {
    if (a.Minor <= b.Minor && a.Build <= b.Build)
        return true;
    if (a.Minor == b.Minor)
        if (a.Build <= b.Build)
            return true;
    return false;
}

SVersion getExeVersion(const std::string & exe) {
    DWORD  verHandle = 0;
    UINT   size = 0;
    LPBYTE lpBuffer = NULL;
    DWORD  verSize = GetFileVersionInfoSizeA(exe.c_str(), &verHandle);
    if (verSize != 0) {
        std::vector<char> verData(verSize);
        if (GetFileVersionInfoA(exe.c_str(), verHandle, verSize, verData.data())) {
            if (VerQueryValueA(verData.data(), "\\", (VOID FAR * FAR*) & lpBuffer, &size)) {
                if (size) {
                    VS_FIXEDFILEINFO* verInfo = (VS_FIXEDFILEINFO*)lpBuffer;
                    if (verInfo->dwSignature == VS_FFI_SIGNATURE) {
                        // Doesn't matter if you are on 32 bit or 64 bit,
                        // DWORD is always 32 bits, so first two revision numbers
                        // come from dwFileVersionMS, last two come from dwFileVersionLS
                        logger.Write(INFO, "File Version: %d.%d.%d.%d",
                            verInfo->dwFileVersionMS >> 16 & 0xffff,
                            verInfo->dwFileVersionMS >> 0 & 0xffff,
                            verInfo->dwFileVersionLS >> 16 & 0xffff,
                            verInfo->dwFileVersionLS >> 0 & 0xffff
                        );
                        return { verInfo->dwFileVersionLS >> 16 & 0xffff,
                                 verInfo->dwFileVersionLS >> 0 & 0xffff };
                    }
                }
            }
        }
    }
    logger.Write(ERROR, "File version detection failed");
    return { 0, 0 };
}

SVersion getExeInfo() {
    std::string currExe = Paths::GetRunningExecutablePath();
    logger.Write(INFO, "Running executable: %s", currExe.c_str());
    return getExeVersion(currExe);
}
