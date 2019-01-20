#pragma once

#ifdef MT_EXPORTS
#define MT_API __declspec(dllexport)
#else
#define MT_API __declspec(dllimport)
#endif

extern "C" {
MT_API const char*  MT_GetVersion();
MT_API bool         MT_IsActive();
MT_API void         MT_SetActive(bool active);

MT_API void         MT_AddIgnoreVehicle(int vehicle);
MT_API void         MT_DelIgnoreVehicle(int vehicle);
MT_API void         MT_ClearIgnoredVehicles();
MT_API unsigned     MT_NumIgnoredVehicles();
MT_API const int*   MT_GetIgnoredVehicles();
MT_API int          MT_GetManagedVehicle();
}