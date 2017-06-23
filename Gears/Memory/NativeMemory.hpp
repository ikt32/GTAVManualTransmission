#pragma once
#include <cstdint>

namespace mem {
	void init();
	extern uint64_t(*GetAddressOfEntity)(int entity);
	uintptr_t FindPattern(const char* pattern, const char* mask);
}
