#pragma once
#include "ScriptSettings.hpp"
void threadCheckUpdate(unsigned milliseconds);

///////////////////////////////////////////////////////////////////////////////
//                           Helper functions/tools
///////////////////////////////////////////////////////////////////////////////

void drawHUD();
void drawDebugInfo();
void drawGForces();
void drawVehicleWheelInfo();
void drawInputWheelInfo();
void drawWarningLights();

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
bool isClutchPressed();

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Gearbox control
///////////////////////////////////////////////////////////////////////////////

void fakeRev(bool customThrottle = false, float customThrottleVal = 0.0f);

///////////////////////////////////////////////////////////////////////////////
//                              Script entry
///////////////////////////////////////////////////////////////////////////////
void loadConfigs(); // Vehicle override configs
void readSettings();
void ScriptMain();
void NPCMain();
void initTimers();

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
