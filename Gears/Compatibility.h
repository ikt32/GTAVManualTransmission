#pragma once
namespace TrainerV {
    extern bool Active();
}

namespace Dismemberment {
    extern bool Available();
    extern void AddBoneDraw(int handle, int start, int end);
    extern void RemoveBoneDraw(int handle);
}

void setupCompatibility();

void releaseCompatibility();
