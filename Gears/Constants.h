#pragma once
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define VERSION_MAJOR 5
#define VERSION_MINOR 5
#define VERSION_PATCH 0
#define VERSION_SUFFIX ""

#define VERSION_DISPLAY STR(VERSION_MAJOR) "."  STR(VERSION_MINOR) "." STR(VERSION_PATCH) VERSION_SUFFIX

namespace Constants {
    static const char* const DisplayVersion = "v" VERSION_DISPLAY;
    static const char* const iktDir = "ikt";
    static const char* const ModDir = "ManualTransmission";
    static const char* const NotificationPrefix =  "~b~Manual Transmission~w~";
}
