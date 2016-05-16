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

#include "..\..\LogitechSteeringWheel_SDK\Include\LogitechSteeringWheelLib.h"

void ScriptMain();

void functionAutoReverse();
void functionRealReverse();
void functionSimpleReverse();
void functionTruckLimiting();
void functionEngBrake();
void functionEngDamage();
void functionEngStall();
void functionClutchCatch();
void functionHShift();
void functionSShift();
void functionHShiftLogitech();
void shiftTo(int gear);
void handleRPM();