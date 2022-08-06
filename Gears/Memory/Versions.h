#pragma once
#include <string>
#include <vector>
#include "../Util/FileVersion.h"

static std::vector<std::string> GameVersionString = {
    "VER_1_0_335_2_STEAM",      // 00
    "VER_1_0_335_2_NOSTEAM",    // 01

    "VER_1_0_350_1_STEAM",      // 02
    "VER_1_0_350_2_NOSTEAM",    // 03

    "VER_1_0_372_2_STEAM",      // 04
    "VER_1_0_372_2_NOSTEAM",    // 05

    "VER_1_0_393_2_STEAM",      // 06
    "VER_1_0_393_2_NOSTEAM",    // 07

    "VER_1_0_393_4_STEAM",      // 08
    "VER_1_0_393_4_NOSTEAM",    // 09

    "VER_1_0_463_1_STEAM",      // 10
    "VER_1_0_463_1_NOSTEAM",    // 11

    "VER_1_0_505_2_STEAM",      // 12
    "VER_1_0_505_2_NOSTEAM",    // 13

    "VER_1_0_573_1_STEAM",      // 14
    "VER_1_0_573_1_NOSTEAM",    // 15

    "VER_1_0_617_1_STEAM",      // 16
    "VER_1_0_617_1_NOSTEAM",    // 17

    "VER_1_0_678_1_STEAM",      // 18
    "VER_1_0_678_1_NOSTEAM",    // 19

    "VER_1_0_757_2_STEAM",      // 20
    "VER_1_0_757_2_NOSTEAM",    // 21

    "VER_1_0_757_4_STEAM",      // 22
    "VER_1_0_757_4_NOSTEAM",    // 23

    "VER_1_0_791_2_STEAM",      // 24
    "VER_1_0_791_2_NOSTEAM",    // 25

    "VER_1_0_877_1_STEAM",      // 26
    "VER_1_0_877_1_NOSTEAM",    // 27

    "VER_1_0_944_2_STEAM",      // 28
    "VER_1_0_944_2_NOSTEAM",    // 29

    "VER_1_0_1011_1_STEAM",     // 30
    "VER_1_0_1011_1_NOSTEAM",   // 31

    "VER_1_0_1032_1_STEAM",     // 32
    "VER_1_0_1032_1_NOSTEAM",   // 33

    "VER_1_0_1103_2_STEAM",     // 34
    "VER_1_0_1103_2_NOSTEAM",   // 35

    "VER_1_0_1180_2_STEAM",     // 36
    "VER_1_0_1180_2_NOSTEAM",   // 37

    "VER_1_0_1290_1_STEAM",     // 38
    "VER_1_0_1290_1_NOSTEAM",   // 39

    "VER_1_0_1365_1_STEAM",     // 40
    "VER_1_0_1365_1_NOSTEAM",   // 41

    "VER_1_0_1493_0_STEAM",     // 42
    "VER_1_0_1493_0_NOSTEAM",   // 43

    "VER_1_0_1493_1_STEAM",     // 44
    "VER_1_0_1493_1_NOSTEAM",   // 45
    
    "VER_1_0_1604_0_STEAM",     // 46
    "VER_1_0_1604_0_NOSTEAM",   // 47

    "VER_1_0_1604_1_STEAM",     // 48
    "VER_1_0_1604_1_NOSTEAM",   // 49

    //"VER_1_0_1734_0_STEAM",   // XX
    //"VER_1_0_1734_0_NOSTEAM", // XX

    "VER_1_0_1737_0_STEAM",     // 50
    "VER_1_0_1737_0_NOSTEAM",   // 51

    "VER_1_0_1737_6_STEAM",     // 52
    "VER_1_0_1737_6_NOSTEAM",   // 53

    "VER_1_0_1868_0_STEAM",     // 54
    "VER_1_0_1868_0_NOSTEAM",   // 55

    "VER_1_0_1868_1_STEAM",     // 56
    "VER_1_0_1868_1_NOSTEAM",   // 57

    "VER_1_0_1868_4_EGS",       // 58

    "VER_1_0_2060_0_STEAM",     // 59
    "VER_1_0_2060_0_NOSTEAM",   // 60
    
    "VER_1_0_2060_1_STEAM",     // 61
    "VER_1_0_2060_1_NOSTEAM",   // 62

    "VER_1_0_2189_0_STEAM",     // 63
    "VER_1_0_2189_0_NOSTEAM",   // 64

    "VER_1_0_2215_0_STEAM",     // 65
    "VER_1_0_2215_0_NOSTEAM",   // 66

    "VER_1_0_2245_0_STEAM",     // 67
    "VER_1_0_2245_0_NOSTEAM",   // 68

    "VER_1_0_2372_0_STEAM",     // 69
    "VER_1_0_2372_0_NOSTEAM",   // 70

    "VER_1_0_2545_0_STEAM",     // 71
    "VER_1_0_2545_0_NOSTEAM",   // 72
};

