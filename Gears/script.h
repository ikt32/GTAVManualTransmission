#pragma once
#include <string>
#include <vector>

#include <inc/types.h>

void update_npc();

///////////////////////////////////////////////////////////////////////////////
//                           Helper functions/tools
///////////////////////////////////////////////////////////////////////////////

void drawHUD();
void drawDebugInfo();
void crossScript();
void drawVehicleWheelInfo();
void drawInputWheelInfo();

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Mod control
///////////////////////////////////////////////////////////////////////////////

void resetSteeringMultiplier();
void toggleManual();
void initWheel();
void stopForceFeedback();
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
void functionEngLock();
//void manageBrakePatch();

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Gearbox control
///////////////////////////////////////////////////////////////////////////////

void fakeRev(bool customThrottle = false, float customThrottleVal = 0.0f);
void handleRPM();
//void functionTruckLimiting();
void functionLimiter();

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

void checkWheelButtons();

///////////////////////////////////////////////////////////////////////////////
//                    Wheel input and force feedback
///////////////////////////////////////////////////////////////////////////////

void playFFBGround();
void playFFBWater();
void doWheelSteering();

///////////////////////////////////////////////////////////////////////////////
//                             Misc features
///////////////////////////////////////////////////////////////////////////////

void functionAutoLookback();
void functionAutoGear1();
void functionHillGravity();
void functionHidePlayerInFPV();

///////////////////////////////////////////////////////////////////////////////
//                              Script entry
///////////////////////////////////////////////////////////////////////////////
void readSettings();
void ScriptMain();
void NPCMain();


///////////////////////////////////////////////////////////////////////////////
//                              Menu-related
///////////////////////////////////////////////////////////////////////////////
void update_menu();
void clearAxis(const std::string& axis);
void clearButton(const std::string& button);
void clearWheelToKey();
void clearHShifter();
void clearASelect();
void clearKeyboardKey(const std::string& button);
void clearControllerButton(const std::string& button);
void clearLControllerButton(const std::string& button);

bool configAxis(const std::string& confTag);
bool configWheelToKey();
bool configButton(const std::string& confTag);
bool configHPattern();
bool configASelect();
bool configKeyboardKey(const std::string &confTag);
bool configControllerButton(const std::string &confTag);
bool configLControllerButton(const std::string &confTag);

void updateSteeringMultiplier();

void onMenuInit();
void onMenuClose();

///////////////////////////////////////////////////////////////////////////////
//                        Script-specific utils
///////////////////////////////////////////////////////////////////////////////
std::vector<float> getDrivenWheelsSpeeds(std::vector<float> wheelSpeeds);


void     blockButtons();
void startStopEngine();

void drawSteeringLines(float steeringAngle);
float getSteeringAngle(Vehicle v);

bool isPlayerAvailable(Player player, Ped playerPed);

bool isVehicleAvailable(Vehicle vehicle, Ped playerPed);
