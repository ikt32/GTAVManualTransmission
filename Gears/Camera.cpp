#include "Camera.h"
#include "ScriptSettings.hpp"
#include "VehicleData.hpp"
#include "Input/CarControls.hpp"
#include "ScriptUtils.h"
#include "Util/MathExt.h"
#include "Util/Timer.h"

#include <inc/enums.h>
#include <inc/natives.h>
#include <fmt/format.h>

extern Vehicle g_playerVehicle;
extern Ped g_playerPed;
extern ScriptSettings g_settings;
extern CarControls g_controls;
extern VehicleData g_vehData;

namespace {
    Cam cameraHandle = -1;
    Vector3 camRot{};

    // Distance in meters to lean over to the center when looking back
    const float lookLeanCenterDist = 0.25f;

    // Distance in meters to lean forward when looking back/sideways
    const float lookLeanFrontDist = 0.15f;

    Timer lastMouseLookInputTimer(500);

    // Accumulated values for mouse look
    float lookUpDownAcc = 0.0f;
    float lookLeftRightAcc = 0.0f;
}

void updateControllerLook(bool& lookingIntoGlass);
void updateMouseLook(bool& lookingIntoGlass);

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
    bool lookingIntoGlass;

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

    CAM::SET_CAM_AFFECTS_AIMING(cameraHandle, true);

    if (CONTROLS::IS_CONTROL_PRESSED(2, ControlVehicleAim)) {
        UI::SHOW_HUD_COMPONENT_THIS_FRAME(14);
    }

    // Mouse look
    if (CONTROLS::_IS_INPUT_DISABLED(2) == TRUE) {
        updateMouseLook(lookingIntoGlass);
    }
    // Controller look
    else {
        updateControllerLook(lookingIntoGlass);
    }

    float directionLookAngle = 0.0f;
    if (g_settings.Misc.Camera.FollowMovement) {
        Vector3 speedVector = ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true);
        if (speedVector.y > 0.0f) {
            Vector3 target = Normalize(speedVector);
            float travelDir = atan2(target.y, target.x) - static_cast<float>(M_PI) / 2.0f;
            if (travelDir > static_cast<float>(M_PI) / 2.0f) {
                travelDir -= static_cast<float>(M_PI);
            }
            if (travelDir < -static_cast<float>(M_PI) / 2.0f) {
                travelDir += static_cast<float>(M_PI);
            }

            Vector3 rotationVelocity = ENTITY::GET_ENTITY_ROTATION_VELOCITY(g_playerVehicle);

            directionLookAngle = -(rad2deg(travelDir) / 2.0f + rad2deg(rotationVelocity.z) / 4.0f);

            if (Length(speedVector) < 3.0f) {
                directionLookAngle = map(speedVector.y, 0.0f, 3.0f, 0.0f, directionLookAngle);
            }
        }
    }

    float offsetX = 0.0f;
    float offsetY = g_settings.Misc.Camera.OffsetForward;

    if (!lookingIntoGlass) {
        // Left
        if (camRot.z > 85.0f) {
            offsetX = map(camRot.z, 85.0f, 180.0f, 0.0f, -lookLeanCenterDist);
            offsetX = std::clamp(offsetX, -lookLeanCenterDist, 0.0f);
        }
        // Right
        if (camRot.z < -85.0f) {
            offsetX = map(camRot.z, -85.0f, -180.0f, 0.0f, lookLeanCenterDist);
            offsetX = std::clamp(offsetX, 0.0f, lookLeanCenterDist);
        }
    }

    {
        // Left
        if (camRot.z > 85.0f) {
            float frontLean = map(camRot.z, 85.0f, 180.0f, 0.0f, lookLeanFrontDist);
            frontLean = std::clamp(frontLean, lookLeanFrontDist, 0.0f);
            offsetY += frontLean;
        }
        // Right
        if (camRot.z < -85.0f) {
            float frontLean = map(camRot.z, -85.0f, -180.0f, 0.0f, lookLeanFrontDist);
            frontLean = std::clamp(frontLean, lookLeanFrontDist, 0.0f);
            offsetY += frontLean;
        }
    }

    // 0x796E skel_head id
    CAM::ATTACH_CAM_TO_PED_BONE(cameraHandle, g_playerPed, 0x796E,
        offsetX,
        offsetY,
        g_settings.Misc.Camera.OffsetHeight, true);

    auto rot = ENTITY::GET_ENTITY_ROTATION(g_playerPed, 0);
    CAM::SET_CAM_ROT(
        cameraHandle,
        rot.x + camRot.x + g_settings.Misc.Camera.Pitch,
        rot.y,
        rot.z + camRot.z - directionLookAngle,
        0);

    CAM::SET_CAM_FOV(cameraHandle, g_settings.Misc.Camera.FOV);

}