enum G_GameVersion : int {
    G_VER_1_0_335_2_STEAM,      // 00
    G_VER_1_0_335_2_NOSTEAM,    // 01

    G_VER_1_0_350_1_STEAM,      // 02
    G_VER_1_0_350_2_NOSTEAM,    // 03

    G_VER_1_0_372_2_STEAM,      // 04
    G_VER_1_0_372_2_NOSTEAM,    // 05

    G_VER_1_0_393_2_STEAM,      // 06
    G_VER_1_0_393_2_NOSTEAM,    // 07

    G_VER_1_0_393_4_STEAM,      // 08
    G_VER_1_0_393_4_NOSTEAM,    // 09

    G_VER_1_0_463_1_STEAM,      // 10
    G_VER_1_0_463_1_NOSTEAM,    // 11

    G_VER_1_0_505_2_STEAM,      // 12
    G_VER_1_0_505_2_NOSTEAM,    // 13

    G_VER_1_0_573_1_STEAM,      // 14
    G_VER_1_0_573_1_NOSTEAM,    // 15

    G_VER_1_0_617_1_STEAM,      // 16
    G_VER_1_0_617_1_NOSTEAM,    // 17

    G_VER_1_0_678_1_STEAM,      // 18
    G_VER_1_0_678_1_NOSTEAM,    // 19

    G_VER_1_0_757_2_STEAM,      // 20
    G_VER_1_0_757_2_NOSTEAM,    // 21

    G_VER_1_0_757_4_STEAM,      // 22
    G_VER_1_0_757_4_NOSTEAM,    // 23

    G_VER_1_0_791_2_STEAM,      // 24
    G_VER_1_0_791_2_NOSTEAM,    // 25

    G_VER_1_0_877_1_STEAM,      // 26
    G_VER_1_0_877_1_NOSTEAM,    // 27

    G_VER_1_0_944_2_STEAM,      // 28
    G_VER_1_0_944_2_NOSTEAM,    // 29

    G_VER_1_0_1011_1_STEAM,     // 30
    G_VER_1_0_1011_1_NOSTEAM,   // 31

    G_VER_1_0_1032_1_STEAM,     // 32
    G_VER_1_0_1032_1_NOSTEAM,   // 33

    G_VER_1_0_1103_2_STEAM,     // 34
    G_VER_1_0_1103_2_NOSTEAM,   // 35

    G_VER_1_0_1180_2_STEAM,     // 36
    G_VER_1_0_1180_2_NOSTEAM,   // 37

    G_VER_1_0_1290_1_STEAM,     // 38
    G_VER_1_0_1290_1_NOSTEAM,   // 39

    G_VER_1_0_1365_1_STEAM,     // 40
    G_VER_1_0_1365_1_NOSTEAM,   // 41

    G_VER_1_0_1493_0_STEAM,     // 42
    G_VER_1_0_1493_0_NOSTEAM,   // 43

