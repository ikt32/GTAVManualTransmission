#include "../../ScriptHookV_SDK/inc/main.h"

#include "script.h"
#include "Input/keyboard.h"
#include "Util/Paths.h"
#include "Util/Logger.hpp"
#include "Memory/MemoryPatcher.hpp"
#include "Memory/Versions.h"
#include "Constants.h"
#include "Util/StringFormat.h"
#include <filesystem>
#include "Memory/VehicleExtensions.hpp"

#pragma comment(lib,"Version.lib")

namespace fs = std::experimental::filesystem;
std::vector<std::string> scanFolder(std::string path) {
    std::vector<std::string> files;
    for (auto &file : fs::directory_iterator(path)) {
        std::string fileName = fs::path(file).string();
        
    }
}

int getExeVersion(std::string newestExe) {
    DWORD  verHandle = 0;
    UINT   size = 0;
    LPBYTE lpBuffer = NULL;
    DWORD  verSize = GetFileVersionInfoSizeA(newestExe.c_str(), &verHandle);
    if (verSize != 0) {
        LPSTR verData = new char[verSize];
        if (GetFileVersionInfoA(newestExe.c_str(), verHandle, verSize, verData)) {
            if (VerQueryValueA(verData, "\\", (VOID FAR* FAR*)&lpBuffer, &size)) {
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
        delete[] verData;
    }
    return -1;
}

int getExeInfo() {
    char filename[MAX_PATH];
    DWORD size = GetModuleFileNameA(NULL, filename, MAX_PATH);
    if (size) {
        logger.Write(INFO, "Executable: %s", filename);
        if (strfind(filename,"fivem")) {
            auto folder = std::string(filename).substr(0, std::string(filename).find_last_of("\\"));
            auto cacheFolder = folder + "\\FiveM.app\\cache\\game";
            std::string newestExe;
            fs::directory_entry newestExeEntry;
            for (auto &file : fs::directory_iterator(cacheFolder)) {
                if (strfind(fs::path(file).string(), "GTA5.exe")) {
                    if (newestExe == "" || last_write_time(newestExeEntry) < last_write_time(file)) {
                        newestExe = fs::path(file).string();
                        newestExeEntry = file;
                    }
                }
            }
            if (newestExe != "") {
                return getExeVersion(newestExe);
            }
        }
        else if (strfind(filename, "gta5.exe")){
            return getExeVersion(filename);
        }
        else {
            logger.Write(WARN, "Unknown game executable!");
        }
    }
    else {
        logger.Write(WARN, "Unknown game executable!");
    }
    return -1;
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved) {
    std::string logFile = Paths::GetModuleFolder(hInstance) + mtDir +
        "\\" + Paths::GetModuleNameWithoutExtension(hInstance) + ".log";
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
            int actualVersion = ExeVersionMap.lower_bound(getExeInfo())->second;
            if (scriptingVersion % 2) {
                scriptingVersion--;
            }
            if (actualVersion != scriptingVersion && actualVersion != -1) {
                logger.Write(WARN, "Version mismatch! Using %s for script!", eGameVersionToString(actualVersion).c_str());
                MemoryPatcher::SetPatterns(actualVersion);
                VehicleExtensions::ChangeVersion(actualVersion);
            }
            else {                
                MemoryPatcher::SetPatterns(scriptingVersion);
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
            if (MemoryPatcher::RestoreSteeringCorrection())
                actual++;
            if (MemoryPatcher::RestoreSteeringControl()) 
                actual++;
            if (MemoryPatcher::RestoreBrakeDecrement())
                actual++;
            if (MemoryPatcher::RestoreThrottleDecrement())
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
