/*
THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
http://dev-c.com
(C) Alexander Blade 2015
*/

#include "..\..\ScriptHookV_SDK\inc\main.h"

#include "script.h"
#include "Input/keyboard.h"
#include "Util/Paths.h"
#include "Util/Logger.hpp"
#include "Memory/MemoryPatcher.hpp"
#include "Util/Versions.h"


BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved) {
	std::string logFile = Paths::GetModuleFolder(hInstance) + mtDir +
		"\\" + Paths::GetModuleNameWithoutExtension(hInstance) + ".log";
	logger.SetFile(logFile);
	Paths::SetOurModuleHandle(hInstance);

	// ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
	switch (reason) {
		case DLL_PROCESS_ATTACH: {
			scriptRegister(hInstance, ScriptMain);
			//keyboardHandlerRegister(OnKeyboardMessage);
			logger.Clear();
			logger.Write("GTAVManualTransmission " + std::string(DISPLAY_VERSION));
			logger.Write("Game version " + eGameVersionToString(getGameVersion()));
			logger.Write("Script registered");
			break;
		}
		case DLL_PROCESS_DETACH: {
			logger.Write("Init shutdown");
			bool successI = MemoryPatcher::RestoreInstructions();
			bool successS = MemoryPatcher::RestoreSteeringCorrection();
			scriptUnregister(hInstance);

			if (successI && successS) {
				logger.Write("Shut down script successfully");
			}
			else {
				if (!successI)
					logger.Write("Shut down script with instructions not restored");
				if (!successS)
					logger.Write("Shut down script with steering not restored");
			}
			//keyboardHandlerUnregister(OnKeyboardMessage);
			break;
		}
	}
	return TRUE;
}
