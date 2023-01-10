#pragma once
namespace TrainerV {
    bool Active();
}

namespace DashHook {
    bool Available();
}

namespace HandlingReplacement {
    bool Available();
    bool Enable(int vehicle, void** ppHandlingData);
    bool Disable(int vehicle, void** ppHandlingData);
    bool GetHandlingData(int vehicle, void** ppHandlingDataOriginal, void** ppHandlingDataReplaced);
}

void setupCompatibility();

void releaseCompatibility();
