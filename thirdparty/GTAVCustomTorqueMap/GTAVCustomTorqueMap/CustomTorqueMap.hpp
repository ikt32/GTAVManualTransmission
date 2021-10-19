#pragma once

#ifdef CTM_EXPORTS
#define CTM_API extern "C" __declspec(dllexport)
#else
#ifndef CTM_RUNTIME
#define CTM_API extern "C" __declspec(dllimport)
#else
// noop
#define CTM_API
#endif
#endif

struct CTM_RPMInfo {
    float IdleRPM;
    float RevLimitRPM;
    float ActualRPM;
};

CTM_API bool CTM_GetPlayerRPMInfo(CTM_RPMInfo* rpmInfo);
