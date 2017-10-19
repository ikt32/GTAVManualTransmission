#pragma once

/*
 * Probably consider ReSharper-ing all of these so I can just
 * have a bunch of pointers to classes depending on version, instead of
 * resolving versions like this right now.
 * 
 * Wouldn't work for anything but simple get/setters though, and these
 * stl containers are just too nice.
 * 
 * Update 1103.2 - I did not realize manually updating the 
 * offsets was such a pain in the ass.
 */

#include <vector>
#include <cstdint>
#include "../../../ScriptHookV_SDK/inc/types.h"
#include "../../../ScriptHookV_SDK/inc/nativeCaller.h"

struct WheelDimensions {
    float TyreRadius;
    float RimRadius;
    float TyreWidth;
};

class VehicleExtensions {
public:
    VehicleExtensions();
    BYTE *GetAddress(Vehicle handle);

    /*
     * Vehicle struct
     */
    bool GetRocketBoostActive(Vehicle handle);
    void SetRocketBoostActive(Vehicle handle, bool val);

    float GetRocketBoostCharge(Vehicle handle);
    void SetRocketBoostCharge(Vehicle handle, float value);

    uint32_t GetGears(Vehicle handle);
    void SetGears(Vehicle handle, uint32_t value);

    uint16_t GetGearCurr(Vehicle handle);
    void SetGearCurr(Vehicle handle, uint16_t value);
    
    uint16_t GetGearNext(Vehicle handle);
    void SetGearNext(Vehicle handle, uint16_t value);

    unsigned char GetTopGear(Vehicle handle);
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

    // Handling data related getters
    uint64_t GetHandlingPtr(Vehicle handle);
    float GetDriveBiasFront(Vehicle handle);
    float GetDriveBiasRear(Vehicle handle);
    float GetPetrolTankVolume(Vehicle handle);
    float GetOilVolume(Vehicle handle);

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
    std::vector<float> GetWheelSteeringAngles(Vehicle handle);
    std::vector<bool> GetWheelsOnGround(Vehicle handle);

    // Unit: meters, probably.
    std::vector<WheelDimensions> GetWheelDimensions(Vehicle handle);
    // Unit: rad/s
    std::vector<float> GetWheelRotationSpeeds(Vehicle handle);
    // Unit: m/s, at the tyres. This probably doesn't work well for popped tyres.
    std::vector<float> GetTyreSpeeds(Vehicle handle);

    // How much smoke and skidmarks the wheels/tires are generating.
    std::vector<float> GetWheelSkidSmokeEffect(Vehicle handle);
    void SetWheelSkidSmokeEffect(Vehicle handle, uint8_t index, float speed);

    // Strangely, braking/pulling the handbrake just adds to this value.
    // This behavior is visible when the instruction decreasing this is patched away.
    // Needs patching of the instruction manipulating this field for applied values
    // to stick properly.
    void SetWheelBrakePressure(Vehicle handle, uint8_t index, float value);

    // Divide GetDriveMaxFlatVel by the values in this thing to get the top
    // speed for the gear.
    std::vector<float> GetGearRatios(Vehicle handle);

    float GetDriveForce(Vehicle handle);

    float GetInitialDriveMaxFlatVel(Vehicle handle);

    float GetDriveMaxFlatVel(Vehicle handle);

private:
    eGameVersion gameVersion = getGameVersion();
};
