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
}

void UpdateAdaptive(float& targetSetpoint, float& brakeThreshold);

bool CruiseControl::GetActive() {
    return active;
}

void CruiseControl::SetActive(bool active) {
    ::active = active;
}

SRayResult RayCast(Vehicle vehicle, float range, Vector3 startOffset, Vector3 endOffset) {
    auto rayOrg = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(vehicle, startOffset.x, startOffset.y, startOffset.z);
    auto rayEnd = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(vehicle, endOffset.x, endOffset.y + range, endOffset.z);

    // START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE or START_SHAPE_TEST_LOS_PROBE
    int ray = SHAPETEST::_START_SHAPE_TEST_RAY(
        rayOrg.x, rayOrg.y, rayOrg.z,
        rayEnd.x, rayEnd.y, rayEnd.z, 10, vehicle, 7);

    //GRAPHICS::DRAW_LINE(rayOrg.x, rayOrg.y, rayOrg.z,
    //    rayEnd.x, rayEnd.y, rayEnd.z, 255, 0, 255, 255);

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
            (minResult.HitEntity == 0 || result.Distance < minResult.Distance )) {
            minResult = result;
        }
    }

    return minResult;
}

void CruiseControl::Update(float& throttle, float& brake, float& clutch) {
    }

    if (active) {
        float prevThrottle = VExt::GetThrottleP(g_playerVehicle);
        float prevBrake = VExt::GetBrakeP(g_playerVehicle);

        float brakeThreshold = 6.0f;
        float deltaMax = 12.0f;

        float targetSetpoint = g_settings().DriveAssists.CruiseControl.Speed;
        if (g_settings().DriveAssists.CruiseControl.Adaptive) {
            UpdateAdaptive(targetSetpoint, brakeThreshold);
        }

        float delta = g_vehData.mVelocity.y - targetSetpoint;

        // Too fast, slow down
        if (delta > 0) {
            // Lift
            if (prevThrottle > 0) {
                throttle = prevThrottle - map(delta, 0.0f, deltaMax, 0.0f, 1.0f);
            }
            // Brake
            else if (delta > brakeThreshold) {
                throttle = 0.0f;
                brake = map(delta, 0.0f, 2.0f * brakeThreshold, 0.0f, 1.0f);
            }
            // Coast
            else {
                throttle = prevThrottle;
            }
        }
        // Too slow, accelerate
        else {
            // increase speed
            throttle = prevThrottle + map(delta, 0.0f, -deltaMax, 0.0f, 1.0f);
            brake = 0.0f;
        }

        throttle = std::clamp(throttle, 0.0f, 1.0f);
        brake = std::clamp(brake, 0.0f, 1.0f);
    }
}

void UpdateAdaptive(float& targetSetpoint, float& brakeThreshold) {
    const float minFrontBuff = 5.0f;
    const float maxFrontBuff = 10.0f;

    // 120m to 150m detection range
    const float maxCastDist = 120.0f;

    auto result = MultiCast(g_playerVehicle, maxCastDist);
    bool hit = result.Hit;
    Vector3 hitCoord = result.HitCoord;
    Entity hitEntity = result.HitEntity;

    UI::ShowText(0.60f, 0.200f, 0.25f, fmt::format("Adaptive {}", hit ? "~g~active" : "~y~stand by"));

    // Distances need to scale with vehicles' own speed.
    // min: Distance where it should match speed
    //  Increase when fast
    //  Decrease when slow
    // max: Distance where it should start slowing down
    //  Increase when fast
    //  Decrease when slow

    if (hit) {
        UI::DrawSphere(hitCoord, 0.10f, Util::ColorsI::SolidPink);

        float distMin = std::max(g_vehData.mDimMax.y + minFrontBuff, g_vehData.mVelocity.y / 2.0f); // 15.0
        float distMax = std::clamp(distMin + g_vehData.mVelocity.y * 4.0f, distMin, maxCastDist); // 60.0

        auto vehPos = ENTITY::GET_ENTITY_COORDS(g_playerVehicle, true);
        float distance = Distance(vehPos, hitCoord);
        float otherSpeed = ENTITY::GET_ENTITY_SPEED(hitEntity);

        UI::ShowText(0.60f, 0.225f, 0.25f, fmt::format("{}Dist: {:.1f}, Max: {:.1f}", distance < distMax ? "~g~" : "", distance, distMax));

        if (distance < distMax) {
            float targetSpeed =
                map(distance,
                    distMin, distMax,
                    otherSpeed, targetSetpoint);
            targetSpeed = std::clamp(targetSpeed, 0.0f, targetSetpoint);

            targetSetpoint = targetSpeed;

            UI::ShowText(0.60f, 0.250f, 0.25f, fmt::format("{}Dist: {:.1f}, Min: {:.1f}", distance < distMin ? "~g~" : "", distance, distMin));
            UI::ShowText(0.60f, 0.275f, 0.25f, fmt::format("Spd: {:.1f}, TgtSpd: {:.1f}", otherSpeed, targetSpeed));

            if (distance < g_vehData.mDimMax.y + maxFrontBuff) {
                brakeThreshold = map(distance, g_vehData.mDimMax.y + minFrontBuff, g_vehData.mDimMax.y + maxFrontBuff, 0.0f, brakeThreshold);
                brakeThreshold = std::max(brakeThreshold, 0.01f);
            }

            UI::ShowText(0.60f, 0.300f, 0.25f, fmt::format("BrkTrs: {:.1f}", brakeThreshold));

        }
    }
}