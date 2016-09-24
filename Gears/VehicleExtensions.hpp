#pragma once

#include <cstdint>
#include "../../ScriptHookV_SDK/inc/types.h"
#include "NativeMemory.hpp"
#include <vector>

class VehicleExtensions {
public:
	uint64_t GetAddress(Vehicle handle) const;
	uint16_t GetGearCurr(Vehicle handle) const;
	uint16_t GetGearNext(Vehicle handle) const;
	uint32_t GetGears(Vehicle handle) const;
	void SetGears(Vehicle handle, uint32_t value) const;
	void SetGearCurr(Vehicle handle, uint16_t value) const;
	void SetGearNext(Vehicle handle, uint16_t value) const;
	uint32_t GetTopGear(Vehicle handle) const;
	float GetCurrentRPM(Vehicle handle) const;
	void SetCurrentRPM(Vehicle handle, float value) const;
	float GetClutch(Vehicle handle) const;
	void SetClutch(Vehicle handle, float value) const;
	float GetTurbo(Vehicle handle) const;
	void SetTurbo(Vehicle handle, float value) const;
	float GetThrottle(Vehicle handle) const;
	void SetThrottle(Vehicle handle, float value) const;
	float GetThrottleP(Vehicle handle) const;
	void SetThrottleP(Vehicle handle, float value) const;
	float GetBrakeP(Vehicle handle) const;
	void SetBrakeP(Vehicle handle, float value) const;
	float GetFuelLevel(Vehicle handle) const;
	void SetFuelLevel(Vehicle handle, float value) const;
	uint64_t GetWheelsPtr(Vehicle handle) const;
	static uint64_t GetWheelPtr(uint64_t address, int index);
	void SetWheelsHealth(Vehicle handle, float health) const;
	std::vector<float> GetWheelsCompression(Vehicle handle) const;
	float GetSteeringInputAngle(Vehicle handle) const;
	void SetSteeringInputAngle(Vehicle handle, float value) const;
	float GetSteeringAngle(Vehicle handle) const;
	void SetSteeringAngle(Vehicle handle, float value) const;

private:
	MemoryAccess mem;
};