void updateControllerLook(bool& lookingIntoGlass) {
    float lookLeftRight = CONTROLS::GET_CONTROL_NORMAL(0, eControl::ControlLookLeftRight);
    float lookUpDown = CONTROLS::GET_CONTROL_NORMAL(0, eControl::ControlLookUpDown);

    if (g_vehData.mIsRhd && lookLeftRight > 0.01f) {
        lookingIntoGlass = true;
    }

    if (!g_vehData.mIsRhd && lookLeftRight < -0.01f) {
        lookingIntoGlass = true;
    }

    camRot.x = lerp(camRot.x, 90.0f * -lookUpDown,
        1.0f - pow(g_settings.Misc.Camera.LookTime, GAMEPLAY::GET_FRAME_TIME()));

    if (CONTROLS::GET_CONTROL_NORMAL(0, eControl::ControlVehicleLookBehind) != 0.0f) {
        float lookBackAngle = -179.0f; // Look over right shoulder
        if (g_vehData.mIsRhd) {
            lookBackAngle = 179.0f; // Look over left shoulder
        }
        camRot.z = lerp(camRot.z, lookBackAngle,
            1.0f - pow(g_settings.Misc.Camera.LookTime, GAMEPLAY::GET_FRAME_TIME()));
    }
    else {
        // Manual look
        float maxAngle = lookingIntoGlass ? 135.0f : 179.0f;
        camRot.z = lerp(camRot.z, maxAngle * -lookLeftRight,
            1.0f - pow(g_settings.Misc.Camera.LookTime, GAMEPLAY::GET_FRAME_TIME()));
    }
}

void updateMouseLook(bool& lookingIntoGlass) {
    float lookLeftRight = 
        CONTROLS::GET_CONTROL_NORMAL(0, eControl::ControlLookLeftRight) * g_settings.Misc.Camera.MouseSensitivity;
    float lookUpDown = 
        CONTROLS::GET_CONTROL_NORMAL(0, eControl::ControlLookUpDown) * g_settings.Misc.Camera.MouseSensitivity;

    bool lookBehind = CONTROLS::GET_CONTROL_NORMAL(0, eControl::ControlVehicleLookBehind) != 0.0f;

    if (g_vehData.mIsRhd && lookLeftRightAcc > 0.01f) {
        lookingIntoGlass = true;
    }

    if (!g_vehData.mIsRhd && lookLeftRightAcc < -0.01f) {
        lookingIntoGlass = true;
    }

    // Re-center on no input
    if (lookLeftRight != 0.0f || lookUpDown != 0.0f) {
        lastMouseLookInputTimer.Reset(g_settings.Misc.Camera.MouseCenterTimeout);
    }

    if (lastMouseLookInputTimer.Expired() && Length(g_vehData.mVelocity) > 1.0f && !lookBehind) {
        lookUpDownAcc = lerp(lookUpDownAcc, 0.0f,
            1.0f - pow(g_settings.Misc.Camera.LookTime, GAMEPLAY::GET_FRAME_TIME()));
        lookLeftRightAcc = lerp(lookLeftRightAcc, 0.0f,
            1.0f - pow(g_settings.Misc.Camera.LookTime, GAMEPLAY::GET_FRAME_TIME()));
    }
    else {
        lookUpDownAcc += lookUpDown;
        lookLeftRightAcc += lookLeftRight;

        lookUpDownAcc = std::clamp(lookUpDownAcc, -1.0f, 1.0f);
        lookLeftRightAcc = std::clamp(lookLeftRightAcc, -1.0f, 1.0f);
    }

    camRot.x = lerp(camRot.x, 90 * -lookUpDownAcc,
        1.0f - pow(g_settings.Misc.Camera.MouseLookTime, GAMEPLAY::GET_FRAME_TIME()));

    // Override any camRot.z changes while looking back 
    if (lookBehind) {
        float lookBackAngle = -179.0f; // Look over right shoulder
        if (g_vehData.mIsRhd) {
            lookBackAngle = 179.0f; // Look over left shoulder
        }
        camRot.z = lerp(camRot.z, lookBackAngle,
            1.0f - pow(g_settings.Misc.Camera.LookTime, GAMEPLAY::GET_FRAME_TIME()));
    }
    else {
        float maxAngle = lookingIntoGlass ? 135.0f : 179.0f;
        camRot.z = lerp(camRot.z, maxAngle * -lookLeftRightAcc,
            1.0f - pow(g_settings.Misc.Camera.MouseLookTime, GAMEPLAY::GET_FRAME_TIME()));
    }
}