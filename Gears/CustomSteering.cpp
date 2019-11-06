#include "CustomSteering.h"

#include "ScriptUtils.h"
#include "ScriptSettings.hpp"
#include "Util/MathExt.h"
#include "Util/UIUtils.h"
#include "Util/Util.hpp"
#include "Memory/VehicleExtensions.hpp"

#include "inc/enums.h"
#include "inc/natives.h"
#include "inc/types.h"

#include <algorithm>

extern Vehicle g_playerVehicle;
extern Ped g_playerPed;
extern VehicleExtensions g_ext;
extern ScriptSettings g_settings;

namespace {
    float steerPrev = 0.0f;
}

namespace CustomSteering {
    float calculateReduction();
    float calculateDesiredHeading(float steeringMax, float desiredHeading, float reduction);
    void drawDebug();
}

float CustomSteering::calculateReduction() {
    Vehicle mVehicle = g_playerVehicle;
    float mult = 1;
    Vector3 vel = ENTITY::GET_ENTITY_VELOCITY(mVehicle);
    Vector3 pos = ENTITY::GET_ENTITY_COORDS(mVehicle, 1);
    Vector3 motion = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(mVehicle, pos.x + vel.x, pos.y + vel.y,
        pos.z + vel.z);
    if (motion.y > 3) {
        mult = 0.15f + powf(0.9f, abs(motion.y) - 7.2f);
        if (mult != 0) { mult = floorf(mult * 1000) / 1000; }
        if (mult > 1) { mult = 1; }
    }
    mult = (1 + (mult - 1) * g_settings.CustomSteering.SteeringReduction);
    return mult;
}

// Returns in radians
float CustomSteering::calculateDesiredHeading(float steeringMax, float desiredHeading, float reduction) {
    // Scale input with both reduction and steering limit
    desiredHeading = desiredHeading * reduction * steeringMax;
    float correction = desiredHeading;

    Vector3 speedVector = ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true);
    if (abs(speedVector.y) > 3.0f) {
        Vector3 target = Normalize(speedVector);
        float travelDir = atan2(target.y, target.x) - static_cast<float>(M_PI) / 2.0f;
        if (travelDir > static_cast<float>(M_PI) / 2.0f) {
            travelDir -= static_cast<float>(M_PI);
        }
        if (travelDir < -static_cast<float>(M_PI) / 2.0f) {
            travelDir += static_cast<float>(M_PI);
        }
        // Correct for reverse
        travelDir *= sgn(speedVector.y);

        // Custom user multiplier. Division by steeringmult needed to neutralize over-correction.
        travelDir *= g_settings.CustomSteering.CountersteerMult / g_settings.CustomSteering.SteeringMult;

        // clamp auto correction to countersteer limit
        travelDir = std::clamp(travelDir, deg2rad(-g_settings.CustomSteering.CountersteerLimit), deg2rad(g_settings.CustomSteering.CountersteerLimit));

        correction = travelDir + desiredHeading;
    }
    return std::clamp(correction, -steeringMax, steeringMax);
}

void CustomSteering::drawDebug() {
    float steeringAngle = g_ext.GetWheelLargestAngle(g_playerVehicle);

    Vector3 speedVector = ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true);
    Vector3 positionWorld = ENTITY::GET_ENTITY_COORDS(g_playerVehicle, 1);
    Vector3 travelRelative = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(g_playerVehicle, speedVector.x, speedVector.y, speedVector.z);

    float steeringAngleRelX = ENTITY::GET_ENTITY_SPEED(g_playerVehicle) * -sin(steeringAngle);
    float steeringAngleRelY = ENTITY::GET_ENTITY_SPEED(g_playerVehicle) * cos(steeringAngle);
    Vector3 steeringWorld = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(g_playerVehicle, steeringAngleRelX, steeringAngleRelY, 0.0f);

    //showText(0.3f, 0.15f, 0.5f, fmt::format("Angle: {}", rad2deg(steeringAngle)));

    GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, travelRelative.x, travelRelative.y, travelRelative.z, 0, 255, 0, 255);
    drawSphere(travelRelative, 0.25f, { 0, 255, 0, 255 });
    GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, steeringWorld.x, steeringWorld.y, steeringWorld.z, 255, 0, 255, 255);
    drawSphere(steeringWorld, 0.25f, { 255, 0, 255, 255 });
}

// Main custom steering function - called by main loop.
void CustomSteering::Update() {
    if (!Util::VehicleAvailable(g_playerVehicle, g_playerPed))
        return;

    if (g_settings.Debug.DisplayInfo)
        drawDebug();

    float limitRadians = g_ext.GetMaxSteeringAngle(g_playerVehicle);
    float reduction = calculateReduction();

    float steer = -CONTROLS::GET_DISABLED_CONTROL_NORMAL(1, ControlMoveLeftRight);

    float steerCurr;

    float steerValGammaL = pow(-steer, g_settings.CustomSteering.Gamma);
    float steerValGammaR = pow(steer, g_settings.CustomSteering.Gamma);
    float steerValGamma = steer < 0.0f ? -steerValGammaL : steerValGammaR;

    // TODO: Other approach to smoothing input. Config transform speed.
    if (steer == 0.0f) {
        steerCurr = lerp(
            steerPrev,
            steerValGamma,
            1.0f - pow(0.000001f, GAMEPLAY::GET_FRAME_TIME()));
    }
    else {
        steerCurr = lerp(
            steerPrev,
            steerValGamma,
            1.0f - pow(0.0001f, GAMEPLAY::GET_FRAME_TIME()));
    }
    steerPrev = steerCurr;

    float desiredHeading = calculateDesiredHeading(limitRadians, steerCurr, reduction);

//https://developercommunity.visualstudio.com/content/problem/549386/c4307-warning-in-c17-constexpr-code-reported-with.html
//https://developercommunity.visualstudio.com/content/problem/211134/unsigned-integer-overflows-in-constexpr-functionsa.html
#pragma warning( disable : 4307 )
    switch (ENTITY::GET_ENTITY_MODEL(g_playerVehicle)) {
    case joaat("TOWTRUCK"):
    case joaat("TOWTRUCK2"):
        if (!VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(g_playerVehicle))
            CONTROLS::DISABLE_CONTROL_ACTION(1, ControlVehicleMoveUpDown, true);
        CONTROLS::DISABLE_CONTROL_ACTION(1, ControlVehicleMoveLeftRight, true);
        break;
    case joaat("deluxo"):
    case joaat("stromberg"):
        if (VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(g_playerVehicle)) {
            CONTROLS::DISABLE_CONTROL_ACTION(1, ControlVehicleMoveUpDown, true);
            CONTROLS::DISABLE_CONTROL_ACTION(1, ControlVehicleMoveLeftRight, true);
        }
    case joaat("oppressor"):
    case joaat("oppressor2"):
        if (VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(g_playerVehicle))
            CONTROLS::DISABLE_CONTROL_ACTION(1, ControlVehicleMoveLeftRight, true);
        break;
    default:
        CONTROLS::DISABLE_CONTROL_ACTION(1, ControlVehicleMoveUpDown, true);
        CONTROLS::DISABLE_CONTROL_ACTION(1, ControlVehicleMoveLeftRight, true);
        break;
    }

    // Both need to be set, top one with radian limit
    g_ext.SetSteeringAngle(g_playerVehicle, desiredHeading);
    g_ext.SetSteeringInputAngle(g_playerVehicle, desiredHeading * (1.0f / limitRadians));
}
