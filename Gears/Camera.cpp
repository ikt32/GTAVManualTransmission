#include "Camera.h"
#include "ScriptSettings.hpp"
#include "VehicleData.hpp"
#include "Input/CarControls.hpp"
#include "ScriptUtils.h"
#include "Util/MathExt.h"
#include <inc/enums.h>
#include <inc/natives.h>

#include "Util/UIUtils.h"
#include <fmt/format.h>

extern Vehicle g_playerVehicle;
extern Ped g_playerPed;
extern ScriptSettings g_settings;
extern CarControls g_controls;
extern VehicleData g_vehData;

// TODO: Mouse look / Re-centering
// TODO: Fix 3 m/s camera transition with movement follow

namespace {
    Cam cameraHandle = -1;
    Vector3 camRot{};

    // Distance in meters to lean over to the center
    // when looking back
    const float lookLeanDist = 0.25f;
}

void cancelCam() {
    if (CAM::DOES_CAM_EXIST(cameraHandle)) {
        CAM::RENDER_SCRIPT_CAMS(false, false, 0, true, false);
        CAM::SET_CAM_ACTIVE(cameraHandle, false);
        CAM::DESTROY_CAM(cameraHandle, false);
        cameraHandle = -1;
    }
}

void initCam() {
    auto cV = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(g_playerVehicle, 0.0f, 2.0f, 0.5f);
    cameraHandle = CAM::CREATE_CAM_WITH_PARAMS(
        "DEFAULT_SCRIPTED_CAMERA",
        cV.x, cV.y, cV.z,
        0, 0, 0,
        g_settings.Misc.Camera.FOV, 1, 2);
}

void FPVCam::Update() {
    bool enable = g_settings.Misc.Camera.Enable;
    bool fpv = CAM::GET_FOLLOW_VEHICLE_CAM_VIEW_MODE() == 4;
    bool inCar = Util::VehicleAvailable(g_playerVehicle, g_playerPed);

    if (!enable || !fpv || !inCar) {
        cancelCam();
        return;
    }

    // Initialize camera
    if (cameraHandle == -1) {
        initCam();

        CAM::SET_CAM_ACTIVE(cameraHandle, true);
        CAM::RENDER_SCRIPT_CAMS(true, false, 0, true, false);
    }

    auto rot = ENTITY::GET_ENTITY_ROTATION(g_playerPed, 0);

    camRot.x = lerp(camRot.x, 90.0f * -CONTROLS::GET_CONTROL_NORMAL(0, eControl::ControlLookUpDown),
        1.0f - pow(g_settings.CustomSteering.CenterTime, GAMEPLAY::GET_FRAME_TIME()));

    float lookLeftRight = CONTROLS::GET_CONTROL_NORMAL(0, eControl::ControlLookLeftRight);
    bool lookingIntoGlass = false;

    if (g_vehData.mIsRhd && lookLeftRight > 0.01f) {
        lookingIntoGlass = true;
    }

    if (!g_vehData.mIsRhd && lookLeftRight < -0.01f) {
        lookingIntoGlass = true;
    }

    if (CONTROLS::GET_CONTROL_NORMAL(0, eControl::ControlVehicleLookBehind) != 0.0f) {
        float lookBackAngle = -179.0f; // Look over right shoulder
        if (g_vehData.mIsRhd) {
            lookBackAngle = 179.0f; // Look over left shoulder
        }
        camRot.z = lerp(camRot.z, lookBackAngle,
            1.0f - pow(g_settings.Misc.Camera.LookTime, GAMEPLAY::GET_FRAME_TIME()));
    }
    else {
        float maxAngle = lookingIntoGlass ? 135.0f : 179.0f;
        camRot.z = lerp(camRot.z, maxAngle * -lookLeftRight,
            1.0f - pow(g_settings.Misc.Camera.LookTime, GAMEPLAY::GET_FRAME_TIME()));
    }

    float directionLookAngle = 0.0f;
    if (g_settings.Misc.Camera.FollowMovement) {
        Vector3 speedVector = ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true);
        if (speedVector.y > 3.0f) {
            Vector3 target = Normalize(speedVector);
            float travelDir = atan2(target.y, target.x) - static_cast<float>(M_PI) / 2.0f;
            if (travelDir > static_cast<float>(M_PI) / 2.0f) {
                travelDir -= static_cast<float>(M_PI);
            }
            if (travelDir < -static_cast<float>(M_PI) / 2.0f) {
                travelDir += static_cast<float>(M_PI);
            }

            Vector3 rotationVelocity = ENTITY::GET_ENTITY_ROTATION_VELOCITY(g_playerVehicle);

            directionLookAngle = - (rad2deg(travelDir) / 2.0f + rad2deg(rotationVelocity.z) / 4.0f);
        }
    }

    float offsetX = 0.0f;
    float offsetY = g_settings.Misc.Camera.OffsetForward;

    if (!lookingIntoGlass) {
        // Left
        if (camRot.z > 85.0f) {
            offsetX = map(camRot.z, 85.0f, 180.0f, 0.0f, -lookLeanDist);
            offsetX = std::clamp(offsetX, -lookLeanDist, 0.0f);
        }
        // Right
        if (camRot.z < -85.0f) {
            offsetX = map(camRot.z, -85.0f, -180.0f, 0.0f, lookLeanDist);
            offsetX = std::clamp(offsetX, 0.0f, lookLeanDist);
        }
    }

    // 0x796E skel_head id
    CAM::ATTACH_CAM_TO_PED_BONE(cameraHandle, g_playerPed, 0x796E,
        offsetX,
        offsetY,
        g_settings.Misc.Camera.OffsetHeight, true);

    CAM::SET_CAM_ROT(
        cameraHandle, 
        rot.x + camRot.x, 
        rot.y, 
        rot.z + camRot.z - directionLookAngle,
        0);

    CAM::SET_CAM_FOV(cameraHandle, g_settings.Misc.Camera.FOV);
}
