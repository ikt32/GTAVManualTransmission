#include <inc/natives.h>

#include "VehicleData.hpp"
#include "Memory/VehicleFlags.h"
#include "Memory/Offsets.hpp"
#include "Memory/Versions.h"
#include "Util/MathExt.h"

#include "ScriptSettings.hpp"

using VExt = VehicleExtensions;
extern ScriptSettings g_settings;

namespace {
    bool GetIsRhd(Vehicle v) {
        Vector3 driverSeatPos = 
            ENTITY::GET_WORLD_POSITION_OF_ENTITY_BONE(v, ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(v, "seat_dside_f"));
        Vector3 driverSeatPosRel = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(v,
            driverSeatPos.x, driverSeatPos.y, driverSeatPos.z);

        //Vector3 dimMin, dimMax;
        //MISC::GET_MODEL_DIMENSIONS(ENTITY::GET_ENTITY_MODEL(v), &dimMin, &dimMax);

        if (driverSeatPosRel.x > 0.01f && 
            sgn(driverSeatPosRel.x) == sgn(1.0f)) {
            return true;
        }
        return false;
    }
}

VehicleData::VehicleData()
    : mVehicle(0), mHandlingPtr(0)
    , mVelocity(), mAcceleration(), mRPM(0), mRPMPrev(0)
    , mClutch(0), mThrottle(0), mTurbo(0), mHandbrake(false)
    , mSteeringInput(0), mSteeringAngle(0), mSteeringMult(0)
    , mGearCurr(0), mGearNext(0), mGearTop(0)
    , mDriveMaxFlatVel(0), mInitialDriveMaxFlatVel(0)
    , mWheelCount(0), mDiffSpeed(0)
    , mNonLockSpeed(0), mLastNonLockSpeed(0)
    , mEstimatedSpeed(0), mLastEstimatedSpeed(0), mEstimatedSpeedUsed(false)
    , mHasSpeedo(false)
    , mHandlingFlags(0), mModelFlags(0)
    , mIsElectric(false), mIsCVT(false), mHasClutch(false)
    , mHasABS(false), mABSType()
    , mClass(), mDomain(), mIsAmphibious(false), mIsRhd(false)
    , mPrevVelocity() {}

void VehicleData::SetVehicle(Vehicle v) {
    mVehicle = v;
    if (ENTITY::DOES_ENTITY_EXIST(mVehicle)) {
        mHandlingPtr = VExt::GetHandlingPtr(mVehicle);

        mClass = findClass(ENTITY::GET_ENTITY_MODEL(mVehicle));
        mDomain = findDomain(mClass);
        auto type = VExt::GetModelType(mVehicle);
        mIsAmphibious = (type == 6 || type == 7);
        mHasSpeedo = false;
        mIsRhd = GetIsRhd(v);

        mModel = ENTITY::GET_ENTITY_MODEL(mVehicle);
        MISC::GET_MODEL_DIMENSIONS(mVehicle, &mDimMin, &mDimMax);

        // initialize prev's init state
        mVelocity = ENTITY::GET_ENTITY_SPEED_VECTOR(mVehicle, true);
        mRPM = VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(mVehicle) ?
            VExt::GetCurrentRPM(mVehicle) : 0.01f;
        mSuspensionTravel = VExt::GetWheelCompressions(mVehicle);

        mFlags = VExt::GetVehicleFlags(mVehicle);

        mModelFlags = *reinterpret_cast<uint32_t*>(mHandlingPtr + hOffsets.dwStrModelFlags);
        mHandlingFlags = *reinterpret_cast<uint32_t*>(mHandlingPtr + hOffsets.dwStrHandlingFlags);
        
        mIsElectric = mFlags[1] & eVehicleFlag2::FLAG_IS_ELECTRIC;
        mIsCVT = mHandlingFlags & 0x00001000; // CVT accelerates while clutch is in?
        mHasClutch = !mIsElectric && !mIsCVT;

        mHasABS = getABSType(mModelFlags) != ABSType::ABS_NONE;
        mABSType = getABSType(mModelFlags);

        mWheelsTcs.clear();
        mWheelsTcs.resize(VExt::GetNumWheels(mVehicle));

        mWheelsAbs.clear();
        mWheelsAbs.resize(VExt::GetNumWheels(mVehicle));

        mWheelsEspO.clear();
        mWheelsEspO.resize(VExt::GetNumWheels(mVehicle));

        mWheelsEspU.clear();
        mWheelsEspU.resize(VExt::GetNumWheels(mVehicle));

        mSuspensionTravelSpeedsHistory.clear();
        Update();
    }
}

