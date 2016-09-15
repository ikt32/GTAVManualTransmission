/*
THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
http://dev-c.com
(C) Alexander Blade 2015
*/

#pragma once

#include "../../ScriptHookV_SDK/inc/types.h"

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif

class ScriptSettings;
void ScriptMain();

void crossScriptComms();

void showText(float x, float y, float scale, const char* text);
void showNotification(char* message);
void showDebugInfo();
void reInit();
void toggleManual();
void reset();

void updateLastInputDevice();


void handlePedalsDefault(float logiThrottleVal, float logiBrakeVal);
void handlePedalsRealReverse(float logiThrottleVal, float logiBrakeVal);
void handleVehicleButtons();
void playWheelEffects(
	float speed,
	Vector3 accelVals,
	Vector3 accelValsAvg,
	ScriptSettings& settings,
	bool airborne);
void doWheelSteering();

void functionAutoReverse();
void functionRealReverse();
void functionSimpleReverse();
void functionTruckLimiting();
void functionEngBrake();
void functionEngDamage();
void functionEngStall();
void functionClutchCatch();
void functionHShiftTo(int i);
void functionHShiftKeyboard();
void functionSShift();
void functionHShiftWheel();

void shiftTo(int gear, bool autoClutch);
void handleRPM();
