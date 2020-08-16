#pragma once
namespace TrainerV {
    bool Active();
}

namespace Dismemberment {
    bool Available();
    void AddBoneDraw(int handle, int start, int end);
    void RemoveBoneDraw(int handle);
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