void VehicleData::Update() {
    if (!ENTITY::DOES_ENTITY_EXIST(mVehicle))
        return;

    // Set previous values
    mPrevVelocity = mVelocity;
    mRPMPrev = mRPM;
    mPrevSuspensionTravel = mSuspensionTravel;

    // Get current values
    mVelocity = ENTITY::GET_ENTITY_SPEED_VECTOR(mVehicle, true);
    mRPM = VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(mVehicle) ?
        VExt::GetCurrentRPM(mVehicle) : 0.01f;
    mClutch = VExt::GetClutch(mVehicle);
    mThrottle = VExt::GetThrottle(mVehicle);
    mTurbo = VExt::GetTurbo(mVehicle);
    mHandbrake = VExt::GetHandbrake(mVehicle);

    mSteeringInput = VExt::GetSteeringInputAngle(mVehicle);
    mSteeringAngle = VExt::GetSteeringAngle(mVehicle);
    auto steeringMults = VExt::GetWheelSteeringMultipliers(mVehicle);
    mSteeringMult = steeringMults.size() > 0 ? steeringMults[0] : 1.0f;

    mGearCurr = static_cast<uint8_t>(VExt::GetGearCurr(mVehicle));
    mGearNext = static_cast<uint8_t>(VExt::GetGearNext(mVehicle));
    mGearTop = VExt::GetTopGear(mVehicle);
    mGearRatios = VExt::GetGearRatios(mVehicle);

    mDriveMaxFlatVel = VExt::GetDriveMaxFlatVel(mVehicle);
    mInitialDriveMaxFlatVel = VExt::GetInitialDriveMaxFlatVel(mVehicle);

    mWheelCount = VExt::GetNumWheels(mVehicle);
    mWheelTyreSpeeds = VExt::GetTyreSpeeds(mVehicle);

    mWheelsOnGround = VExt::GetWheelsOnGround(mVehicle);
    mWheelSteeringAngles = VExt::GetWheelSteeringAngles(mVehicle);

    mSuspensionTravel = VExt::GetWheelCompressions(mVehicle);
    mBrakePressures = VExt::GetWheelBrakePressure(mVehicle);

    if (!mHasSpeedo && VExt::GetDashSpeed(mVehicle) > 0.0f) {
        mHasSpeedo = true;
    }

    // These depend on values retrieved in the current tick
    mWheelsDriven = getDrivenWheels();
    mDiffSpeed = getAverageDrivenWheelTyreSpeeds();
    mWheelsLockedUp = getWheelsLockedUp();
    mNonLockSpeed = getAverageNonLockedWheelTyreSpeeds();
    mSuspensionTravelSpeeds = getSuspensionTravelSpeeds();
    mAcceleration = getAcceleration();
    mEstimatedSpeed = getEstimatedForwardSpeed();

    mSuspensionTravelSpeedsHistory.push_back(mSuspensionTravelSpeeds);
    while (mSuspensionTravelSpeedsHistory.size() > g_settings.Wheel.FFB.DetailMAW) {
        mSuspensionTravelSpeedsHistory.erase(mSuspensionTravelSpeedsHistory.begin());
    }

    std::vector<float> averageSpeeds(mSuspensionTravelSpeedsHistory[0].size());
    for (size_t i = 0; i < mSuspensionTravelSpeedsHistory.size(); ++i) {
        auto speeds = mSuspensionTravelSpeedsHistory[i];
        for (size_t wheelIdx = 0; wheelIdx < speeds.size(); ++wheelIdx) {
            averageSpeeds[wheelIdx] += speeds[wheelIdx];
        }
    }
    for(size_t i = 0; i < averageSpeeds.size(); ++i) {
        averageSpeeds[i] /= static_cast<float>(g_settings.Wheel.FFB.DetailMAW);
    }

    mSuspensionTravelSpeeds = averageSpeeds;
}

