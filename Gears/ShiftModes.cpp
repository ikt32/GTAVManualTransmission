#include "ShiftModes.h"

#include <type_traits>

ShiftModes& operator++(ShiftModes &mode) {
    using IntType = typename std::underlying_type<ShiftModes>::type;
    mode = static_cast<ShiftModes>(static_cast<IntType>(mode) + 1);
    if (mode == ShiftModes::SIZEOF_ShiftModes)
        mode = static_cast<ShiftModes>(0);
    return mode;
}

ShiftModes operator++(ShiftModes &c, int) {
    ShiftModes result = c;
    ++c;
    return result;
}
