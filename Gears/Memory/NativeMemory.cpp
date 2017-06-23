#include "NativeMemory.hpp"

#include <Windows.h>
#include <Psapi.h>

namespace mem {

	uint64_t(*GetAddressOfEntity)(int entity) = nullptr;

	void init() {
		if (GetAddressOfEntity == nullptr) {
			uintptr_t GetAddressOfEntityAddress = FindPattern("\x83\xF9\xFF\x74\x31\x4C\x8B\x0D\x00\x00\x00\x00\x44\x8B\xC1\x49\x8B\x41\x08",
															  "xxxxxxxx????xxxxxxx");
			GetAddressOfEntity = reinterpret_cast<uint64_t(*)(int)>(GetAddressOfEntityAddress);
		}
	}

	uintptr_t FindPattern(const char* pattern, const char* mask) {
		MODULEINFO modInfo = { nullptr };
		GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

		const char* start_offset = reinterpret_cast<const char *>(modInfo.lpBaseOfDll);
		const uintptr_t size = static_cast<uintptr_t>(modInfo.SizeOfImage);

		intptr_t pos = 0;
		const uintptr_t searchLen = static_cast<uintptr_t>(strlen(mask) - 1);

		for (const char* retAddress = start_offset; retAddress < start_offset + size; retAddress++) {
			if (*retAddress == pattern[pos] || mask[pos] == '?') {
				if (mask[pos + 1] == '\0') {
					return (reinterpret_cast<uintptr_t>(retAddress) - searchLen);
				}

				pos++;
			}
			else {
				pos = 0;
			}
		}

		return 0;
	}
}
