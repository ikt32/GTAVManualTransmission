#include "CruiseControl.h"

#include "ScriptSettings.hpp"
#include "VehicleData.hpp"
#include "Input/CarControls.hpp"
#include "Util/MathExt.h"
#include "Util/UIUtils.h"

#include <fmt/format.h>
#include <inc/enums.h>
#include <inc/natives.h>

extern CarControls g_controls;
extern ScriptSettings g_settings;
extern Vehicle g_playerVehicle;
extern VehicleData g_vehData;

using VExt = VehicleExtensions;

struct SRayResult {
    BOOL Hit;
    Vector3 HitCoord;
    Entity HitEntity;
    float Distance;
};

namespace {
    bool active = false;
    float cruiseThrottle = 0.0f;
    bool adaptiveActive = false;
}

void UpdateAdaptive(float& targetSetpoint, float& brakeValue);

bool CruiseControl::GetActive() {
    return active;
}

void CruiseControl::SetActive(bool active) {
    g_settings().DriveAssists.CruiseControl.Enable = active;
    ::active = active;
    cruiseThrottle = 0.0f;
    UI::Notify(INFO, fmt::format("Cruise control {}", active ? "~g~ON" : "~r~OFF"), true);
}

bool CruiseControl::GetAdaptiveActive() {
    return adaptiveActive;
}

SRayResult RayCast(Vehicle vehicle, float range, Vector3 startOffset, Vector3 endOffset) {
    auto rayOrg = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(vehicle, startOffset.x, startOffset.y, startOffset.z);
    auto rayEnd = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(vehicle, endOffset.x, endOffset.y + range, endOffset.z);

    // START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE or START_SHAPE_TEST_LOS_PROBE
    int ray = SHAPETEST::_START_SHAPE_TEST_RAY(
        rayOrg.x, rayOrg.y, rayOrg.z,
        rayEnd.x, rayEnd.y, rayEnd.z, 10, vehicle, 7);

    BOOL hit = false;
    Vector3 hitCoord, surfaceNormal;
    Entity hitEntity;
    int rayResult = SHAPETEST::GET_SHAPE_TEST_RESULT(ray, &hit, &hitCoord, &surfaceNormal, &hitEntity);
    float distance = Distance(rayOrg, hitCoord);

    return {
        hit, hitCoord, hitEntity, distance
    };
}

// Casts an array of 9, returns closest result.
SRayResult MultiCast(Vehicle vehicle, float range) {
    Vector3 dimMax, dimMin;
    MISC::GET_MODEL_DIMENSIONS(ENTITY::GET_ENTITY_MODEL(vehicle), &dimMin, &dimMax);
    Vector3 vel = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true);

    Vector3 rotVel = ENTITY::GET_ENTITY_ROTATION_VELOCITY(g_playerVehicle);
    Vector3 rotRelative{
        120.0f * -sin(rotVel.z), 0,
        120.0f * cos(rotVel.z), 0,
        0, 0
    };

    float avgAngle = VExt::GetWheelAverageAngle(g_playerVehicle);
    Vector3 vecNextRot = (vel + rotRelative) * 0.5f;

    std::vector<Vector3> offsets{
        // centercenter
        Vector3 {
            0.0f, 0,
            dimMax.y, 0,
            0.0f, 0
        },
        // centertop / 2
        Vector3 {
            0.0f, 0,
            dimMax.y, 0,
            dimMax.z / 2.0f, 0
        },
        // centertop
        Vector3 {
            0.0f, 0,
            dimMax.y, 0,
            dimMax.z, 0
        },
        // centerbot / 2
        Vector3 {
            0.0f, 0,
            dimMax.y, 0,
            dimMin.z / 2.0f, 0
        },
        // centerbot
        Vector3 {
            0.0f, 0,
            dimMax.y, 0,
            dimMin.z, 0
        },
        // leftcenter / 2
        Vector3 {
            dimMin.x / 2.0f, 0,
            dimMax.y, 0,
            0.0f, 0
        },
        // leftcenter
        Vector3 {
            dimMin.x, 0,
            dimMax.y, 0,
            0.0f, 0
        },
        // rightcenter / 2
        Vector3 {
            dimMax.x / 2.0f, 0,
            dimMax.y, 0,
            0.0f, 0
        },
        // rightcenter
        Vector3 {
            dimMax.x, 0,
            dimMax.y, 0,
            0.0f, 0
        },
    };

    SRayResult minResult{};
    for (const auto& offset : offsets) {
        Vector3 offVel = offset;
        offVel.x += vecNextRot.x;
        auto result = RayCast(g_playerVehicle, 120.0f, offset, offVel);

        if (result.Hit &&
            (minResult.HitEntity == 0 || result.Distance < minResult.Distance)) {
            minResult = result;
        }
    }

    return minResult;
}

