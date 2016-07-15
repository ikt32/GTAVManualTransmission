/*
THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
http://dev-c.com
(C) Alexander Blade 2015
*/

#include "..\..\ScriptHookV_SDK\inc\main.h"

#include "script.h"
#include "Logger.hpp"
#include "MemoryPatcher.hpp"

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	Logger logger(LOGFILE);
	std::string ver;
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		scriptRegister(hInstance, ScriptMain);
		logger.Clear();
		ver = "Version ";
		ver.append(std::to_string(getGameVersion()));
		logger.Write(ver); 
		logger.Write("Script loaded");
		break;
	case DLL_PROCESS_DETACH:
		logger.Write("Init shutdown");
		if (LogiIsConnected(0)) {
			resetWheelFeedback(0);
			LogiSteeringShutdown();
		}
		bool successI = MemoryPatcher::RestoreInstructions();
		//bool successJ = MemoryPatcher::RestoreJustS_LOW();

		if (successI) {// && successJ) {
			logger.Write("Shut down script successfully");
		}
		else {
			if (!successI)
				logger.Write("Shut down script with instructions not restored");
			//if (!successJ)
			//	logger.Write("Shut down script with S_LOW not restored");
		}
		scriptUnregister(hInstance);
		break;
	}
	return TRUE;
}