    G_VER_1_0_1493_1_STEAM,     // 44
    G_VER_1_0_1493_1_NOSTEAM,   // 45

    G_VER_1_0_1604_0_STEAM,     // 46
    G_VER_1_0_1604_0_NOSTEAM,   // 47

    G_VER_1_0_1604_1_STEAM,     // 48
    G_VER_1_0_1604_1_NOSTEAM,   // 49

    //G_VER_1_0_1734_0_STEAM,   // XX
    //G_VER_1_0_1734_0_NOSTEAM, // XX
    
    G_VER_1_0_1737_0_STEAM,     // 50
    G_VER_1_0_1737_0_NOSTEAM,   // 51

    G_VER_1_0_1737_6_STEAM,     // 52
    G_VER_1_0_1737_6_NOSTEAM,   // 53

    G_VER_1_0_1868_0_STEAM,     // 54
    G_VER_1_0_1868_0_NOSTEAM,   // 55

    G_VER_1_0_1868_1_STEAM,     // 56
    G_VER_1_0_1868_1_NOSTEAM,   // 57

    G_VER_1_0_1868_4_EGS,       // 58

    G_VER_1_0_2060_0_STEAM,     // 59
    G_VER_1_0_2060_0_NOSTEAM,   // 60

    G_VER_1_0_2060_1_STEAM,     // 61
    G_VER_1_0_2060_1_NOSTEAM,   // 62

    G_VER_1_0_2189_0_STEAM,     // 63
    G_VER_1_0_2189_0_NOSTEAM,   // 64

    G_VER_1_0_2215_0_STEAM,     // 65
    G_VER_1_0_2215_0_NOSTEAM,   // 66

    G_VER_1_0_2245_0_STEAM,     // 67
    G_VER_1_0_2245_0_NOSTEAM,   // 68

    G_VER_1_0_2372_0_STEAM,     // 69
    G_VER_1_0_2372_0_NOSTEAM,   // 70

    G_VER_1_0_2545_0_STEAM,     // 71
    G_VER_1_0_2545_0_NOSTEAM,   // 72
};

