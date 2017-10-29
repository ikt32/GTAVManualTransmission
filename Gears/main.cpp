#include "../../ScriptHookV_SDK/inc/main.h"

#include "script.h"
#include "Input/keyboard.h"
#include "Util/Paths.h"
#include "Util/Logger.hpp"
#include "Memory/MemoryPatcher.hpp"
#include "Memory/Versions.h"
#include "Memory/NativeMemory.hpp"


BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved) {
    mem::init();
    std::string logFile = Paths::GetModuleFolder(hInstance) + mtDir +
        "\\" + Paths::GetModuleNameWithoutExtension(hInstance) + ".log";
    logger.SetFile(logFile);
    Paths::SetOurModuleHandle(hInstance);

    // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            scriptRegister(hInstance, ScriptMain);
            logger.Clear();
            logger.Write("GTAVManualTransmission " + std::string(DISPLAY_VERSION));
            logger.Write("Game version " + eGameVersionToString(getGameVersion()));
            if (getGameVersion() < G_VER_1_0_877_1_STEAM) {
                logger.Write("WARNING: Unsupported game version! Update your game.");
            }

            logger.Write("Script registered");
            break;
        }
        case DLL_PROCESS_DETACH: {
            logger.Write("Init shutdown");
            bool successI = MemoryPatcher::RestoreInstructions();
            bool successS = MemoryPatcher::RestoreSteeringCorrection();
            bool successSC= MemoryPatcher::RestoreSteeringControl();
            bool successB = MemoryPatcher::RestoreBrakeDecrement();
            resetSteeringMultiplier();
            scriptUnregister(hInstance);

            if (successI && successS && successSC && successB) {
                logger.Write("Shut down script successfully");
            }
            else {
                if (!successI)
                    logger.Write("WARNING: Shut down script with instructions not restored");
                if (!successS)
                    logger.Write("WARNING: Shut down script with steer correction not restored");
                if (!successSC)
                    logger.Write("WARNING: Shut down script with steer control not restored");
                if (!successB)
                    logger.Write("WARNING: Shut down script with brake decrement not restored");
            }
            break;
        }
    }
    return TRUE;
}
