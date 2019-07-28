#include "Compatibility.h"
#include "Util/Logger.hpp"
#include <Windows.h>
#include <string>

//bool(*TrainerV::Active)() = nullptr;

bool* TrainerV::Active = nullptr;

HMODULE TrainerVModule = nullptr;

template <typename T>
T CheckAddr(HMODULE lib, std::string funcName)
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

void setupCompatibility() {
    logger.Write(INFO, "Setting up TrainerV compatibility");
    TrainerVModule = GetModuleHandle(L"trainerv.asi");
    if (!TrainerVModule) {
        logger.Write(INFO, "TrainerV.asi not found");
        return;
    }
    //bool(*)()
    TrainerV::Active = CheckAddr<bool*>(TrainerVModule, "?Menu1@@3_NA");
}

void releaseCompatibility() {
    if (TrainerVModule && TrainerV::Active) {
        FreeLibrary(TrainerVModule);
    }
}