std::vector<bool> VehicleData::getDrivenWheels() {
    std::vector<bool> wheelsToConsider;
    wheelsToConsider.reserve(mWheelCount);
    for (int i = 0; i < mWheelCount; i++)
        wheelsToConsider.push_back(VExt::IsWheelPowered(mVehicle, i));

    return wheelsToConsider;
}

float VehicleData::getAverageDrivenWheelTyreSpeeds() {
    float drivenWheelCount = 0.0f;
    float speeds = 0.0f;

    for (uint8_t i = 0; i < mWheelTyreSpeeds.size(); ++i) {
        if (mWheelsDriven[i]) {
            speeds += mWheelTyreSpeeds[i];
            drivenWheelCount += 1.0f;
        }
    }
    return speeds / drivenWheelCount;
}

float VehicleData::getAverageNonLockedWheelTyreSpeeds() {
    if (mWheelCount == 0)
        return 0.0f;

    mLastNonLockSpeed = mNonLockSpeed;

    float nonLockedWheelCount = 0.0f;
    float speeds = 0.0f;

    auto brakePressures = VExt::GetWheelBrakePressure(mVehicle);
    float topTyreSpeed = *std::max_element(mWheelTyreSpeeds.begin(), mWheelTyreSpeeds.end());

    for (uint8_t i = 0; i < mWheelTyreSpeeds.size(); ++i) {
        bool lockedUp = false;
        if (mWheelsLockedUp[i] && mSuspensionTravel[i] > 0.0f && brakePressures[i] > 0.0f)
            lockedUp = true;

        if (mWheelTyreSpeeds[i] < abs(topTyreSpeed) / 2.0f)
            lockedUp = true;

        if (!lockedUp) {
            speeds += mWheelTyreSpeeds[i];
            nonLockedWheelCount += 1.0f;
        }
    }

    if (nonLockedWheelCount == 0.0f)
        return avg(mWheelTyreSpeeds);

    return speeds / nonLockedWheelCount;
}

float VehicleData::getEstimatedForwardSpeed() {
    float accelNonLock = (mNonLockSpeed - mLastNonLockSpeed) / MISC::GET_FRAME_TIME();

    // Wheels are decelerating faster than the acceleration sensor measured, so wheel speed is probably invalid.
    if (mAcceleration.y < 0.0f && 
        accelNonLock < mAcceleration.y && 
        abs(accelNonLock) > abs(mAcceleration.y) * 4.0f) {

        // If we also entered this during the previous tick, two or more improbable decelerations happened in a row.
        // Re-estimated the speed and return it.
        if (mEstimatedSpeedUsed) {
            float estimatedSpeed = mLastEstimatedSpeed - (mPrevVelocity - mVelocity).y;
            mLastEstimatedSpeed = estimatedSpeed;
            return mLastEstimatedSpeed;
        }

        float estimatedSpeed = mLastNonLockSpeed - (mPrevVelocity - mVelocity).y;
        mLastEstimatedSpeed = estimatedSpeed;

        // Indicate the estimated speed was used.
        mEstimatedSpeedUsed = true;

        return estimatedSpeed;
    }

    // We used the estimated speed the last tick. However the previous check only compared the previous
    // frame with the current, but not before that - so as a sanity check, check if the non-lock speed is in range
    // of the speed that we should have been decelerating with.
    if (mAcceleration.y < 0.0f && mEstimatedSpeedUsed) {
        float deltaV = (mPrevVelocity - mVelocity).y;
        float estimatedSpeed = mLastEstimatedSpeed - deltaV;

        if (!Math::Near(mNonLockSpeed, estimatedSpeed, abs(deltaV))) {
            mLastEstimatedSpeed = estimatedSpeed;
            return estimatedSpeed;
        }
    }

    mEstimatedSpeedUsed = false;
    mLastEstimatedSpeed = mNonLockSpeed;
    return mNonLockSpeed;
}

