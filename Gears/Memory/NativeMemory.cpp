#include "NativeMemory.hpp"

#include "../Util/Logger.hpp"
#include <Windows.h>
#include <Psapi.h>
#include <sstream>

#include "inc/main.h"

namespace {
    template<typename Out>
    void split(const std::string& s, char delim, Out result) {
        std::stringstream ss;
        ss.str(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            *(result++) = item;
        }
    }

    std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> elems;
        ::split(s, delim, std::back_inserter(elems));
        return elems;
    }
}

extern eGameVersion g_gameVersion;

namespace mem {

    uintptr_t(*GetAddressOfEntity)(int entity) = nullptr;
    uintptr_t(*GetModelInfo)(unsigned int modelHash, int* index) = nullptr;

    void init() {
        auto addr = FindPattern("\x83\xF9\xFF\x74\x31\x4C\x8B\x0D\x00\x00\x00\x00\x44\x8B\xC1\x49\x8B\x41\x08",
                                                            "xxxxxxxx????xxxxxxx");
        if (!addr) logger.Write(ERROR, "Couldn't find GetAddressOfEntity");
        GetAddressOfEntity = reinterpret_cast<uintptr_t(*)(int)>(addr);

        if (g_gameVersion < 58) {
            addr = FindPattern(
                "\x0F\xB7\x05\x00\x00\x00\x00"
                "\x45\x33\xC9\x4C\x8B\xDA\x66\x85\xC0"
                "\x0F\x84\x00\x00\x00\x00"
                "\x44\x0F\xB7\xC0\x33\xD2\x8B\xC1\x41\xF7\xF0\x48"
                "\x8B\x05\x00\x00\x00\x00"
                "\x4C\x8B\x14\xD0\xEB\x09\x41\x3B\x0A\x74\x54",
                "xxx????"
                "xxxxxxxxx"
                "xx????"
                "xxxxxxxxxxxx"
                "xx????"
                "xxxxxxxxxxx");

            if (!addr) {
                logger.Write(ERROR, "Couldn't find GetModelInfo");
            }
        }
        else {
            addr = FindPattern("\xEB\x09\x41\x3B\x0A\x74\x54", "xxxxxxx");
            if (!addr) {
                logger.Write(ERROR, "Couldn't find GetModelInfo (v58+)");
            }
            addr = addr - 0x2C;
        }

        GetModelInfo = reinterpret_cast<uintptr_t(*)(unsigned int modelHash, int* index)>(addr);
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

    std::vector<uintptr_t> FindPatterns(const char* pattern, const char* mask) {
        std::vector <uintptr_t> addresses;

        MODULEINFO modInfo = { nullptr };
        GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

        const char* start_offset = reinterpret_cast<const char *>(modInfo.lpBaseOfDll);
        const uintptr_t size = static_cast<uintptr_t>(modInfo.SizeOfImage);

        intptr_t pos = 0;
        const uintptr_t searchLen = static_cast<uintptr_t>(strlen(mask) - 1);

        for (const char* retAddress = start_offset; retAddress < start_offset + size; retAddress++) {
            if (*retAddress == pattern[pos] || mask[pos] == '?') {
                if (mask[pos + 1] == '\0')
                    addresses.push_back(reinterpret_cast<uintptr_t>(retAddress) - searchLen);
                pos++;
            }
            else {
                pos = 0;
            }
        }
        return addresses;
    }

uintptr_t FindPattern(const char* pattStr) {
    std::vector<std::string> bytesStr = split(pattStr, ' ');

    MODULEINFO modInfo{};
    GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

    auto* start_offset = static_cast<uint8_t*>(modInfo.lpBaseOfDll);
    const auto size = static_cast<uintptr_t>(modInfo.SizeOfImage);

    uintptr_t pos = 0;
    const uintptr_t searchLen = bytesStr.size();

    for (auto* retAddress = start_offset; retAddress < start_offset + size; retAddress++) {
        if (bytesStr[pos] == "??" || bytesStr[pos] == "?" || 
            *retAddress == static_cast<uint8_t>(std::strtoul(bytesStr[pos].c_str(), nullptr, 16))) {
            if (pos + 1 == bytesStr.size())
                return (reinterpret_cast<uintptr_t>(retAddress) - searchLen + 1);
            pos++;
        }
        else {
            pos = 0;
        }
    }
    return 0;
}
}
