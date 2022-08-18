#include "script.h"

#include "Constants.h"
#include "GitInfo.h"

#include "SteeringAnim.h"

#include "Compatibility.h"
#include "Input/keyboard.h"
#include "Memory/MemoryPatcher.hpp"
#include "Memory/VehicleExtensions.hpp"
#include "Memory/Versions.h"
#include "Util/FileVersion.h"
#include "Util/Logger.hpp"
#include "Util/Paths.h"

#include <inc/main.h>
#include <fmt/format.h>
#include <Psapi.h>
#include <filesystem>
#include <atomic>

namespace fs = std::filesystem;

void resolveVersion() {
    int shvVersion = getGameVersion();

    logger.Write(INFO, "SHV API Game version: %s (%d)", eGameVersionToString(shvVersion).c_str(), shvVersion);
    // Also prints the other stuff, annoyingly.
    SVersion exeVersion = getExeInfo();

    if (shvVersion < G_VER_1_0_1604_0_STEAM) {
        logger.Write(WARN, "Outdated game version! Update your game.");
    }

    // Version we *explicitly* support
    std::vector<int> exeVersionsSupp = findNextLowest(ExeVersionMap, exeVersion);
    if (exeVersionsSupp.empty() || exeVersionsSupp.size() == 1 && exeVersionsSupp[0] == -1) {
        logger.Write(ERROR, "Failed to find a corresponding game version.");
        logger.Write(WARN, "    Using SHV API version [%s] (%d)", eGameVersionToString(shvVersion).c_str(), shvVersion);
        MemoryPatcher::SetPatterns(shvVersion);
        VehicleExtensions::SetVersion(shvVersion);
        return;
    }

    int highestSupportedVersion = *std::max_element(std::begin(exeVersionsSupp), std::end(exeVersionsSupp));
    if (shvVersion > highestSupportedVersion) {
        logger.Write(WARN, "Game newer than last supported version");
        logger.Write(WARN, "    You might experience instabilities or crashes");
        logger.Write(WARN, "    Using SHV API version [%s] (%d)", eGameVersionToString(shvVersion).c_str(), shvVersion);
        MemoryPatcher::SetPatterns(shvVersion);
        VehicleExtensions::SetVersion(shvVersion);
        return;
    }

    int lowestSupportedVersion = *std::min_element(std::begin(exeVersionsSupp), std::end(exeVersionsSupp));
    if (shvVersion < lowestSupportedVersion) {
        logger.Write(WARN, "SHV API reported lower version than actual EXE version.");
        logger.Write(WARN, "    EXE version     [%s] (%d)", eGameVersionToString(lowestSupportedVersion).c_str(), lowestSupportedVersion);
        logger.Write(WARN, "    SHV API version [%s] (%d)", eGameVersionToString(shvVersion).c_str(), shvVersion);
        logger.Write(WARN, "    Using EXE version, or highest supported version [%s] (%d)", eGameVersionToString(lowestSupportedVersion).c_str(), lowestSupportedVersion);
        MemoryPatcher::SetPatterns(lowestSupportedVersion);
        VehicleExtensions::SetVersion(lowestSupportedVersion);
        return;
    }

    logger.Write(INFO, "Using offsets based on SHV API version [%s] (%d)", eGameVersionToString(shvVersion).c_str(), shvVersion);
    MemoryPatcher::SetPatterns(shvVersion);
    VehicleExtensions::SetVersion(shvVersion);
}

void InitializePaths(HMODULE hInstance) {
    Paths::SetOurModuleHandle(hInstance);

    auto localAppDataPath       = Paths::GetLocalAppDataPath();
    auto localAppDataModPath    = localAppDataPath / Constants::iktDir / Constants::ModDir;
    std::string originalModPath = Paths::GetModuleFolder(hInstance) + std::string("\\") + Constants::ModDir;
    Paths::SetModPath(originalModPath);

    bool useAltModPath = false;
    if (std::filesystem::exists(localAppDataModPath)) {
        useAltModPath = true;
    }

    std::string modPath;
    std::string logFile;

    // Use LocalAppData if it already exists.
    if (useAltModPath) {
        modPath = localAppDataModPath.string();
        logFile = (localAppDataModPath / (Paths::GetModuleNameWithoutExtension(hInstance) + ".log")).string();
    } else {
        modPath = originalModPath;
        logFile = modPath + std::string("\\") + Paths::GetModuleNameWithoutExtension(hInstance) + ".log";
    }

    Paths::SetModPath(modPath);

    if (!fs::is_directory(modPath) || !fs::exists(modPath)) {
        fs::create_directories(modPath);
    }

    logger.SetFile(logFile);
    logger.Clear();

    if (logger.Error()) {
        modPath = localAppDataModPath.string();
        logFile = (localAppDataModPath / (Paths::GetModuleNameWithoutExtension(hInstance) + ".log")).string();
        fs::create_directories(modPath);

        Paths::SetModPath(modPath);
        logger.SetFile(logFile);

        fs::copy(fs::path(originalModPath), localAppDataModPath, fs::copy_options::update_existing | fs::copy_options::recursive);

        // Fix perms
        for (auto& path : fs::recursive_directory_iterator(localAppDataModPath)) {
            try {
                fs::permissions(path, fs::perms::all);
            } catch (std::exception& e) {
                logger.Write(ERROR, "Failed to set permissions on [%s]: %s.", path.path().string().c_str(), e.what());
            }
        }

        logger.ClearError();
        logger.Clear();
        logger.Write(WARN, "Copied to [%s] from [%s] due to read/write issues.", modPath.c_str(), originalModPath.c_str());
    }
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            InitializePaths(hInstance);
            logger.Write(INFO, "Manual Transmission %s (built %s %s) (%s)", Constants::DisplayVersion, __DATE__, __TIME__, GIT_HASH GIT_DIFF);
            logger.Write(INFO, "Date: %s", GetTimestampReadable(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()).c_str());
            resolveVersion();
            logger.Write(INFO, "Data path: %s", Paths::GetModPath().c_str());

            scriptRegister(hInstance, ScriptMain);

            logger.Write(INFO, "Script registered");
            break;
        }
        case DLL_PROCESS_DETACH: {
            scriptUnregister(hInstance);

            logger.Write(INFO, "[Patch] Init shutdown");
            const uint8_t expected = 6;
            uint8_t actual         = 0;

            if (MemoryPatcher::RevertGearboxPatches()) actual++;
            if (MemoryPatcher::RestoreSteeringAssist()) actual++;
            if (MemoryPatcher::RestoreSteeringControl()) actual++;
            if (MemoryPatcher::RestoreBrake()) actual++;
            if (MemoryPatcher::RestoreThrottle()) actual++;
            if (MemoryPatcher::RestoreThrottleControl()) actual++;

            if (actual == expected) {
                logger.Write(INFO, "[Patch] Script shut down cleanly");
            } else {
                logger.Write(ERROR, "[Patch] Script shut down with unrestored patches!");
            }

            resetSteeringMultiplier();
            logger.Write(DEBUG, "[Misc] Reset steering wheel multipliers");
            releaseCompatibility();
            logger.Write(DEBUG, "[Compat] Released libraries");

            SteeringAnimation::CancelAnimation();
            logger.Write(DEBUG, "[Anim] Cancelled custom animations");

            // This is where wheel stuff should've been explicitly shut down,
            // but due to GTA5.exe hanging, it's just left as-is.
            extern CarControls g_controls;
            g_controls.GetWheel().FreeDirectInput();
            break;
        }
        default:
            // Do nothing
            break;
    }
    return TRUE;
}
