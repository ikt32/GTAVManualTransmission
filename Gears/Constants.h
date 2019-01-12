#pragma once
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define VERSION_MAJOR 4
#define VERSION_MINOR 6
#define VERSION_PATCH 6

static const char* const DISPLAY_VERSION = "v" STR(VERSION_MAJOR) "."  STR(VERSION_MINOR) "." STR(VERSION_PATCH);
static const char* const mtDir = "\\ManualTransmission";
