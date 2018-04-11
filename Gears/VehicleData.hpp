#pragma once

#include <inc/types.h>

//#include <Windows.h>
#include <array>
#include <vector>
#include <chrono>
#include "Constants.h"
#include "Memory/VehicleExtensions.hpp"

#define SAMPLES 6

enum class VehicleClass {
    Car,
    Bike,
    Bicycle,
    Boat,
    Plane,
    Heli,
    Quad,
    Train,
    Unknown
};

enum class VehicleDomain {
    Road,
    Air,
    Bicycle,
    Water,
    Rail,
    Unknown
};

class VehicleData {
public:
    VehicleData();
    void UpdateValues(VehicleExtensions& ext, Vehicle vehicle);

    std::vector<float> GetWheelCompressionSpeeds();
    Vector3 GetRelativeAcceleration();            // dV/dT
    Vector3 GetRelativeAccelerationAverage();     // Moving average

    // Static-ish vehicle info stuff
    VehicleClass Class = VehicleClass::Unknown;
    VehicleDomain Domain = VehicleDomain::Unknown;
    bool Amphibious = false;
    bool HasSpeedo = false;
    bool NoClutch = false;

    // Continuously updated

    // Gearbox stuff
    float PrevRPM = 0.0f;
    float RPM = 0.0f;
    uint8_t LockGear = 1;

    bool FakeNeutral = false;
    bool HitLimiter = false;

    std::array<float, NUM_GEARS> UpshiftSpeedsGame = {};
    std::array<float, NUM_GEARS> UpshiftSpeedsMod = {};

    // Auto gearbox stuff need to find a better solution/workaround
    DWORD PrevUpshiftTime = 0;
    bool IgnoreAccelerationUpshiftTrigger = false;

    // Brake stuff
    bool EngBrakeActive = false;
    bool EngLockActive = false;
    bool InduceBurnout = false;
private:
    VehicleClass findClass(Hash model);
    VehicleDomain findDomain(VehicleClass vehicleClass);
    bool isAmphibious(VehicleExtensions &ext, Vehicle vehicle);
    void updateAverages(long long thisTick);

    long long prevTick = 0;

    Vector3 speedVector = {};
    Vector3 prevSpeedVector = {};
    Vector3 acceleration = {};
    std::array<Vector3, SAMPLES> accelerationSamples{};
    int averageAccelIndex = 0;

    // Misc stuff
    std::vector<float> wheelCompressions = {};
    std::vector<float> prevCompressions = {};
    std::vector<float> compressionSpeeds = {};

};

// Standalone <<<thing>>>
struct VehiclePeripherals {
    // "Peripherals"
    bool BlinkerLeft = false;
    bool BlinkerRight = false;
    bool BlinkerHazard = false;
    int BlinkerTicks = 0;
    bool LookBackRShoulder = false;
    int RadioStationIndex = 0;

    // Gearbox stuff
    float StallProgress = 0.0f;

};