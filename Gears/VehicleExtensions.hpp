#pragma once

#include <cstdint>
#include "..\..\ScriptHookV_SDK\inc\types.h"
#include "NativeMemory.hpp"

class VehicleExtensions {
public:
	// Clutch disengage @ Low Speed High Gear 
	uintptr_t PatchClutchLow();
	void RestoreClutchLow(uintptr_t address);

	// Clutch 0.1 @ Stationary
	uintptr_t PatchClutchStationary01();
	void RestoreClutchStationary01(uintptr_t address);

	// Clutch 0.4 @ Stationary
	uintptr_t PatchClutchStationary04();
	void RestoreClutchStationary04(uintptr_t address);

	// Throttle disengage at redline
	uintptr_t PatchThrottleRedline();
	void RestoreThrottleRedline(uintptr_t address);

	// Clutch small value @ full press
	uintptr_t PatchClutchStationaryLow();
	void RestoreClutchStationaryLow(uintptr_t address);

	// Keep Clutch disengage at redline

	uint64_t GetAddress(Vehicle handle);
	uint32_t GetGears(Vehicle handle);
	void SetGears(Vehicle handle, uint32_t value);
	uint32_t GetTopGear(Vehicle handle);
	float GetCurrentRPM(Vehicle handle);
	void SetCurrentRPM(Vehicle handle, float value);
	float GetClutch(Vehicle handle);
	void SetClutch(Vehicle handle, float value);
	float GetTurbo(Vehicle handle);
	void SetTurbo(Vehicle handle, float value);
	float GetThrottle(Vehicle handle);
	void SetThrottle(Vehicle handle, float value);
	float GetThrottleP(Vehicle handle);
	void SetThrottleP(Vehicle handle, float value);
	float GetBrakeP(Vehicle handle);
	void SetBrakeP(Vehicle handle, float value);
	float GetFuelLevel(Vehicle handle);
	void SetFuelLevel(Vehicle handle, float value);
	uint64_t GetWheelsPtr(Vehicle handle);
	uint64_t GetWheelPtr(uint64_t address, int index);
	void SetWheelsHealth(Vehicle handle, float health);
private:
	MemoryAccess mem;
};

