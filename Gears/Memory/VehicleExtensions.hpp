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

	// Steering input angle, steering lock independent
	float GetSteeringInputAngle(Vehicle handle);
	void SetSteeringInputAngle(Vehicle handle, float value);

	// Wheel angle, steering lock dependent
	float GetSteeringAngle(Vehicle handle);
	void SetSteeringAngle(Vehicle handle, float value);

	uint8_t GetNumWheels(Vehicle handle);
	uint64_t GetWheelsPtr(Vehicle handle);
	
	/*
	 * Suspension info struct
	 */
	
	std::vector<uint64_t> GetWheelPtrs(Vehicle handle);
	
	// 0 is default. Pos is lowered, Neg = change height. Used by LSC. Set once.
	// Physics are NOT affected, including hitbox.
	float GetVisualHeight(Vehicle handle);
	void SetVisualHeight(Vehicle handle, float height);

	/*
	 * Wheel struct
	 */	
	std::vector<float> GetWheelHealths(Vehicle handle);
	void SetWheelsHealth(Vehicle handle, float health);
	
	float GetSteeringMultiplier(Vehicle handle);
	void SetSteeringMultiplier(Vehicle handle, float value);

	std::vector<Vector3> GetWheelOffsets(Vehicle handle);
	std::vector<Vector3> GetWheelCoords(Vehicle handle, Vector3 base, Vector3 rotation, Vector3 direction);
	std::vector<Vector3> GetWheelLastContactCoords(Vehicle handle);
	std::vector<float> GetWheelCompressions(Vehicle handle);
	std::vector<bool> GetWheelsOnGround(Vehicle handle);

	// Unit: meters, probably.
	std::vector<WheelDimensions> GetWheelDimensions(Vehicle handle);
	// Unit: rad/s
	std::vector<float> GetWheelRotationSpeeds(Vehicle handle);
	// Unit: m/s, at the tyres. This probably doesn't work well for popped tyres.
	std::vector<float> GetTyreSpeeds(Vehicle handle);

private:
	BYTE* currAddress = nullptr;
	eGameVersion gameVersion = getGameVersion();
};

// Wheel stuff:
// 20: offset from body?
// 30: Similar-ish?

// 40, 50: last position on ground?
// 60 - current position? seems to be 0,0,0 when contact is lost

// B0 - world velocity
// C0 - Same, but flipped?

// 168 - rotation speed rad/s
// 16C - slippage y-dir?
// 170 - heating

// materials.dat related
// 190 - surface related: tyre grip
// 194 - surface related: wet grip
// 198 - surface related: tyre drag
// 19C - surface related: top speed mult


// 1C4 - steering angle
// 1C8/1D4 - Brake
// 1CC/1D8 - Throttle/powered
// 1E0/1E4 - Health

// 1EC / (flags?) - Powered states

// ulong offPosX = 0x40;
// ulong offPosY = 0x44;
// ulong offPosZ = 0x48;
