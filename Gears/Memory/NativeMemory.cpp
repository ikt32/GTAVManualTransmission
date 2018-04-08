#include "NativeMemory.hpp"

#include <Windows.h>
#include <Psapi.h>
#include "Util/Logger.hpp"

namespace mem {

    uintptr_t(*GetAddressOfEntity)(int entity) = nullptr;
    uintptr_t(*GetModelInfo)(unsigned int modelHash, int* index) = nullptr;

    void init() {
        auto addr = FindPattern("\x83\xF9\xFF\x74\x31\x4C\x8B\x0D\x00\x00\x00\x00\x44\x8B\xC1\x49\x8B\x41\x08",
                                                            "xxxxxxxx????xxxxxxx");
        if (!addr) logger.Write(ERROR, "Couldn't find GetAddressOfEntity");
        GetAddressOfEntity = reinterpret_cast<uintptr_t(*)(int)>(addr);
        
        addr = FindPattern("\x0F\xB7\x05\x00\x00\x00\x00"
            "\x45\x33\xC9\x4C\x8B\xDA\x66\x85\xC0"
            "\x0F\x84\x00\x00\x00\x00"
            "\x44\x0F\xB7\xC0\x33\xD2\x8B\xC1\x41\xF7\xF0\x48"
            "\x8B\x05\x00\x00\x00\x00"
            "\x4C\x8B\x14\xD0\xEB\x09\x41\x3B\x0A\x74\x54",
            "xxx????xxxxxxxxxxx????"
            "xxxxxxxxxxxxxx????xxxxxxxxxxx");
        if (!addr) logger.Write(ERROR, "Couldn't find GetModelInfo");
        GetModelInfo = reinterpret_cast<uintptr_t(*)(unsigned int modelHash, int *index)>(addr);
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
                if (mask[pos + 1] == '\0')
                    return (reinterpret_cast<uintptr_t>(retAddress) - searchLen);
                pos++;
            }
            else {
                pos = 0;
            }
        }
        return 0;
    }
}
