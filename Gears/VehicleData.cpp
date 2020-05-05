#include <inc/natives.h>
#include <numeric>

#include "VehicleData.hpp"
#include "Memory/VehicleFlags.h"
#include "Memory/Offsets.hpp"
#include "Memory/Versions.h"
#include "Util/MathExt.h"


#include "ScriptSettings.hpp"
extern ScriptSettings g_settings;

VehicleData::VehicleData(VehicleExtensions& ext)
    : mVehicle(0), mExt(ext), mHandlingPtr(0)
    , mVelocity(), mAcceleration(), mRPM(0), mRPMPrev(0)
    , mClutch(0), mThrottle(0), mTurbo(0), mHandbrake(false)
    , mSteeringInput(0), mSteeringAngle(0), mSteeringMult(0)
    , mGearCurr(0), mGearNext(0), mGearTop(0)
    , mDriveMaxFlatVel(0), mInitialDriveMaxFlatVel(0)
    , mWheelCount(0), mWheelAverageDrivenTyreSpeed(0)
    , mHasSpeedo(false)
    , mHandlingFlags(0), mModelFlags(0)
    , mIsElectric(false), mIsCVT(false), mHasClutch(false)
    , mHasABS(false), mABSType()
    , mClass(), mDomain(), mIsAmphibious(false)
    , mPrevVelocity() {}

void VehicleData::SetVehicle(Vehicle v) {
    mVehicle = v;
    if (ENTITY::DOES_ENTITY_EXIST(mVehicle)) {
        mHandlingPtr = mExt.GetHandlingPtr(mVehicle);

        mClass = findClass(ENTITY::GET_ENTITY_MODEL(mVehicle));
        mDomain = findDomain(mClass);
        auto type = mExt.GetModelType(mVehicle);
        mIsAmphibious = (type == 6 || type == 7);
        mHasSpeedo = false;

        // initialize prev's init state
        mVelocity = ENTITY::GET_ENTITY_SPEED_VECTOR(mVehicle, true);
        mRPM = VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(mVehicle) ?
            mExt.GetCurrentRPM(mVehicle) : 0.01f;
        mSuspensionTravel = mExt.GetWheelCompressions(mVehicle);

        mFlags = mExt.GetVehicleFlags(mVehicle);

        extern eGameVersion g_gameVersion;
        if (g_gameVersion >= G_VER_1_0_1604_0_STEAM) {
            mModelFlags = *reinterpret_cast<uint32_t*>(mHandlingPtr + hOffsets1604.dwStrModelFlags);
            mHandlingFlags = *reinterpret_cast<uint32_t*>(mHandlingPtr + hOffsets1604.dwStrHandlingFlags);
        }
        else {
            mModelFlags = *reinterpret_cast<uint32_t*>(mHandlingPtr + hOffsets.dwStrModelFlags);
            mHandlingFlags = *reinterpret_cast<uint32_t*>(mHandlingPtr + hOffsets.dwStrHandlingFlags);
        }

        mIsElectric = mFlags[1] & eVehicleFlag2::FLAG_IS_ELECTRIC;
        mIsCVT = mHandlingFlags & 0x00001000;
        mHasClutch = !mIsElectric && !mIsCVT;

        mHasABS = getABSType(mModelFlags) != ABSType::ABS_NONE;
        mABSType = getABSType(mModelFlags);
        mWheelsTcs.resize(mExt.GetNumWheels(mVehicle));
        mWheelsAbs.resize(mExt.GetNumWheels(mVehicle));
        mWheelsEspO.resize(mExt.GetNumWheels(mVehicle));
        mWheelsEspU.resize(mExt.GetNumWheels(mVehicle));
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

    if (!mHasSpeedo && mExt.GetDashSpeed(mVehicle) > 0.0f) {
        mHasSpeedo = true;
    }

    // These depend on values retrieved in the current tick
    mWheelsDriven = getDrivenWheels();
    mWheelAverageDrivenTyreSpeed = getAverageDrivenWheelTyreSpeeds();
    mWheelsLockedUp = getWheelsLockedUp();
    mSuspensionTravelSpeeds = getSuspensionTravelSpeeds();
    mAcceleration = getAcceleration();

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

float VehicleGearboxStates::parsePowertrainRatioThreshold() {
    auto avg = (std::accumulate(PowertrainHistoryDistribution.begin(), PowertrainHistoryDistribution.end(), 0.0f)) / PowertrainHistoryDistribution.size();
    auto max = *std::max_element(PowertrainHistoryDistribution.begin(), PowertrainHistoryDistribution.end());
    if (max > avg) {
        std::vector<float> gapElements = {};
        auto gap = max - avg;
        for (auto& c : PowertrainHistoryDistribution)
            if (c > avg) gapElements.push_back(c);
        auto ratio = avg - ((gap * ((gapElements.size() + 0.0f) / (PowertrainHistoryDistribution.size() + 0.00f))) * 2.0f);
        auto integralityIndex = (PowertrainHistoryDistribution.size() + 1.0f) / (PowertrainHistorySize + 1.0f);
        if (integralityIndex > 1.0f) integralityIndex = 1.0f;
        return ratio * integralityIndex;
    }
    else return avg;
}

float VehicleGearboxStates::parsePowertrainRatio() {
    auto avg = (std::accumulate(PowertrainHistoryDistribution.begin(), PowertrainHistoryDistribution.end(), 0.0f)) / PowertrainHistoryDistribution.size();
    auto max = *std::max_element(PowertrainHistoryDistribution.begin(), PowertrainHistoryDistribution.end());
    if (max > avg) {
        std::vector<float> gapElements = {};
        auto gap = max - avg;
        for (auto& c : PowertrainHistoryDistribution)
            if (c > avg) gapElements.push_back(c);
        return avg + (gap * ((gapElements.size() + 0.0f) / (PowertrainHistoryDistribution.size() + 0.00f)));
    }
    else return avg;
}

void VehicleGearboxStates::updatePowertrainRatioDistribution(float ratio) {
    PowertrainHistoryDistribution.push_back(ratio);
    while (PowertrainHistoryDistribution.size() > PowertrainHistorySize || PowertrainHistoryDistribution[0] == 0.000123f)
        PowertrainHistoryDistribution.erase(PowertrainHistoryDistribution.begin());
}

bool VehicleGearboxStates::isPowertrainRatioTrustworthy() {
    auto ratio = parsePowertrainRatio();
    return PowertrainHistoryDistribution.size() > (PowertrainHistorySize * 0.5) && (abs(ratio - parsePowertrainRatioThreshold()) / ratio) < 0.08f;
}
