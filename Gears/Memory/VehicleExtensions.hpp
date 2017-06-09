#pragma once

/*
 * Probably consider ReSharper-ing all of these so I can just
 * have a bunch of pointers to classes depending on version, instead of
 * resolving versions like this right now.
 * 
 * Wouldn't work for anything but simple get/setters though, and these
 * stl containers are just too nice.
 */

#include <vector>
#include <cstdint>
#include "../../../ScriptHookV_SDK/inc/types.h"
#include "../../../ScriptHookV_SDK/inc/nativeCaller.h"

typedef struct {
	float TyreRadius;
	float RimRadius;
	float TyreWidth;
} WheelDimensions;

class VehicleExtensions {
public:
	void ClearAddress();
	BYTE *GetAddress(Vehicle handle);

	/*
	 * Vehicle struct
	 */
	uint32_t GetGears(Vehicle handle);
	void SetGears(Vehicle handle, uint32_t value);

	uint16_t GetGearCurr(Vehicle handle);
	void SetGearCurr(Vehicle handle, uint16_t value);
	
	uint16_t GetGearNext(Vehicle handle);
	void SetGearNext(Vehicle handle, uint16_t value);
	
	uint32_t GetTopGear(Vehicle handle);
	// SetTopGear not logical
	
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
	// SetHandbrake not implemented
	
	float GetFuelLevel(Vehicle handle);
	void SetFuelLevel(Vehicle handle, float value);

	float GetEngineTemp(Vehicle handle);
	// No set impl.

	float GetDirtLevel(Vehicle handle);
	// No set impl.

	float GetDashSpeed(Vehicle handle);
	// No set impl.

	float GetSteeringInputAngle(Vehicle handle);
	void SetSteeringInputAngle(Vehicle handle, float value);

	float GetSteeringAngle(Vehicle handle);
	void SetSteeringAngle(Vehicle handle, float value);

	uint8_t GetNumWheels(Vehicle handle);
	uint64_t GetWheelsPtr(Vehicle handle);
	
	/*
	* Suspension info struct
	*/
	
	std::vector<uint64_t> GetWheelPtrs(Vehicle handle);
	
	float GetVisualHeight(Vehicle handle);
	void SetVisualHeight(Vehicle handle, float height);

	/*
	 * Wheel struct
	 */
	std::vector<float> GetWheelHealths(Vehicle handle);					// 0-1000
	void SetWheelsHealth(Vehicle handle, float health);					// 0-1000
	
	float GetSteeringMultiplier(Vehicle handle);
	void SetSteeringMultiplier(Vehicle handle, float value);

	std::vector<float> GetWheelCompressions(Vehicle handle);
	// Unit: rad/s
	std::vector<float> GetWheelRotationSpeeds(Vehicle handle);			
	std::vector<Vector3> GetWheelLastContactCoords(Vehicle handle);		
	// Unit: meters, probably.
	std::vector<WheelDimensions> GetWheelDimensions(Vehicle handle);	

	// Unit: m/s, at the tyres. This probably doesn't work well for popped tyres.
	std::vector<float> GetTyreSpeeds(Vehicle handle);

private:
	BYTE* currAddress = nullptr;
	eGameVersion gameVersion = getGameVersion();
};
