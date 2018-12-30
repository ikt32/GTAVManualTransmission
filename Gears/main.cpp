#include <inc/main.h>

#include <filesystem>
#include <Psapi.h>

#include "script.h"

#include "Constants.h"
#include "Memory/MemoryPatcher.hpp"
#include "Memory/VehicleExtensions.hpp"
#include "Memory/Versions.h"
#include "Util/Logger.hpp"
#include "Util/Paths.h"
#include "Util/StringFormat.h"

#pragma comment(lib,"Version.lib")

namespace fs = std::filesystem;

bool isModulePresent(const std::string& name, std::string& modulePath) {
    bool found = false;
    
    HMODULE hMods[1024];
    DWORD cbNeeded;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
        PROCESS_VM_READ,
        FALSE, GetCurrentProcessId());

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (unsigned int i = 0; i < cbNeeded / sizeof(HMODULE); ++i) {
            CHAR szModName[MAX_PATH];
            if (GetModuleFileNameExA(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
                if (strfind(szModName, name)) {
                    found = true;
                    modulePath = szModName;
                    break;
                }
            }
        }
    }
    CloseHandle(hProcess);
    return found;
}

int getExeVersion(std::string exe) {
    DWORD  verHandle = 0;
    UINT   size = 0;
    LPBYTE lpBuffer = NULL;
    DWORD  verSize = GetFileVersionInfoSizeA(exe.c_str(), &verHandle);
    if (verSize != 0) {
        std::vector<char> verData(verSize);
        if (GetFileVersionInfoA(exe.c_str(), verHandle, verSize, verData.data())) {
            if (VerQueryValueA(verData.data(), "\\", (VOID FAR* FAR*)&lpBuffer, &size)) {
                if (size) {
                    VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
                    if (verInfo->dwSignature == VS_FFI_SIGNATURE) {
                        // Doesn't matter if you are on 32 bit or 64 bit,
                        // DWORD is always 32 bits, so first two revision numbers
                        // come from dwFileVersionMS, last two come from dwFileVersionLS
                        logger.Write(INFO, "File Version: %d.%d.%d.%d",
                                     (verInfo->dwFileVersionMS >> 16) & 0xffff,
                                     (verInfo->dwFileVersionMS >> 0) & 0xffff,
                                     (verInfo->dwFileVersionLS >> 16) & 0xffff,
                                     (verInfo->dwFileVersionLS >> 0) & 0xffff
                        );
                        return (verInfo->dwFileVersionLS >> 16) & 0xffff;
                    }
                }
            }
        }
    }
    logger.Write(ERROR, "File version detection failed");
    return -1;
}

int getExeInfo() {
    std::string currExe = Paths::GetRunningExecutablePath();
    logger.Write(INFO, "Running executable: %s", currExe.c_str());
    std::string citizenDir;
    bool fiveM = isModulePresent("CitizenGame.dll", citizenDir);

    if (fiveM) {
        logger.Write(INFO, "FiveM detected");
        auto FiveMApp = std::string(citizenDir).substr(0, std::string(citizenDir).find_last_of('\\'));
        logger.Write(INFO, "FiveM.app dir: %s", FiveMApp.c_str());
        auto cacheFolder = FiveMApp + "\\cache\\game";
        std::string newestExe;
        fs::directory_entry newestExeEntry;
        for (auto &file : fs::directory_iterator(cacheFolder)) {
            if (strfind(fs::path(file).string(), "GTA5.exe")) {
                if (newestExe.empty() || last_write_time(newestExeEntry) < last_write_time(file)) {
                    newestExe = fs::path(file).string();
                    newestExeEntry = file;
                }
            }
        }
        currExe = newestExe;
    }
    return getExeVersion(currExe);
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved) {
    const std::string modPath = Paths::GetModuleFolder(hInstance) + mtDir;
    const std::string logFile = modPath + "\\" + Paths::GetModuleNameWithoutExtension(hInstance) + ".log";

    if (!fs::is_directory(modPath) || !fs::exists(modPath)) {
        fs::create_directory(modPath);
    }

    logger.SetFile(logFile);
    Paths::SetOurModuleHandle(hInstance);

    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            int scriptingVersion = getGameVersion();
            logger.Clear();
            logger.Write(INFO, "GTAVManualTransmission %s (build %s)", DISPLAY_VERSION, __DATE__);
            logger.Write(INFO, "Game version " + eGameVersionToString(scriptingVersion));
            if (scriptingVersion < G_VER_1_0_877_1_STEAM) {
                logger.Write(WARN, "Unsupported game version! Update your game.");
            }

            int exeVersion = getExeInfo();
            int actualVersion = findNextLowest(ExeVersionMap, exeVersion);
            if (scriptingVersion % 2) {
                scriptingVersion--;
            }
            if (actualVersion != scriptingVersion) {
                logger.Write(WARN, "Version mismatch!");
                logger.Write(WARN, "    Detected: %s", eGameVersionToString(actualVersion).c_str());
                logger.Write(WARN, "    Reported: %s", eGameVersionToString(scriptingVersion).c_str());
                if (actualVersion == -1) {
                    logger.Write(WARN, "Version detection failed");
                    logger.Write(WARN, "    Using reported version (%s)", eGameVersionToString(scriptingVersion).c_str());
                    actualVersion = scriptingVersion;
                }
                else {
                    logger.Write(WARN, "Using detected version (%s)", eGameVersionToString(actualVersion).c_str());
                }
                MemoryPatcher::SetPatterns(actualVersion);
                VehicleExtensions::ChangeVersion(actualVersion);
            }
            else {                
                MemoryPatcher::SetPatterns(scriptingVersion);
            }

            if (actualVersion >= G_VER_1_0_1604_0_STEAM) {
                g_numGears = 11;
            }
            
            scriptRegister(hInstance, ScriptMain);
            scriptRegisterAdditionalThread(hInstance, NPCMain);
            
            logger.Write(INFO, "Script registered");
            break;
        }
        case DLL_PROCESS_DETACH: {
            logger.Write(INFO, "PATCH: Init shutdown");
            const uint8_t expected = 5;
            uint8_t actual = 0;

            if (MemoryPatcher::RevertGearboxPatches()) 
                actual++;
            if (MemoryPatcher::RestoreSteeringAssist())
                actual++;
            if (MemoryPatcher::RestoreSteeringControl()) 
                actual++;
            if (MemoryPatcher::RestoreBrake())
                actual++;
            if (MemoryPatcher::RestoreThrottle())
                actual++;

            resetSteeringMultiplier();
            stopForceFeedback();
            scriptUnregister(hInstance);

            if (actual == expected) {
                logger.Write(INFO, "PATCH: Script shut down cleanly");
            }
            else {
                logger.Write(ERROR, "PATCH: Script shut down with unrestored patches!");
            }
            break;
        }
        default:
            // Do nothing
            break;
    }
    return TRUE;
}
