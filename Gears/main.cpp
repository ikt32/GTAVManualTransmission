#include <inc/main.h>

#include <filesystem>
#include <Psapi.h>

#include "script.h"

#include "Compatibility.h"
#include "Constants.h"
#include "Memory/MemoryPatcher.hpp"
#include "Memory/VehicleExtensions.hpp"
#include "Memory/Versions.h"
#include "Util/FileVersion.h"
#include "Util/Logger.hpp"
#include "Util/Paths.h"

namespace fs = std::filesystem;

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
            logger.Write(INFO, "Manual Transmission %s (built %s %s)", DISPLAY_VERSION, __DATE__, __TIME__);
            logger.Write(INFO, "Game version " + eGameVersionToString(scriptingVersion));
            if (scriptingVersion < G_VER_1_0_877_1_STEAM) {
                logger.Write(WARN, "Unsupported game version! Update your game.");
            }

            SVersion exeVersion = getExeInfo();
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

            if (actual == expected) {
                logger.Write(INFO, "PATCH: Script shut down cleanly");
            }
            else {
                logger.Write(ERROR, "PATCH: Script shut down with unrestored patches!");
            }

            resetSteeringMultiplier();
            stopForceFeedback();
            releaseCompatibility();

            scriptUnregister(hInstance);
            break;
        }
        default:
            // Do nothing
            break;
    }
    return TRUE;
}
