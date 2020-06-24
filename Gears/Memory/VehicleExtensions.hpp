#pragma once
#include <inc/types.h>
#include <vector>
#include <cstdint>

struct WheelDimensions {
    float TyreRadius;
    float RimRadius;
    float TyreWidth;
};

class VehicleExtensions {
public:
    static void ChangeVersion(int version);

    static void Init();

    static BYTE* GetAddress(Vehicle handle);

    // <  1604:  8 gears
    // >= 1604: 11 gears
    static uint8_t GearsAvailable();

    /*
     * Vehicle struct
     */
    static bool GetRocketBoostActive(Vehicle handle);
    static void SetRocketBoostActive(Vehicle handle, bool val);

    static float GetRocketBoostCharge(Vehicle handle);
    static void SetRocketBoostCharge(Vehicle handle, float value);

    static float GetHoverTransformRatio(Vehicle handle);
    static void SetHoverTransformRatio(Vehicle handle, float value);

    static float GetHoverTransformRatioLerp(Vehicle handle);
    static void SetHoverTransformRatioLerp(Vehicle handle, float value);

    static float GetFuelLevel(Vehicle handle);
    static void SetFuelLevel(Vehicle handle, float value);

    // TODO: CVeh + 0x84c (1604 - 1868) (Lights damaged)

    static uint16_t GetGearNext(Vehicle handle);
    static void SetGearNext(Vehicle handle, uint16_t value);

    static uint16_t GetGearCurr(Vehicle handle);
    static void SetGearCurr(Vehicle handle, uint16_t value);

    static uint8_t GetTopGear(Vehicle handle);
    static void SetTopGear(Vehicle handle, uint8_t value);

    // Divide GetDriveMaxFlatVel by the values in this thing to get the top
    // speed for the gear.
    static float* GetGearRatioPtr(Vehicle handle, uint8_t gear);
    static std::vector<float> GetGearRatios(Vehicle handle);
    static void SetGearRatios(Vehicle handle, const std::vector<float>& values);

    static float GetDriveForce(Vehicle handle);
    static void SetDriveForce(Vehicle handle, float value);

    static float GetInitialDriveMaxFlatVel(Vehicle handle);
    static void SetInitialDriveMaxFlatVel(Vehicle handle, float value);

    static float GetDriveMaxFlatVel(Vehicle handle);
    static void SetDriveMaxFlatVel(Vehicle handle, float value);

    static float GetCurrentRPM(Vehicle handle);
    static void SetCurrentRPM(Vehicle handle, float value);
    
    static float GetClutch(Vehicle handle);
    static void SetClutch(Vehicle handle, float value);
    
    static float GetThrottle(Vehicle handle);
    static void SetThrottle(Vehicle handle, float value);

    static float GetTurbo(Vehicle handle);
    static void SetTurbo(Vehicle handle, float value);

    static float GetArenaBoost(Vehicle handle);
    static void SetArenaBoost(Vehicle handle, float value);

    static uint64_t GetHandlingPtr(Vehicle handle);

    static uint32_t GetLightStates(Vehicle handle);
    static void SetLightStates(Vehicle handle, uint32_t value);

    // Steering input angle, steering lock independent
    static float GetSteeringInputAngle(Vehicle handle);
    static void SetSteeringInputAngle(Vehicle handle, float value);

    // Wheel angle, steering lock dependent
    static float GetSteeringAngle(Vehicle handle);
    static void SetSteeringAngle(Vehicle handle, float value);
   
    static float GetThrottleP(Vehicle handle);
    static void SetThrottleP(Vehicle handle, float value);
    
    static float GetBrakeP(Vehicle handle);
    static void SetBrakeP(Vehicle handle, float value);
    
    static bool GetHandbrake(Vehicle handle);
    static void SetHandbrake(Vehicle handle, bool value);

    static float GetDirtLevel(Vehicle handle);
    // No set impl.

    static float GetEngineTemp(Vehicle handle);
    // No set impl.

    static float GetDashSpeed(Vehicle handle);
    // No set impl.

    // 0 = car, 1 = plane, 2 = trailer, 3 = quad,
    // 6 = amphibious car, 7 = amphibious quad,
    // 8 = heli, 11 = motorcycle, 12 = bicycle, 13 = boat, 14 = train
    static int GetModelType(Vehicle handle);

    static uint64_t GetWheelsPtr(Vehicle handle);
    static uint8_t GetNumWheels(Vehicle handle);

    /*
     * Handling data related getters
     */
    static float GetDriveBiasFront(Vehicle handle);
    static float GetDriveBiasRear(Vehicle handle);
    static float GetPetrolTankVolume(Vehicle handle);
    static float GetOilVolume(Vehicle handle);
    static float GetMaxSteeringAngle(Vehicle handle);
    static Hash GetAIHandling(Vehicle handle);

    /*
     * Suspension info struct
     */
    
    static std::vector<uint64_t> GetWheelPtrs(Vehicle handle);
    
    // 0 is default. Pos is lowered, Neg = change height. Used by LSC. Set once.
    // Physics are NOT affected, including hitbox.
    static float GetVisualHeight(Vehicle handle);
    static void SetVisualHeight(Vehicle handle, float height);

    /*
     * Wheel struct
     */	
    static std::vector<float> GetWheelHealths(Vehicle handle);
    static void SetWheelsHealth(Vehicle handle, float health);
    
    static float GetSteeringMultiplier(Vehicle handle);
    static void SetSteeringMultiplier(Vehicle handle, float value);

    static std::vector<Vector3> GetWheelOffsets(Vehicle handle);
    static std::vector<Vector3> GetWheelLastContactCoords(Vehicle handle);
    static std::vector<float> GetWheelCompressions(Vehicle handle);
    static std::vector<float> GetWheelSteeringAngles(Vehicle handle);
    static std::vector<bool> GetWheelsOnGround(Vehicle handle);

    static float GetWheelLargestAngle(Vehicle handle);
    static float GetWheelAverageAngle(Vehicle handle);

    // Unit: meters, probably.
    static std::vector<WheelDimensions> GetWheelDimensions(Vehicle handle);
    // Unit: rad/s
    static std::vector<float> GetWheelRotationSpeeds(Vehicle handle);
    // For forward, use negative speed.
    static void SetWheelRotationSpeed(Vehicle handle, uint8_t index, float value);
    // Unit: m/s, at the tyres. This probably doesn't work well for popped tyres.
    static std::vector<float> GetTyreSpeeds(Vehicle handle);

    // How much smoke and skidmarks the wheels/tires are generating.
    static std::vector<float> GetWheelSkidSmokeEffect(Vehicle handle);
    static void SetWheelSkidSmokeEffect(Vehicle handle, uint8_t index, float speed);

    // Needs patching the decreasing thing
    static std::vector<float> GetWheelPower(Vehicle handle);
    static void SetWheelPower(Vehicle handle, uint8_t index, float value);

    // Strangely, braking/pulling the handbrake just adds to this value.
    // This behavior is visible when the instruction decreasing this is patched away.
    // Needs patching of the instruction manipulating this field for applied values
    // to stick properly.
    static std::vector<float> GetWheelBrakePressure(Vehicle handle);
    static void SetWheelBrakePressure(Vehicle handle, uint8_t index, float value);


    static bool IsWheelPowered(Vehicle handle, uint8_t index);
    static std::vector<uint16_t> GetWheelFlags(Vehicle handle);

    static std::vector<uint32_t> GetVehicleFlags(Vehicle handle);
private:
    VehicleExtensions() = default;
};
