#pragma once

#include <vector>
#include <cstdint>
#include <inc/types.h>
#include <inc/nativeCaller.h>

extern uint8_t g_numGears;

struct WheelDimensions {
    float TyreRadius;
    float RimRadius;
    float TyreWidth;
};

class VehicleExtensions {
public:
    VehicleExtensions();
    static void ChangeVersion(int version);
    BYTE *GetAddress(Vehicle handle);

    /*
     * Vehicle struct
     */
    bool GetRocketBoostActive(Vehicle handle);
    void SetRocketBoostActive(Vehicle handle, bool val);

    float GetRocketBoostCharge(Vehicle handle);
    void SetRocketBoostCharge(Vehicle handle, float value);

    float GetHoverTransformRatio(Vehicle handle);
    void SetHoverTransformRatio(Vehicle handle, float value);

    float GetHoverTransformRatioLerp(Vehicle handle);
    void SetHoverTransformRatioLerp(Vehicle handle, float value);

    float GetFuelLevel(Vehicle handle);
    void SetFuelLevel(Vehicle handle, float value);

    uint16_t GetGearNext(Vehicle handle);
    void SetGearNext(Vehicle handle, uint16_t value);

    uint16_t GetGearCurr(Vehicle handle);
    void SetGearCurr(Vehicle handle, uint16_t value);

    uint8_t GetTopGear(Vehicle handle);
    void SetTopGear(Vehicle handle, uint8_t value);

    // Divide GetDriveMaxFlatVel by the values in this thing to get the top
    // speed for the gear.
    float* GetGearRatioPtr(Vehicle handle, uint8_t gear);
    std::vector<float> GetGearRatios(Vehicle handle);
    void SetGearRatios(Vehicle handle, const std::vector<float>& values);

    float GetDriveForce(Vehicle handle);
    void SetDriveForce(Vehicle handle, float value);

    float GetInitialDriveMaxFlatVel(Vehicle handle);
    void SetInitialDriveMaxFlatVel(Vehicle handle, float value);

    float GetDriveMaxFlatVel(Vehicle handle);
    void SetDriveMaxFlatVel(Vehicle handle, float value);

    float GetCurrentRPM(Vehicle handle);
    void SetCurrentRPM(Vehicle handle, float value);
    
    float GetClutch(Vehicle handle);
    void SetClutch(Vehicle handle, float value);
    
    float GetThrottle(Vehicle handle);
    void SetThrottle(Vehicle handle, float value);

    float GetTurbo(Vehicle handle);
    void SetTurbo(Vehicle handle, float value);

    uint64_t GetHandlingPtr(Vehicle handle);

    // Steering input angle, steering lock independent
    float GetSteeringInputAngle(Vehicle handle);
    void SetSteeringInputAngle(Vehicle handle, float value);

    // Wheel angle, steering lock dependent
    float GetSteeringAngle(Vehicle handle);
    void SetSteeringAngle(Vehicle handle, float value);
   
    float GetThrottleP(Vehicle handle);
    void SetThrottleP(Vehicle handle, float value);
    
    float GetBrakeP(Vehicle handle);
    void SetBrakeP(Vehicle handle, float value);
    
    bool GetHandbrake(Vehicle handle);
    void SetHandbrake(Vehicle handle, bool value);

    float GetDirtLevel(Vehicle handle);
    // No set impl.

    float GetEngineTemp(Vehicle handle);
    // No set impl.

    float GetDashSpeed(Vehicle handle);
    // No set impl.

    // 0 = car, 1 = plane, 2 = trailer, 3 = quad,
    // 6 = amphibious car, 7 = amphibious quad,
    // 8 = heli, 11 = motorcycle, 12 = bicycle, 13 = boat, 14 = train
    int GetModelType(Vehicle handle);

    uint64_t GetWheelsPtr(Vehicle handle);
    uint8_t GetNumWheels(Vehicle handle);

