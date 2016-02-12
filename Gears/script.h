/*
THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
http://dev-c.com
(C) Alexander Blade 2015
*/

#pragma once

#include "..\..\ScriptHookV_SDK\inc\natives.h"
#include "..\..\ScriptHookV_SDK\inc\types.h"
#include "..\..\ScriptHookV_SDK\inc\enums.h"

#include "..\..\ScriptHookV_SDK\inc\main.h"

#include <fstream>
#include <Windows.h>

#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include "VehicleExtensions.hpp"

extern VehExt::VehicleExtensions ext;

extern uintptr_t clutchInstrLowAddr;
extern uintptr_t clutchInstrLowTemp;

extern uintptr_t clutchS01Addr;
extern uintptr_t clutchS01Temp;

extern uintptr_t clutchS04Addr;
extern uintptr_t clutchS04Temp;

void ScriptMain();

extern void clearLog();

extern void writeToLog(const std::string &text);

extern void patchClutchInstructions();

extern bool restoreClutchInstructions();
