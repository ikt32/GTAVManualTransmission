#pragma once

#include <cstdint>
#include "../../../ScriptHookV_SDK/inc/types.h"
#include "NativeMemory.hpp"
#include <vector>

class VehicleExtensions {
public:
	static uint64_t GetAddress(Vehicle handle);
	static uint16_t GetGearCurr(Vehicle handle);
	static uint16_t GetGearNext(Vehicle handle);
	static uint32_t GetGears(Vehicle handle);
	static void SetGears(Vehicle handle, uint32_t value);
	static void SetGearCurr(Vehicle handle, uint16_t value);
	static void SetGearNext(Vehicle handle, uint16_t value);
	static uint32_t GetTopGear(Vehicle handle);
	static float GetCurrentRPM(Vehicle handle);
	static void SetCurrentRPM(Vehicle handle, float value);
	static float GetClutch(Vehicle handle);
	static void SetClutch(Vehicle handle, float value);
	static float GetTurbo(Vehicle handle);
	static void SetTurbo(Vehicle handle, float value);
	static float GetThrottle(Vehicle handle);
	static void SetThrottle(Vehicle handle, float value);
	static float GetThrottleP(Vehicle handle);
	static void SetThrottleP(Vehicle handle, float value);
	static float GetBrakeP(Vehicle handle);
	static void SetBrakeP(Vehicle handle, float value);
	static float GetFuelLevel(Vehicle handle);
	static void SetFuelLevel(Vehicle handle, float value);
	static uint64_t GetWheelsPtr(Vehicle handle);
	static void SetWheelsHealth(Vehicle handle, float health);
	static std::vector<float> GetWheelsCompression(Vehicle handle);
	static float GetSteeringInputAngle(Vehicle handle);
	static void SetSteeringInputAngle(Vehicle handle, float value);
	static float GetSteeringAngle(Vehicle handle);
	static void SetSteeringAngle(Vehicle handle, float value);

private:
	MemoryAccess mem;
};