    /*
     * Handling data related getters
     */
    float GetDriveBiasFront(Vehicle handle);
    float GetDriveBiasRear(Vehicle handle);
    float GetPetrolTankVolume(Vehicle handle);
    float GetOilVolume(Vehicle handle);
    float GetMaxSteeringAngle(Vehicle handle);
    Hash GetAIHandling(Vehicle handle);

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
    std::vector<Vector3> GetWheelLastContactCoords(Vehicle handle);
    std::vector<float> GetWheelCompressions(Vehicle handle);
    std::vector<float> GetWheelSteeringAngles(Vehicle handle);
    std::vector<bool> GetWheelsOnGround(Vehicle handle);

    float GetWheelLargestAngle(Vehicle handle);
    float GetWheelAverageAngle(Vehicle handle);

    // Unit: meters, probably.
    std::vector<WheelDimensions> GetWheelDimensions(Vehicle handle);
    // Unit: rad/s
    std::vector<float> GetWheelRotationSpeeds(Vehicle handle);
    // Unit: m/s, at the tyres. This probably doesn't work well for popped tyres.
    std::vector<float> GetTyreSpeeds(Vehicle handle);

    // How much smoke and skidmarks the wheels/tires are generating.
    std::vector<float> GetWheelSkidSmokeEffect(Vehicle handle);
    void SetWheelSkidSmokeEffect(Vehicle handle, uint8_t index, float speed);

    // Needs patching the decreasing thing
    std::vector<float> GetWheelPower(Vehicle handle);
    void SetWheelPower(Vehicle handle, uint8_t index, float value);

    // Strangely, braking/pulling the handbrake just adds to this value.
    // This behavior is visible when the instruction decreasing this is patched away.
    // Needs patching of the instruction manipulating this field for applied values
    // to stick properly.
    std::vector<float> GetWheelBrakePressure(Vehicle handle);
    void SetWheelBrakePressure(Vehicle handle, uint8_t index, float value);


    bool IsWheelPowered(Vehicle handle, uint8_t index);
    std::vector<uint16_t> GetWheelFlags(Vehicle handle);

    std::vector<uint32_t> GetVehicleFlags(Vehicle handle);

    void initOffsets();


private:
    int rocketBoostActiveOffset = 0;
    int rocketBoostChargeOffset = 0;
    int hoverTransformRatioOffset = 0;
    int hoverTransformRatioLerpOffset = 0;
    int fuelLevelOffset = 0;
    int nextGearOffset = 0;
    int currentGearOffset = 0;
    int topGearOffset = 0;
    int gearRatiosOffset = 0;
    int driveForceOffset = 0;
    int initialDriveMaxFlatVelOffset = 0;
    int driveMaxFlatVelOffset = 0;
    int currentRPMOffset = 0;
    int clutchOffset = 0;
    int throttleOffset = 0;
    int turboOffset = 0;
    int handlingOffset = 0;
    int steeringAngleInputOffset = 0;
    int steeringAngleOffset = 0;
    int throttlePOffset = 0;
    int brakePOffset = 0;
    int handbrakeOffset = 0;
    int dirtLevelOffset = 0;
    int engineTempOffset = 0;
    int dashSpeedOffset = 0;
    int wheelsPtrOffset = 0;
    int numWheelsOffset = 0;
    int modelTypeOffset = 0; 
    const int vehicleModelInfoOffset = 0x020;
    int vehicleFlagsOffset = 0;

    int steeringMultOffset = 0;

    // Wheel stuff
    int wheelHealthOffset = 0;
    int wheelSuspensionCompressionOffset = 0;
    int wheelSteeringAngleOffset = 0;
    int wheelAngularVelocityOffset = 0;
    int wheelSmokeOffset = 0;
    int wheelPowerOffset = 0;
    int wheelBrakeOffset = 0;
    int wheelFlagsOffset = 0;

};
