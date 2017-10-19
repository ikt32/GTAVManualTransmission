#pragma once

enum ShiftModes {
    Sequential,
    HPattern,
    Automatic,
    SIZEOF_ShiftModes
};

ShiftModes& operator++(ShiftModes &mode);
ShiftModes operator++(ShiftModes &c, int);
