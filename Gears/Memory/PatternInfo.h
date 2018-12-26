#pragma once
#include <vector>
namespace MemoryPatcher {
struct PatternInfo {
    PatternInfo() : Pattern(nullptr), Mask(nullptr) {}

    PatternInfo(const char* pattern, const char* mask, std::vector<uint8_t> data)
        : Pattern(pattern), Mask(mask), Data(std::move(data)) { }
    PatternInfo(const char* pattern, const char* mask, std::vector<uint8_t> data, uint32_t offset)
        : Pattern(pattern), Mask(mask), Data(std::move(data)), Offset(offset) { }
    const char* Pattern;
    const char* Mask;
    std::vector<uint8_t> Data;
    uint32_t Offset = 0;
};
}
