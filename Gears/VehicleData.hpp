#pragma once

#include <inc/types.h>

#include <vector>
#include <chrono>
#include "Memory/VehicleExtensions.hpp"
#include "AtcuGearbox.h"

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

enum class ABSType {
    ABS_NONE,
    ABS_STD,
    ABS_OPTION,
    ABS_ALT_STD,
    ABS_ALT_OPTION,
};

enum class IgnitionState {
    Off,
    Stall,
    On,
};

struct VehiclePeripherals {
    // "Peripherals"
    bool BlinkerLeft = false;
    bool BlinkerRight = false;
    bool BlinkerHazard = false;
    int BlinkerTicks = 0;
    bool LookBackRShoulder = false;
    int RadioStationIndex = 0;
    IgnitionState IgnitionState = IgnitionState::Off;
};

enum class ShiftDirection {
    Up,
    Down
};

struct VehicleGearboxStates {
    // Gearbox stuff
    float StallProgress = 0.0f;
    uint8_t LockGear = 1;
    bool FakeNeutral = false;
    bool HitRPMSpeedLimiter = false; // Limit speed at top RPM
    bool HitRPMLimiter = false; // Limit RPM so it doesn't >1.0f

    // Delayed shifting
    bool Shifting = false; 
    uint8_t NextGear = 1;
    float ClutchVal = 0.0f; // Clutch value _while_ Shifting
    ShiftDirection ShiftDirection = ShiftDirection::Up;

    // Auto gearbox stuff
    float ThrottleHang = 0.0f; // throttle value for low load upshifting
    int LastUpshiftTime = 0;

    // Auto gearbox debug
    float EngineLoad = 0.0f;
    float UpshiftLoad = 0.0f;
    float DownshiftLoad = 0.0f;

    //ATCU
    AtcuGearbox Atcu = AtcuGearbox();
};

struct WheelPatchStates {
    // Brake and throttle patch related stuff
    bool EngBrakeActive = false;
    bool EngLockActive = false;
    bool InduceBurnout = false;
};


// Contains all data of a vehicle, gets updated on tick.
// Prefer to use this class over reading ext and calculating stuff
// all the damn time.
class VehicleData {
public:
    VehicleData();

    void SetVehicle(Vehicle v);
    void Update();

    // These should be read-only, but I cba to write getters for all of these.    
    // Vehicle this data is valid for
    Vehicle mVehicle;
    uint64_t mHandlingPtr;

    Vector3 mVelocity;
    Vector3 mAcceleration;

    float mRPM;
    float mRPMPrev;
    float mClutch;
    float mThrottle;
    float mTurbo;
    bool mHandbrake;

    float mSteeringInput; // -1.0f to 1.0f
    float mSteeringAngle; // in radians
    float mSteeringMult;

    uint8_t mGearCurr;
    uint8_t mGearNext;
    uint8_t mGearTop;
    std::vector<float> mGearRatios;

    float mDriveMaxFlatVel;
    float mInitialDriveMaxFlatVel;

    uint8_t mWheelCount;
    std::vector<bool> mWheelsDriven;
    std::vector<float> mWheelTyreSpeeds;
    float mWheelAverageDrivenTyreSpeed;

    std::vector<bool> mWheelsLockedUp;
    std::vector<bool> mWheelsOnGround;
    std::vector<float> mWheelSteeringAngles;

    std::vector<float> mSuspensionTravel;
    std::vector<float> mSuspensionTravelSpeeds;

    std::vector<float> mBrakePressures;

    // set externally, boo?
    std::vector<bool> mWheelsTcs;
    std::vector<bool> mWheelsAbs;
    std::vector<bool> mWheelsEspO;
    std::vector<bool> mWheelsEspU;
    // Workaround
    bool mHasSpeedo;

    // Note: Size == 6
    std::vector<uint32_t> mFlags;

    uint32_t mHandlingFlags;
    uint32_t mModelFlags;
    
    bool mIsElectric;
    bool mIsCVT;
    bool mHasClutch;
    bool mHasABS;
    ABSType mABSType;

    //VehicleInfo mInfo;

    // Non-changing for instance
    VehicleClass mClass;
    VehicleDomain mDomain;
    bool mIsAmphibious;
    bool mIsRhd;
private:
    std::vector<bool> getDrivenWheels();
    float getAverageDrivenWheelTyreSpeeds();
    std::vector<bool> getWheelsLockedUp();
    std::vector<float> getSuspensionTravelSpeeds();
    Vector3 getAcceleration();

    VehicleClass findClass(Hash model);
    VehicleDomain findDomain(VehicleClass vehicleClass);
    ABSType getABSType(uint32_t handlingFlags);

    std::vector<float> mPrevSuspensionTravel;
    std::vector<std::vector<float>> mSuspensionTravelSpeedsHistory;

    Vector3 mPrevVelocity;
};
