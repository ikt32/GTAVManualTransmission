#pragma once
#include "Patcher.h"

namespace MemoryPatcher {
void SetPatterns(int version);
bool Test();

/*
 * Patch clutch and gearbox behavior so they can be script-controlled
 * Changes multiple things.
 */
bool ApplyGearboxPatches();
bool RevertGearboxPatches();

/*
 * Remove steering correction and steering scaling at speed
 */
bool PatchSteeringAssist();
bool RestoreSteeringAssist();

/*
 * Disable keyboard and controller from steer input, only wheel control
 */
bool PatchSteeringControl();
bool RestoreSteeringControl();

/*
 * Remove brake pressure drop, more script control over brakes
 */
bool PatchBrake();
bool RestoreBrake();

/*
* Remove brake pressure drop, more script control over brakes
*/
bool PatchThrottle();
bool RestoreThrottle();
bool RestoreThrottleControl();

extern bool Error;

extern const int NumGearboxPatches;
extern int NumGearboxPatched;

extern Patcher ThrottlePatcher;
extern Patcher BrakePatcher;
extern PatcherJmp SteeringAssistPatcher;
extern Patcher SteeringControlPatcher;
extern Patcher ThrottleControlPatcher;
}
