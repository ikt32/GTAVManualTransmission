#pragma once
#include <vector>
namespace MemoryPatcher {
struct PatternInfo {
    PatternInfo() : Pattern(nullptr), Mask(nullptr) {}

    // data: provides memory to copy the original bytes to, gets overwritten on Patcher::Apply()
    // data.size() is used as length to NOP the found stuff with
    PatternInfo(const char* pattern, const char* mask, std::vector<uint8_t> data)
        : Pattern(pattern), Mask(mask), Data(std::move(data)) { }

    // offset: Use if the address of pattern is not the target to patch
    PatternInfo(const char* pattern, const char* mask, std::vector<uint8_t> data, uint32_t offset)
        : Pattern(pattern), Mask(mask), Data(std::move(data)), Offset(offset) { }
    const char* Pattern;
    const char* Mask;
    std::vector<uint8_t> Data;
    uint32_t Offset = 0;
};
}