void CruiseControl::Update(float& throttle, float& brake, float& clutch) {
    if (!active) {
        adaptiveActive = false;
        cruiseThrottle = 0.0f;
        return;
    }

    float brakeValue = 0.0f;

    float targetSetpoint = g_settings().DriveAssists.CruiseControl.Speed;
    if (g_settings().DriveAssists.CruiseControl.Adaptive) {
        UpdateAdaptive(targetSetpoint, brakeValue);
    }

    float delta = g_vehData.mVelocity.y - targetSetpoint;

    if (brakeValue > 0.0f) {
        throttle = 0.0f;
        cruiseThrottle = 0.0f;
        brake = brakeValue;
    }
    else {
        brake = 0.0f;

        // looks constant
        const float cruiseAccCorrFactor = 1.0f;

        // what is this?
        float cruiseTgtAccFactor = 0.4f;

        // keep under max grip * 9.8
        float cruiseMaxAcceleration = g_settings().DriveAssists.CruiseControl.MaxAcceleration;

        // By LieutenantDan
        float targetAcceleration = std::clamp((targetSetpoint - g_vehData.mVelocity.y) * cruiseTgtAccFactor, -cruiseMaxAcceleration, cruiseMaxAcceleration);
        cruiseThrottle = std::clamp(cruiseThrottle + (targetAcceleration - g_vehData.mAcceleration.y) * cruiseAccCorrFactor * MISC::GET_FRAME_TIME(), 0.0f, 1.0f);
        throttle = std::clamp(throttle + cruiseThrottle, 0.0f, 1.0f);
    }

    brake = std::clamp(brake, 0.0f, 1.0f);
    throttle = std::clamp(throttle, 0.0f, 1.0f);
}

void UpdateAdaptive(float& targetSetpoint, float& brakeValue) {
    // 120m to 150m detection range
    const float maxCastDist = 200.0f;

    // When speed is zero, drive up to here
    const float minFollowDistance = g_settings().DriveAssists.CruiseControl.MinFollowDistance;

    // Start to match speed here
    const float maxFollowDistance = g_settings().DriveAssists.CruiseControl.MaxFollowDistance;

    auto result = MultiCast(g_playerVehicle, maxCastDist);
    bool hit = result.Hit;
    Vector3 hitCoord = result.HitCoord;
    Entity hitEntity = result.HitEntity;

    // Distances need to scale with vehicles' own speed.
    // min: Distance where it should match speed
    //  Increase when fast
    //  Decrease when slow
    // max: Distance where it should start slowing down
    //  Increase when fast
    //  Decrease when slow

    if (hit) {
        adaptiveActive = true;
        //UI::DrawSphere(hitCoord, 0.10f, Util::ColorsI::SolidPink);

        const float distMinSpdMult = g_settings().DriveAssists.CruiseControl.MinDistanceSpeedMult;
        const float distMaxSpdMult = g_settings().DriveAssists.CruiseControl.MaxDistanceSpeedMult;

        float distMin = std::max(g_vehData.mDimMax.y + minFollowDistance, g_vehData.mVelocity.y * distMinSpdMult);
        float distMax = std::clamp(distMin + g_vehData.mVelocity.y * distMaxSpdMult, distMin, maxFollowDistance);

        auto vehPos = ENTITY::GET_ENTITY_COORDS(g_playerVehicle, true);
        float distance = Distance(vehPos, hitCoord);
        float otherSpeed = ENTITY::GET_ENTITY_SPEED_VECTOR(hitEntity, true).y;
        float deltaSpeed = g_vehData.mVelocity.y - otherSpeed;

        if (distance < distMax) {
            const float deltaMinBrkMult = g_settings().DriveAssists.CruiseControl.MinDeltaBrakeMult;
            const float deltaMaxBrkMult = g_settings().DriveAssists.CruiseControl.MaxDeltaBrakeMult;

            float targetSpeed =
                map(distance,
                    distMin, distMax,
                    otherSpeed, targetSetpoint);
            targetSpeed = std::clamp(targetSpeed, 0.0f, targetSetpoint);

            targetSetpoint = targetSpeed;

            float brakeMinDistance = g_vehData.mDimMax.y + minFollowDistance + deltaSpeed * deltaMinBrkMult;
            float brakeMaxDistance = g_vehData.mDimMax.y + minFollowDistance + deltaSpeed * deltaMaxBrkMult;
            if (distance < brakeMinDistance) {
                brakeValue = map(distance,
                    brakeMinDistance, brakeMaxDistance,
                    0.0f, 1.0f);
                brakeValue = std::clamp(brakeValue, 0.0f, 1.0f);
            }
        }
        else if (Math::Near(distMin, distMax, 0.1f)) {
            targetSetpoint = otherSpeed;
        }
    }
    else {
        adaptiveActive = false;
    }
}