static std::vector<std::pair<SVersion, std::vector<int>>> ExeVersionMap = {
    { {   0, 0 },   { -1} },
    { { 335, 2 },   { G_VER_1_0_335_2_STEAM, G_VER_1_0_335_2_NOSTEAM } },
    { { 350, 1 },   { G_VER_1_0_350_1_STEAM, G_VER_1_0_350_2_NOSTEAM } },
    { { 372, 2 },   { G_VER_1_0_372_2_STEAM, G_VER_1_0_372_2_NOSTEAM } },
    { { 393, 2 },   { G_VER_1_0_393_2_STEAM, G_VER_1_0_393_2_NOSTEAM } },
    { { 393, 4 },   { G_VER_1_0_393_4_STEAM, G_VER_1_0_393_4_NOSTEAM } },
    { { 463, 1 },   { G_VER_1_0_463_1_STEAM, G_VER_1_0_463_1_NOSTEAM } },
    { { 505, 2 },   { G_VER_1_0_505_2_STEAM, G_VER_1_0_505_2_NOSTEAM } },
    { { 573, 1 },   { G_VER_1_0_573_1_STEAM, G_VER_1_0_573_1_NOSTEAM } },
    { { 617, 1 },   { G_VER_1_0_617_1_STEAM, G_VER_1_0_617_1_NOSTEAM } },
    { { 678, 1 },   { G_VER_1_0_678_1_STEAM, G_VER_1_0_678_1_NOSTEAM } },
    { { 757, 2 },   { G_VER_1_0_757_2_STEAM, G_VER_1_0_757_2_NOSTEAM } },
    { { 757, 4 },   { G_VER_1_0_757_4_STEAM, G_VER_1_0_757_4_NOSTEAM } },
    { { 791, 2 },   { G_VER_1_0_791_2_STEAM, G_VER_1_0_791_2_NOSTEAM } },
    { { 877, 1 },   { G_VER_1_0_877_1_STEAM, G_VER_1_0_877_1_NOSTEAM } },
    { { 944, 2 },   { G_VER_1_0_944_2_STEAM, G_VER_1_0_944_2_NOSTEAM } },
    { { 1011, 1 },  { G_VER_1_0_1011_1_STEAM, G_VER_1_0_1011_1_NOSTEAM } },
    { { 1032, 1 },  { G_VER_1_0_1032_1_STEAM, G_VER_1_0_1032_1_NOSTEAM } },
    { { 1103, 2 },  { G_VER_1_0_1103_2_STEAM, G_VER_1_0_1103_2_NOSTEAM } },
    { { 1180, 2 },  { G_VER_1_0_1180_2_STEAM, G_VER_1_0_1180_2_NOSTEAM } },
    { { 1290, 1 },  { G_VER_1_0_1290_1_STEAM, G_VER_1_0_1290_1_NOSTEAM } },
    { { 1365, 1 },  { G_VER_1_0_1365_1_STEAM, G_VER_1_0_1365_1_NOSTEAM } },
    { { 1493, 0 },  { G_VER_1_0_1493_0_STEAM, G_VER_1_0_1493_0_NOSTEAM } },
    { { 1493, 1 },  { G_VER_1_0_1493_1_STEAM, G_VER_1_0_1493_1_NOSTEAM } },
    { { 1604, 0 },  { G_VER_1_0_1604_0_STEAM, G_VER_1_0_1604_0_NOSTEAM } },
    { { 1604, 1 },  { G_VER_1_0_1604_1_STEAM, G_VER_1_0_1604_1_NOSTEAM } },
  //{ { 1734, 0 },  { G_VER_1_0_1734_0_STEAM, G_VER_1_0_1734_0_NOSTEAM } },
    { { 1737, 0 },  { G_VER_1_0_1737_0_STEAM, G_VER_1_0_1737_0_NOSTEAM } },
    { { 1737, 6 },  { G_VER_1_0_1737_6_STEAM, G_VER_1_0_1737_6_NOSTEAM } },
    { { 1868, 0 },  { G_VER_1_0_1868_0_STEAM, G_VER_1_0_1868_0_NOSTEAM } },
    { { 1868, 1 },  { G_VER_1_0_1868_1_STEAM, G_VER_1_0_1868_1_NOSTEAM } },
    { { 1868, 4 },  { G_VER_1_0_1868_4_EGS} },
    { { 2060, 0 },  { G_VER_1_0_2060_0_STEAM, G_VER_1_0_2060_0_NOSTEAM} },
    { { 2060, 1 },  { G_VER_1_0_2060_1_STEAM, G_VER_1_0_2060_1_NOSTEAM} },
    { { 2189, 0 },  { G_VER_1_0_2189_0_STEAM, G_VER_1_0_2189_0_NOSTEAM} },
    { { 2215, 0 },  { G_VER_1_0_2215_0_STEAM, G_VER_1_0_2215_0_NOSTEAM} },
    { { 2245, 0 },  { G_VER_1_0_2245_0_STEAM, G_VER_1_0_2245_0_NOSTEAM} },
    { { 2372, 0 },  { G_VER_1_0_2372_0_STEAM, G_VER_1_0_2372_0_NOSTEAM} },
    { { 2545, 0 },  { G_VER_1_0_2545_0_STEAM, G_VER_1_0_2545_0_NOSTEAM} },
};

static std::string eGameVersionToString(int version) {
    if (version > GameVersionString.size() - 1 || version < 0) {
        return std::to_string(version);
    }
    return GameVersionString[version];
}

template < typename K, typename V >
V findNextLowest(const std::vector<std::pair<K, V>>& map, const K& key) {
    for (auto it = map.rbegin(); it != map.rend(); ++it) {
        if (it->first <= key)
            return it->second;
    }
    return map.begin()->second;
}
