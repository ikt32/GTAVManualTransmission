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
#ifdef _DEBUG
    logger.SetMinLevel(DEBUG);
#else
    logger.SetMinLevel(INFO);
#endif
    Paths::SetOurModuleHandle(hInstance);

    // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            scriptRegister(hInstance, ScriptMain);
            logger.Clear();
            logger.Write(INFO, "GTAVManualTransmission %s (build %s)", DISPLAY_VERSION, __DATE__);
            logger.Write(INFO, "Game version " + eGameVersionToString(getGameVersion()));
            if (getGameVersion() < G_VER_1_0_877_1_STEAM) {
                logger.Write(WARN, "Unsupported game version! Update your game.");
            }

            logger.Write(INFO, "Script registered");
            break;
        }
        case DLL_PROCESS_DETACH: {
            logger.Write(INFO, "Init shutdown");
            bool successI = MemoryPatcher::RestoreInstructions();
            bool successS = MemoryPatcher::RestoreSteeringCorrection();
            bool successSC= MemoryPatcher::RestoreSteeringControl();
            bool successB = MemoryPatcher::RestoreBrakeDecrement();
            resetSteeringMultiplier();
            stopForceFeedback();
            scriptUnregister(hInstance);

            if (successI && successS && successSC && successB) {
                logger.Write(INFO, "Shut down script successfully");
            }
            else {
                if (!successI)
                    logger.Write(WARN, "Shut down script with instructions not restored");
                if (!successS)
                    logger.Write(WARN, "Shut down script with steer correction not restored");
                if (!successSC)
                    logger.Write(WARN, "Shut down script with steer control not restored");
                if (!successB)
                    logger.Write(WARN, "Shut down script with brake decrement not restored");
            }
            break;
        }
    }
    return TRUE;
}
