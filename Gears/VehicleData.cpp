#include <inc/natives.h>

#include "VehicleData.hpp"
#include "Memory/VehicleFlags.h"
#include "script.h"

VehicleData::VehicleData(VehicleExtensions& ext)
    : mExt(ext) {
}

void VehicleData::SetVehicle(Vehicle v) {
    mVehicle = v;
    if (ENTITY::DOES_ENTITY_EXIST(mVehicle)) {
        mHandlingPtr = mExt.GetHandlingPtr(mVehicle);

        mClass = findClass(ENTITY::GET_ENTITY_MODEL(mVehicle));
        mDomain = findDomain(mClass);
        auto type = mExt.GetModelType(mVehicle);
        mIsAmphibious = (type == 6 || type == 7);

        // initialize prev's init state
        mVelocity = ENTITY::GET_ENTITY_SPEED_VECTOR(mVehicle, true);
        mRPM = VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(mVehicle) ?
            mExt.GetCurrentRPM(mVehicle) : 0.01f;
        mSuspensionTravel = mExt.GetWheelCompressions(mVehicle);

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
        mExt.GetCurrentRPM(mVehicle) : 0.01f;
    mClutch = mExt.GetClutch(mVehicle);
    mThrottle = mExt.GetThrottle(mVehicle);
    mTurbo = mExt.GetTurbo(mVehicle);
    mHandbrake = mExt.GetHandbrake(mVehicle);

    mSteeringInput = mExt.GetSteeringInputAngle(mVehicle);
    mSteeringAngle = mExt.GetSteeringAngle(mVehicle);
    mSteeringMult = mExt.GetSteeringMultiplier(mVehicle);

    mGearCurr = static_cast<uint8_t>(mExt.GetGearCurr(mVehicle));
    mGearNext = static_cast<uint8_t>(mExt.GetGearNext(mVehicle));
    mGearTop = mExt.GetTopGear(mVehicle);
    mGearRatios = mExt.GetGearRatios(mVehicle);

    mDriveMaxFlatVel = mExt.GetDriveMaxFlatVel(mVehicle);
    mInitialDriveMaxFlatVel = mExt.GetInitialDriveMaxFlatVel(mVehicle);

    mWheelCount = mExt.GetNumWheels(mVehicle);
    mWheelTyreSpeeds = mExt.GetTyreSpeeds(mVehicle);

    mWheelsOnGround = mExt.GetWheelsOnGround(mVehicle);
    mWheelSteeringAngles = mExt.GetWheelSteeringAngles(mVehicle);

    mSuspensionTravel = mExt.GetWheelCompressions(mVehicle);
    mBrakePressures = mExt.GetWheelBrakePressure(mVehicle);

    // TODO: Parse Flags
    mFlags = mExt.GetVehicleFlags(mVehicle);

    mIsElectric = mFlags[1] & eVehicleFlag2::FLAG_IS_ELECTRIC;
    mIsCVT = mGearTop == 1; //TODO: Extract from handling flags
    mHasClutch = mIsElectric || mIsCVT;

    if (!mHasSpeedo && mExt.GetDashSpeed(mVehicle) > 0.0f) {
        mHasSpeedo = true;
    }

    // These depend on values retrieved in the current tick
    mWheelsDriven = getDrivenWheels();
    mWheelAverageDrivenTyreSpeed = getAverageDrivenWheelTyreSpeeds();
    mWheelsLockedUp = getWheelsLockedUp();
    mSuspensionTravelSpeeds = getSuspensionTravelSpeeds();
    mAcceleration = getAcceleration();
}

std::vector<bool> VehicleData::getDrivenWheels() {
    std::vector<bool> wheelsToConsider;
    wheelsToConsider.reserve(mWheelCount);
    for (int i = 0; i < mWheelCount; i++)
        wheelsToConsider.push_back(mExt.IsWheelPowered(mVehicle, i));

    return wheelsToConsider;
}

float VehicleData::getAverageDrivenWheelTyreSpeeds() {
    unsigned drivenWheelCount = 0;
    float speeds = 0.0f;

    for (uint8_t i = 0; i < mWheelTyreSpeeds.size(); ++i) {
        if (mWheelsDriven[i]) {
            speeds += mWheelTyreSpeeds[i];
            drivenWheelCount++;
        }
    }
    return speeds / static_cast<float>(drivenWheelCount);
}

std::vector<bool> VehicleData::getWheelsLockedUp() {
    std::vector<bool> lockups;
    auto wheelsSpeed = mExt.GetWheelRotationSpeeds(mVehicle);
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
            (mSuspensionTravel[i] - mPrevSuspensionTravel[i]) / GAMEPLAY::GET_FRAME_TIME();
    }
    return suspensionTravelSpeeds;
}

Vector3 VehicleData::getAcceleration() {
    Vector3 acceleration{};

    acceleration.x = (mVelocity.x - mPrevVelocity.x) / GAMEPLAY::GET_FRAME_TIME();
    acceleration.y = (mVelocity.y - mPrevVelocity.y) / GAMEPLAY::GET_FRAME_TIME();
    acceleration.z = (mVelocity.z - mPrevVelocity.z) / GAMEPLAY::GET_FRAME_TIME();

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