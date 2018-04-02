#pragma once

#include <inc/types.h>

//#include <Windows.h>
#include <array>
#include <vector>
#include <chrono>
#include "script.h"
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
    Vector3 GetRelativeAcceleration();                  // dV/dT
    Vector3 GetRelativeAccelerationAverage() const;     // Moving average

    VehicleClass Class = VehicleClass::Unknown;
    VehicleDomain Domain = VehicleDomain::Unknown;
    bool Amphibious = false;

    float PrevRPM = 0.0f;
    float RPM = 0.0f;
    uint8_t LockGear = 1;
    float LockSpeed = 0.0f;

    bool NoClutch = false;
    bool FakeNeutral = false;
    bool HitLimiter = false;
    DWORD PrevUpshiftTime = 0;
    bool IgnoreAccelerationUpshiftTrigger = false;

    std::vector<float> WheelCompressions = { };
    Vector3 SpeedVector{};

    bool BlinkerLeft = false;
    bool BlinkerRight = false;
    bool BlinkerHazard = false;
    int BlinkerTicks = 0;

    bool LookBackRShoulder = false;

    int RadioStationIndex = 0;
    bool HasSpeedo = false;

    float StallProgress = 0.0f;
    bool EngBrakeActive = false;
    bool EngLockActive = false;

    std::array<float, NUM_GEARS> UpshiftSpeedsGame = { };
    std::array<float, NUM_GEARS> UpshiftSpeedsMod = { };
    bool InduceBurnout = false;
private:
    VehicleClass findClass(Hash model);
    VehicleDomain findDomain(VehicleClass vehicleClass);
    bool isAmphibious(VehicleExtensions &ext, Vehicle vehicle);
    void updateAcceleration();

    Vector3 prevSpeedVector{};
    std::vector<float> prevCompressions{};

    long long prevAccelTime = 0;
    long long prevCompressTime = 0;

    Vector3 acceleration{};
    std::array<Vector3, SAMPLES> accelerationSamples{};
    int averageAccelIndex = 0;
};
