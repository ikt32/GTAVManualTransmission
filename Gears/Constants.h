#pragma once
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define VERSION_MAJOR 4
#define VERSION_MINOR 8
#define VERSION_PATCH 2

namespace Constants {
    static const char* const DisplayVersion = "v" STR(VERSION_MAJOR) "."  STR(VERSION_MINOR) "." STR(VERSION_PATCH);
    static const char* const ModDir = "\\ManualTransmission";
    static const char* const NotificationPrefix =  "~b~Manual Transmission~w~";
}
