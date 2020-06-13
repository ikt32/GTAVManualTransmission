#pragma once
#include <cstdint>
#include <vector>

namespace mem {
void init();
uintptr_t FindPattern(const char* pattern, const char* mask); 
uintptr_t FindPattern(const char* pattStr);
std::vector<uintptr_t> FindPatterns(const char* pattern, const char* mask);
extern uintptr_t(*GetAddressOfEntity)(int entity);
extern uintptr_t(*GetModelInfo)(unsigned int modelHash, int* index);
}
