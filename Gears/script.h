/*
THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
http://dev-c.com
(C) Alexander Blade 2015
*/

#pragma once

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif

#include <string>
const std::string mtDir = "\\ManualTransmission";

class VehicleData;
struct Color;
class ScriptSettings;

//http://pastebin.com/Kj9t38KF
enum eRadioStations {
	LosSantosRockRadio,
	NonStopPopFM,
	RadioLosSantos,
	ChannelX,
	WestCoastTalkRadio,
	RebelRadio,
	SoulwaxFM,
	EastLosFM,
	WestCoastClassics,
	BlueArk,
	WorldideFM,
	FlyLoFM,
	TheLowdown,
	TheLab,
	RadioMirrorPark,
	Space1032,
	VinewoodBlvdRadio,
	SelfRadio,
	BlaineCountyRadio,
	RadioOff = 255
};

enum ShiftModes {
	Sequential = 0,
	HPattern = 1,
	Automatic = 2
};

///////////////////////////////////////////////////////////////////////////////
//                           Helper functions/tools
///////////////////////////////////////////////////////////////////////////////

void showHUD();
void showDebugInfo();
void crossScriptComms();
void crossScriptUpdated();
void showWheelInfo();
void drawSteeringWheelInfo();

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Mod control
///////////////////////////////////////////////////////////////////////////////

void reInit();
void reset();
void applySteeringMultiplier();
void resetSteeringMultiplier();
void toggleManual();
void initWheel();
void initSteeringPatches();
void updateLastInputDevice();

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Shifting
///////////////////////////////////////////////////////////////////////////////

void setShiftMode(int shiftMode);
void cycleShiftMode();
void shiftTo(int gear, bool autoClutch);
void functionHShiftTo(int i);
void functionHShiftKeyboard();
void functionHShiftWheel();
void functionSShift();
void functionAShift();

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Gearbox features
///////////////////////////////////////////////////////////////////////////////

void functionClutchCatch();
void functionEngStall();
void functionEngDamage();
void functionEngBrake();

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Gearbox control
///////////////////////////////////////////////////////////////////////////////

void fakeRev();
void handleRPM();
void functionTruckLimiting();

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Reverse/Pedal handling
///////////////////////////////////////////////////////////////////////////////

void functionAutoReverse();
void functionRealReverse();
void handlePedalsDefault(float logiThrottleVal, float logiBrakeVal);
void handlePedalsRealReverse(float logiThrottleVal, float logiBrakeVal);

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Buttons
///////////////////////////////////////////////////////////////////////////////

void handleVehicleButtons();

///////////////////////////////////////////////////////////////////////////////
//                    Wheel input and force feedback
///////////////////////////////////////////////////////////////////////////////

void playWheelEffects(ScriptSettings& settings,
					  VehicleData& vehData,
                      bool airborne, bool ignoreSpeed = false);
void playFFBAir(ScriptSettings& settings, VehicleData& vehData);
void doWheelSteering();
//void doWheelSteeringBoat();
void doStickControlAir();

///////////////////////////////////////////////////////////////////////////////
//                             Misc features
///////////////////////////////////////////////////////////////////////////////

void functionAutoLookback();
void functionAutoGear1();

///////////////////////////////////////////////////////////////////////////////
//                              Script entry
///////////////////////////////////////////////////////////////////////////////
void ScriptMain();



///////////////////////////////////////////////////////////////////////////////
//                              Menu-related
///////////////////////////////////////////////////////////////////////////////
void update_menu();
void clearAxis(std::string axis);
void clearButton(std::string button);
void clearWheelToKey();
void clearHShifter();
void clearKeyboardKey(std::string button);
void clearControllerButton(std::string button);
void clearLControllerButton(std::string button);

bool configAxis(std::string confTag);
bool configWheelToKey();
bool configButton(std::string confTag);
bool configHPattern();
bool configKeyboardKey(const std::string &confTag);
bool configControllerButton(const std::string &confTag);
bool configLControllerButton(const std::string &confTag);
bool configStickAxis(std::string confTag);

void updateSteeringMultiplier();

void menuInit();
void menuClose();
