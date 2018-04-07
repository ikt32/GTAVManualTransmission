#pragma once

namespace MemoryPatcher {
void SetPatterns(int version);

/*
 * Patch clutch and gearbox behavior so they can be script-controlled
 * Changes multiple things.
 */
bool ApplyGearboxPatches();
bool RevertGearboxPatches();

/*
 * Remove steering correction and steering scaling at speed
 */
bool PatchSteeringCorrection();
bool RestoreSteeringCorrection();

/*
 * Disable keyboard and controller from steer input, only wheel control
 */
bool PatchSteeringControl();
bool RestoreSteeringControl();

/*
 * Remove brake pressure drop, more script control over brakes
 */
bool PatchBrakeDecrement();
bool RestoreBrakeDecrement();

/*
* Remove brake pressure drop, more script control over brakes
*/
bool PatchThrottleDecrement();
bool RestoreThrottleDecrement();

/*
 * "total" refers to the "package" of patches needed to get the gearbox
 * and clutch stuff working.
 */
extern int NumGearboxPatches;
extern int TotalPatched;

extern bool SteerCorrectPatched;
extern bool SteerControlPatched;
extern bool BrakeDecrementPatched;
extern bool ThrottleDecrementPatched;
}
