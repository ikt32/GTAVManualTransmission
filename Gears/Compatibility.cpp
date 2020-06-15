#include "Compatibility.h"
#include "Util/Logger.hpp"

#include <GTAVDashHook/DashHook/DashHook.h>

#include <Windows.h>
#include <string>

bool* g_TrainerVActive = nullptr;
HMODULE g_TrainerVModule = nullptr;

bool g_DashHookLoadLibbed = false;
HMODULE g_DashHookModule = nullptr;

void(*g_DashHook_GetData)(VehicleDashboardData*);
void(*g_DashHook_SetData)(VehicleDashboardData);

HMODULE g_DismembermentModule = nullptr;

void(*g_Dismemberbent_AddBoneDraw)(int handle, int start, int end) = nullptr;
void(*g_Dismemberbent_RemoveBoneDraw)(int handle) = nullptr;

void DashHook_GetData(VehicleDashboardData* data) {
    if (g_DashHook_GetData) {
        g_DashHook_GetData(data);
    }
}

void DashHook_SetData(VehicleDashboardData data) {
    if (g_DashHook_SetData) {
        g_DashHook_SetData(data);
    }
}

template <typename T>
T CheckAddr(HMODULE lib, const std::string& funcName)
{
    FARPROC func = GetProcAddress(lib, funcName.c_str());
    if (!func)
    {
        logger.Write(ERROR, "Couldn't get function [%s]", funcName.c_str());
        return nullptr;
    }
    logger.Write(DEBUG, "Found function [%s]", funcName.c_str());
    return reinterpret_cast<T>(func);
}

void setupTrainerV() {
    logger.Write(INFO, "Setting up TrainerV compatibility");
    g_TrainerVModule = GetModuleHandle(L"trainerv.asi");
    if (!g_TrainerVModule) {
        logger.Write(INFO, "TrainerV.asi not found");
        return;
    }
    g_TrainerVActive = CheckAddr<bool*>(g_TrainerVModule, "?Menu1@@3_NA");
}

void setupDashHook() {
    g_DashHookLoadLibbed = false;
    logger.Write(INFO, "Setting up DashHook compatibility");
    g_DashHookModule = GetModuleHandle(L"DashHook.dll");
    if (!g_DashHookModule) {
        logger.Write(INFO, "DashHook.dll not found running, loading...");
        g_DashHookModule = LoadLibrary(L"DashHook.dll");
        if (!g_DashHookModule) {
            DWORD lastError = GetLastError();
            logger.Write(INFO, "DashHook.dll load failed, error [%lu]", lastError);
            return;
        }
        g_DashHookLoadLibbed = true;
    }

    g_DashHook_GetData = CheckAddr<void(*)(VehicleDashboardData*)>(g_DashHookModule, "DashHook_GetData");
    g_DashHook_SetData = CheckAddr<void(*)(VehicleDashboardData)>(g_DashHookModule, "DashHook_SetData");
}

void setupDismemberment() {
    logger.Write(INFO, "Setting up DismembermentASI");
    g_DismembermentModule = GetModuleHandle(L"DismembermentASI.asi");
    if (!g_DismembermentModule) {
        logger.Write(INFO, "DismembermentASI.asi not found");
        return;
    }

    g_Dismemberbent_AddBoneDraw = CheckAddr<void(*)(int, int, int)>(g_DismembermentModule, "AddBoneDraw");
    g_Dismemberbent_RemoveBoneDraw = CheckAddr<void(*)(int)>(g_DismembermentModule, "RemoveBoneDraw");
}

void setupCompatibility() {
    setupTrainerV();
    setupDashHook();
    setupDismemberment();
}

bool TrainerV::Active() {
    if (g_TrainerVActive)
        return *g_TrainerVActive;
    return false;
}

void releaseCompatibility() {
    g_TrainerVModule = nullptr;
    g_TrainerVActive = nullptr;

    if (g_DashHookLoadLibbed) {
        logger.Write(DEBUG, "DashHook.dll FreeLibrary");
        if (FreeLibrary(g_DashHookModule)) {
            g_DashHookModule = nullptr;
        }
        else {
            logger.Write(ERROR, "DashHook.dll FreeLibrary failed [%ul]", GetLastError());
        }
    }
    g_DashHook_GetData = nullptr;
    g_DashHook_SetData = nullptr;

    g_DismembermentModule = nullptr;
    g_Dismemberbent_AddBoneDraw = nullptr;
    g_Dismemberbent_RemoveBoneDraw = nullptr;
}

bool Dismemberment::Available() {
    return g_Dismemberbent_AddBoneDraw != nullptr &&
        g_Dismemberbent_RemoveBoneDraw != nullptr;
}

void Dismemberment::AddBoneDraw(int handle, int start, int end) {
    if (g_Dismemberbent_AddBoneDraw)
        g_Dismemberbent_AddBoneDraw(handle, start, end);
}

void Dismemberment::RemoveBoneDraw(int handle) {
    if (g_Dismemberbent_RemoveBoneDraw)
        g_Dismemberbent_RemoveBoneDraw(handle);
}

