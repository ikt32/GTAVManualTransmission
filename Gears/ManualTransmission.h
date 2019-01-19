#pragma once

extern "C" {
__declspec(dllexport) const char* MT_GetVersion();
__declspec(dllexport) bool MT_IsActive();
__declspec(dllexport) void MT_SetActive(bool active);
__declspec(dllexport) void MT_AddIgnoreVehicle(int vehicle);
__declspec(dllexport) void MT_DelIgnoreVehicle(int vehicle);
__declspec(dllexport) void MT_ClearIgnoredVehicles();
__declspec(dllexport) unsigned MT_NumIgnoredVehicles();
}