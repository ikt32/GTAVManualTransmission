#pragma once

#include "../../ScriptHookV_SDK/inc/types.h"

#include <Windows.h>
#include <array>
#include <vector>
#include "Memory/VehicleExtensions.hpp"
#include <chrono>

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

    bool NoClutch = false;
    bool IsTruck = false;

    float RPM = 0.0f;
    
    uint8_t LockGear = 1;
    uint16_t PrevGear = 1;
    float LockSpeed = 0.0f;
    bool FakeNeutral = false;

    std::vector<float> WheelCompressions = {};
    Vector3 SpeedVector = {};

    float PrevRPM = 0.0f;

    bool BlinkerLeft = false;
    bool BlinkerRight = false;
    bool BlinkerHazard = false;
    int BlinkerTicks = 0;

    bool TruckLockSpeed = false;
    //bool TruckShiftUp = false;

    int RadioStationIndex = 0;
    bool HasSpeedo = false;

    float StallProgress = 0.0f;
    bool EngBrakeActive = false;
    bool EngLockActive = false;
private:
    std::array<char *, 20> badModelNames = {
        "BENSON",
        "BIFF",
        "HAULER",
        "PACKER",
        "PHANTOM",
        "POUNDER",
        "FIRETRUK",
        "DUMP",
        "FLATBED",
        "MIXER",
        "MIXER2",
        "RUBBLE",
        "TIPTRUCK",
        "TIPTRUCK2",
        "BARRACKS",
        "BARRACKS2",
        "BARRACKS3",
        "RIPLEY",
        "SCRAP",
        "UTILTRUC"
    };

    bool isBadTruck(char* name);
    VehicleClass findClass(Hash model);
    VehicleDomain findDomain(VehicleClass vehicleClass);
    bool isAmphibious(VehicleExtensions &ext, Vehicle vehicle);

    void updateAcceleration();
    void updateAverageAcceleration();

    Vector3 SpeedVectorPrev = {};
    std::vector<float> prevCompressions = {};

    long long prevAccelTime = 0;
    long long prevCompressTime = 0;

    Vector3 acceleration;
    std::array<Vector3, SAMPLES> accelerationSamples = {};
    int averageAccelIndex = 0;
};
