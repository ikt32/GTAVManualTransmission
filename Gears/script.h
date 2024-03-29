#pragma once
#include "ScriptSettings.hpp"

void threadCheckUpdate(unsigned milliseconds);

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Mod control
///////////////////////////////////////////////////////////////////////////////

void resetSteeringMultiplier();
void toggleManual(bool enable);
void initWheel();

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Shifting
///////////////////////////////////////////////////////////////////////////////

void setShiftMode(EShiftMode shiftMode);
void shiftTo(int gear, bool autoClutch);

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Gearbox control
///////////////////////////////////////////////////////////////////////////////

void fakeRev(bool customThrottle = false, float customThrottleVal = 0.0f);
bool isSkidding(float threshold);

///////////////////////////////////////////////////////////////////////////////
//                              Script entry
///////////////////////////////////////////////////////////////////////////////
void loadConfigs(); // Vehicle override configs
void readSettings();
void ScriptMain();
void NPCMain();
void initTimers();
void saveAllSettings();

///////////////////////////////////////////////////////////////////////////////
//                              Menu-related
///////////////////////////////////////////////////////////////////////////////
void update_menu();

void onMenuInit();
void onMenuClose();

///////////////////////////////////////////////////////////////////////////////
//                             Misc features
///////////////////////////////////////////////////////////////////////////////
void functionHidePlayerInFPV(bool optionToggled);
void StartUDPTelemetry();