std::vector<bool> VehicleData::getWheelsLockedUp() {
    std::vector<bool> lockups;
    auto wheelsSpeed = VExt::GetWheelRotationSpeeds(mVehicle);
    for (auto wheelSpeed : wheelsSpeed) {
        if (abs(mVelocity.y) > 0.01f && wheelSpeed == 0.0f)
            lockups.push_back(true);
        else
            lockups.push_back(false);
    }
    return lockups;
}

std::vector<float> VehicleData::getSuspensionTravelSpeeds() {
    std::vector<float> suspensionTravelSpeeds(mWheelCount);
    for (size_t i = 0; i < mWheelCount; ++i) {
        suspensionTravelSpeeds[i] =
            (mSuspensionTravel[i] - mPrevSuspensionTravel[i]) / MISC::GET_FRAME_TIME();
    }
    return suspensionTravelSpeeds;
}

Vector3 VehicleData::getAcceleration() {
    Vector3 acceleration{};

    acceleration.x = (mVelocity.x - mPrevVelocity.x) / MISC::GET_FRAME_TIME();
    acceleration.y = (mVelocity.y - mPrevVelocity.y) / MISC::GET_FRAME_TIME();
    acceleration.z = (mVelocity.z - mPrevVelocity.z) / MISC::GET_FRAME_TIME();

    return acceleration;
}

VehicleClass VehicleData::findClass(Hash model) {
    if (VEHICLE::IS_THIS_MODEL_A_CAR(model))        return VehicleClass::Car;
    if (VEHICLE::IS_THIS_MODEL_A_BICYCLE(model))    return VehicleClass::Bicycle;
    if (VEHICLE::IS_THIS_MODEL_A_BIKE(model))       return VehicleClass::Bike;
    if (VEHICLE::IS_THIS_MODEL_A_BOAT(model))       return VehicleClass::Boat;
    if (VEHICLE::IS_THIS_MODEL_A_PLANE(model))      return VehicleClass::Plane;
    if (VEHICLE::IS_THIS_MODEL_A_HELI(model))       return VehicleClass::Heli;
    if (VEHICLE::IS_THIS_MODEL_A_QUADBIKE(model))   return VehicleClass::Quad;
    return VehicleClass::Unknown;
}

VehicleDomain VehicleData::findDomain(VehicleClass vehicleClass) {
    switch (vehicleClass) {
    case VehicleClass::Bicycle: return VehicleDomain::Bicycle;
    case VehicleClass::Boat:    return VehicleDomain::Water;
    case VehicleClass::Heli:
    case VehicleClass::Plane:   return VehicleDomain::Air;
    case VehicleClass::Train:   return VehicleDomain::Rail;
    case VehicleClass::Unknown: return VehicleDomain::Unknown;
    default: return VehicleDomain::Road;
    }
}

ABSType VehicleData::getABSType(uint32_t handlingFlags) {
    if (handlingFlags & 0x10)
        return ABSType::ABS_STD;
    if (handlingFlags & 0x20)
        return ABSType::ABS_OPTION;
    if (handlingFlags & 0x40)
        return ABSType::ABS_ALT_STD;
    if (handlingFlags & 0x80)
        return ABSType::ABS_ALT_OPTION;

    return ABSType::ABS_NONE;
}
