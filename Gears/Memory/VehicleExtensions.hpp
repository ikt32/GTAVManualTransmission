#pragma once

#include <vector>
#include <cstdint>
#include "../../../ScriptHookV_SDK/inc/types.h"
#include "../../../ScriptHookV_SDK/inc/nativeCaller.h"

class VehicleExtensions {
public:
	void ClearAddress();
	BYTE *GetAddress(Vehicle handle);
	uint16_t GetGearCurr(Vehicle handle);
	uint16_t GetGearNext(Vehicle handle);
	uint32_t GetGears(Vehicle handle);
	void SetGears(Vehicle handle, uint32_t value);
	void SetGearCurr(Vehicle handle, uint16_t value);
	void SetGearNext(Vehicle handle, uint16_t value);
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
	bool GetHandbrake(Vehicle handle);
	float GetFuelLevel(Vehicle handle);
	void SetFuelLevel(Vehicle handle, float value);
	uint8_t GetNumWheels(Vehicle handle);
	uint64_t GetWheelsPtr(Vehicle handle);
	void SetWheelsHealth(Vehicle handle, float health);
	std::vector<float> GetWheelsCompression(Vehicle handle);
	float GetSteeringInputAngle(Vehicle handle);
	void SetSteeringInputAngle(Vehicle handle, float value);
	float GetSteeringAngle(Vehicle handle);
	void SetSteeringAngle(Vehicle handle, float value);
	float GetEngineTemp(Vehicle handle);
	float GetDirtLevel(Vehicle handle);
	float GetDashSpeed(Vehicle handle);
private:
	BYTE* currAddress = nullptr;
	eGameVersion gameVersion = getGameVersion();
};
