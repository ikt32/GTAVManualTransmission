#pragma once
#include <cstdint>

namespace mem {
    void init();
    uintptr_t FindPattern(const char* pattern, const char* mask); 
    extern uintptr_t(*GetAddressOfEntity)(int entity);
    extern uintptr_t(*GetModelInfo)(unsigned int modelHash, int* index);
}
