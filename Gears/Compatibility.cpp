#include "Compatibility.h"
#include "Util/Logger.hpp"
#include "Util/Paths.h"

#include <GTAVDashHook/DashHook/DashHook.h>
#include <GTAVCustomTorqueMap/GTAVCustomTorqueMap/CustomTorqueMap.hpp>

#include <Windows.h>
#include <string>

HMODULE g_TrainerVModule = nullptr;
bool* g_TrainerVActive = nullptr;

HMODULE g_DashHookModule = nullptr;
void(*g_DashHook_GetData)(VehicleDashboardData*);
void(*g_DashHook_SetData)(VehicleDashboardData);

HMODULE g_DismembermentModule = nullptr;
void(*g_Dismemberbent_AddBoneDraw)(int handle, int start, int end) = nullptr;
void(*g_Dismemberbent_RemoveBoneDraw)(int handle) = nullptr;

HMODULE g_HandlingReplacementModule = nullptr;
bool(*g_HR_Enable)(int handle, void** pHandlingData) = nullptr;
bool(*g_HR_Disable)(int handle, void** pHandlingData) = nullptr;
bool(*g_HR_GetHandlingData)(int handle, void** pHandlingDataOriginal, void** pHandlingDataReplaced) = nullptr;

HMODULE g_CustomTorqueMapModule = nullptr;
bool(*g_CTM_GetPlayerRPMInfo)(CTM_RPMInfo* rpmInfo) = nullptr;

template <typename T>
T CheckAddr(HMODULE lib, const std::string& funcName) {
    FARPROC func = GetProcAddress(lib, funcName.c_str());
    if (!func) {
        logger.Write(ERROR, "[Compat] Couldn't get function [%s]", funcName.c_str());
        return nullptr;
    }
    logger.Write(DEBUG, "[Compat] Found function [%s]", funcName.c_str());
    return reinterpret_cast<T>(func);
}

void setupTrainerV() {
    logger.Write(INFO, "[Compat] Setting up TrainerV");
    g_TrainerVModule = GetModuleHandle("trainerv.asi");
    if (!g_TrainerVModule) {
        logger.Write(INFO, "[Compat] TrainerV.asi not found");
        return;
    }
    g_TrainerVActive = CheckAddr<bool*>(g_TrainerVModule, "?Menu1@@3_NA");
}

void setupDashHook() {
    logger.Write(INFO, "[Compat] Setting up DashHook");
    const std::string dashHookPath = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + "\\DashHook.dll";
    g_DashHookModule = LoadLibrary(dashHookPath.c_str());
    if (!g_DashHookModule) {
        logger.Write(INFO, "[Compat] DashHook.dll not found");
        return;
    }

    g_DashHook_GetData = CheckAddr<void(*)(VehicleDashboardData*)>(g_DashHookModule, "DashHook_GetData");
    g_DashHook_SetData = CheckAddr<void(*)(VehicleDashboardData)>(g_DashHookModule, "DashHook_SetData");
}

void setupDismemberment() {
    logger.Write(INFO, "[Compat] Setting up DismembermentASI");
    g_DismembermentModule = GetModuleHandle("DismembermentASI.asi");
    if (!g_DismembermentModule) {
        logger.Write(INFO, "[Compat] DismembermentASI.asi not found");
        return;
    }

    g_Dismemberbent_AddBoneDraw = CheckAddr<void(*)(int, int, int)>(g_DismembermentModule, "AddBoneDraw");
    g_Dismemberbent_RemoveBoneDraw = CheckAddr<void(*)(int)>(g_DismembermentModule, "RemoveBoneDraw");
}

void setupHandlingReplacement() {
    logger.Write(INFO, "[Compat] Setting up HandlingReplacement");
    g_HandlingReplacementModule = GetModuleHandle("HandlingReplacement.asi");
    if (!g_HandlingReplacementModule) {
        logger.Write(INFO, "[Compat] HandlingReplacement.asi not found");
        return;
    }

    g_HR_Enable = CheckAddr<bool(*)(int, void**)>(g_HandlingReplacementModule, "HR_Enable");
    g_HR_Disable = CheckAddr<bool(*)(int, void**)>(g_HandlingReplacementModule, "HR_Disable");
    g_HR_GetHandlingData = CheckAddr<bool(*)(int, void**, void**)>(g_HandlingReplacementModule, "HR_GetHandlingData");
}

void setupCustomTorqueMap() {
    logger.Write(INFO, "[Compat] Setting up CustomTorqueMap");
    g_CustomTorqueMapModule = GetModuleHandle("CustomTorqueMap.asi");
    if (!g_CustomTorqueMapModule) {
        logger.Write(INFO, "[Compat] CustomTorqueMap.asi not found");
        return;
    }

    g_CTM_GetPlayerRPMInfo = CheckAddr<bool(*)(CTM_RPMInfo*)>(g_CustomTorqueMapModule, "CTM_GetPlayerRPMInfo");
}

void setupCompatibility() {
    setupTrainerV();
    setupDashHook();
    setupDismemberment();
    setupHandlingReplacement();
    setupCustomTorqueMap();
}

void releaseCompatibility() {
    g_TrainerVModule = nullptr;
    g_TrainerVActive = nullptr;

    if (FreeLibrary(g_DashHookModule)) {
        g_DashHookModule = nullptr;
    }
    else {
        logger.Write(ERROR, "[Compat] DashHook.dll FreeLibrary failed [%ul]", GetLastError());
    }
    
    g_DashHook_GetData = nullptr;
    g_DashHook_SetData = nullptr;

    g_DismembermentModule = nullptr;
    g_Dismemberbent_AddBoneDraw = nullptr;
    g_Dismemberbent_RemoveBoneDraw = nullptr;
}

bool TrainerV::Active() {
    if (g_TrainerVActive)
        return *g_TrainerVActive;
    return false;
}

bool DashHook::Available() {
    return g_DashHook_GetData && g_DashHook_SetData;
}

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

bool HandlingReplacement::Available() {
    return g_HR_Enable && g_HR_Disable && g_HR_GetHandlingData;
}

bool HandlingReplacement::Enable(int vehicle, void** pHandlingData) {
    if (g_HR_Enable) {
        return g_HR_Enable(vehicle, pHandlingData);
    }
    return false;
}

bool HandlingReplacement::Disable(int vehicle, void** pHandlingData) {
    if (g_HR_Disable) {
        return g_HR_Disable(vehicle, pHandlingData);
    }
    return false;
}

bool HandlingReplacement::GetHandlingData(int vehicle, void** pHandlingDataOriginal, void** pHandlingDataReplaced) {
    if (g_HR_GetHandlingData) {
        return g_HR_GetHandlingData(vehicle, pHandlingDataOriginal, pHandlingDataReplaced);
    }
    return false;
}

bool CTM_GetPlayerRPMInfo(CTM_RPMInfo* rpmInfo) {
    if (g_CTM_GetPlayerRPMInfo) {
        return g_CTM_GetPlayerRPMInfo(rpmInfo);
    }
    return false;
}
