/*
THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
http://dev-c.com
(C) Alexander Blade 2015
*/

#include "..\..\ScriptHookV_SDK\inc\main.h"
#include "script.h"

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		scriptRegister(hInstance, ScriptMain);
		clearLog();
		writeToLog("Script loaded");
		break;
	case DLL_PROCESS_DETACH:
		writeToLog("Init shutdown");
		bool success = restoreClutchInstructions();
		if (success) {
			writeToLog("Shut down script successfully");
		}
		else {
			writeToLog("Shut down script with leftovers");
		}
		scriptUnregister(hInstance);
		break;
	}
	return TRUE;
}