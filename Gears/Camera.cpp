// I wish I never considered touching the camera.

#include "Camera.h"
#include "Compatibility.h"
#include "ScriptSettings.hpp"
#include "VehicleData.hpp"
#include "Input/CarControls.hpp"
#include "ScriptUtils.h"
#include "Util/MathExt.h"
#include "Util/Timer.h"
#include "Memory/NativeMemory.hpp"
#include "Memory/Versions.h"
#include "ManualTransmission.h"

#include <inc/enums.h>
#include <inc/natives.h>
#include <fmt/format.h>

extern Vehicle g_playerVehicle;
extern Ped g_playerPed;
extern ScriptSettings g_settings;
extern CarControls g_controls;
extern VehicleData g_vehData;
extern eGameVersion g_gameVersion;
extern VehiclePeripherals g_peripherals;

namespace {
    Cam cameraHandle = -1;
    Vector3 camRot{};

    // Distance in meters to lean over to the center when looking back
    const float lookLeanCenterDist = 0.25f;

    // Distance in meters to lean forward when looking back/sideways
    const float lookLeanFrontDist = 0.05f;

    Timer lastMouseLookInputTimer(500);

    // Accumulated values for mouse look
    float lookUpDownAcc = 0.0f;
    float lookLeftRightAcc = 0.0f;

    Vector3 driverHeadOffsetStatic{};
    bool headRemoved = false;

    // 1290+?
    unsigned fpvCamOffsetXOffset = 0x450;
}

namespace FPVCam {
    void initCam();
    void updateDriverHeadOffset();
}

void updateControllerLook(bool& lookingIntoGlass);
void updateMouseLook(bool& lookingIntoGlass);
void updateWheelLook(bool& lookingIntoGlass);

void FPVCam::InitOffsets() {
    if (g_gameVersion < G_VER_1_0_1290_1_STEAM) {
        fpvCamOffsetXOffset = 0x450 - 40;
    }
}

void FPVCam::updateDriverHeadOffset() {
    Vector3 dHeadPos = PED::GET_PED_BONE_COORDS(g_playerPed, 0x796E, 0.0f, 0.0f, 0.0f);
    Vector3 dHeadOff = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(g_playerVehicle,
        dHeadPos.x, dHeadPos.y, dHeadPos.z);
    driverHeadOffsetStatic = dHeadOff;
}

void FPVCam::CancelCam() {
    if (CAM::DOES_CAM_EXIST(cameraHandle)) {
        CAM::RENDER_SCRIPT_CAMS(false, false, 0, true, false, 0);
        CAM::SET_CAM_ACTIVE(cameraHandle, false);
        CAM::DESTROY_CAM(cameraHandle, false);
        cameraHandle = -1;
        if (g_settings.Misc.Camera.RemoveHead) {
            HideHead(false);
        }
        HUD::UNLOCK_MINIMAP_ANGLE();
    }
}

void FPVCam::HideHead(bool remove) {
    if (Dismemberment::Available()) {
        if (remove) {
            Dismemberment::AddBoneDraw(g_playerPed, 0x796E, -1);
            headRemoved = true;
        }
        else {
            Dismemberment::RemoveBoneDraw(g_playerPed);
            headRemoved = false;
        }
    }
}

void FPVCam::initCam() {
    auto cV = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(g_playerVehicle, 0.0f, 2.0f, 0.5f);
    cameraHandle = CAM::CREATE_CAM_WITH_PARAMS(
        "DEFAULT_SCRIPTED_CAMERA",
        cV.x, cV.y, cV.z,
        0, 0, 0,
        g_settings.Misc.Camera.FOV, 1, 2);

    // This should be named something else, like "_SET_VEHICLE_SPEED_JITTER" or something.
    // Thanks for finding it, Jitnaught!
    VEHICLE::_SET_CAR_HIGH_SPEED_BUMP_SEVERITY_MULTIPLIER(0.0f);

    if (g_settings.Misc.Camera.RemoveHead && Dismemberment::Available()) {
        HideHead(true);
    }
}

void FPVCam::Update() {
    bool enable = g_settings.Misc.Camera.Enable;
    bool fpv = CAM::GET_FOLLOW_VEHICLE_CAM_VIEW_MODE() == 4;
    bool inCar = Util::VehicleAvailable(g_playerVehicle, g_playerPed);
    bool hasControl = PLAYER::IS_PLAYER_CONTROL_ON(PLAYER::PLAYER_ID()) &&
        PED::IS_PED_SITTING_IN_VEHICLE(g_playerPed, g_playerVehicle);
    bool lookingIntoGlass = false;

    bool aiming = PAD::IS_CONTROL_PRESSED(2, ControlVehicleAim);
    if (g_vehData.mDomain == VehicleDomain::Air)
        aiming = false;

    bool bikeSeat = false;
    if (g_vehData.mClass == VehicleClass::Bike ||
        g_vehData.mClass == VehicleClass::Quad ||
        g_vehData.mClass == VehicleClass::Bicycle) {
        bikeSeat = true;
    }

    bool disableBike = g_settings.Misc.Camera.Bike.Disable && bikeSeat;

    if (!enable || !fpv || !inCar || !hasControl || aiming || disableBike) {
        CancelCam();
        return;
    }

    PAD::DISABLE_CONTROL_ACTION(0, eControl::ControlVehicleCinCam, true);

    // Initialize camera
    if (cameraHandle == -1) {
        initCam();

        CAM::SET_CAM_ACTIVE(cameraHandle, true);
        CAM::RENDER_SCRIPT_CAMS(true, false, 0, true, false, 0);

        if (Length(driverHeadOffsetStatic) == 0.0f || 
            Math::Near(Length(g_vehData.mVelocity), 0.0f, 0.01f) &&
            Math::Near(g_vehData.mSteeringAngle, 0.0f, 0.01f))
        // Just so we have a valid-ish offset
        updateDriverHeadOffset();
    }

    if (MT_LookingLeft() || MT_LookingRight()) {
        updateWheelLook(lookingIntoGlass);
    }
    // Mouse look
    else if (PAD::_IS_INPUT_DISABLED(2) == TRUE) {
        updateMouseLook(lookingIntoGlass);
    }
    // Controller look
    else {
        updateControllerLook(lookingIntoGlass);
    }

    float directionLookAngle = 0.0f;
    if (g_settings.Misc.Camera.FollowMovement) {
        Vector3 speedVector = ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true);

        Vector3 target = Normalize(speedVector);
        float travelDir = atan2(target.y, target.x) - static_cast<float>(M_PI) / 2.0f;
        if (travelDir > static_cast<float>(M_PI) / 2.0f) {
            travelDir -= static_cast<float>(M_PI);
        }
        if (travelDir < -static_cast<float>(M_PI) / 2.0f) {
            travelDir += static_cast<float>(M_PI);
        }

        Vector3 rotationVelocity = ENTITY::GET_ENTITY_ROTATION_VELOCITY(g_playerVehicle);

        float velComponent = travelDir * g_settings.Misc.Camera.MovementMultVel;
        float rotComponent = rotationVelocity.z * g_settings.Misc.Camera.MovementMultRot;
        float totalMove = std::clamp(velComponent + rotComponent,
            -deg2rad(g_settings.Misc.Camera.MovementCap),
             deg2rad(g_settings.Misc.Camera.MovementCap));
        directionLookAngle = -rad2deg(totalMove);

        if (speedVector.y < 3.0f) {
            directionLookAngle = map(speedVector.y, 0.0f, 3.0f, 0.0f, directionLookAngle);
        }
    }

    float offsetX = 0.0f;
    float offsetY = g_settings.Misc.Camera.OffsetForward;

    {
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
            frontLean = std::clamp(frontLean, 0.0f, lookLeanFrontDist);
            offsetY += frontLean;
        }
        // Right
        if (camRot.z < -85.0f) {
            float frontLean = map(camRot.z, -85.0f, -180.0f, 0.0f, lookLeanFrontDist);
            frontLean = std::clamp(frontLean, 0.0f, lookLeanFrontDist);
            offsetY += frontLean;
        }
    }

    bool wearingHelmet = PED::IS_PED_WEARING_HELMET(g_playerPed);

    if (wearingHelmet || !headRemoved) {
        // FPV driving gameplay is 0.149
        // Add 2.6cm so the helmet or head model are entirely clipped out
        CAM::SET_CAM_NEAR_CLIP(cameraHandle, 0.175f);
    }
    else {
        // Same as FPV walking gameplay
        CAM::SET_CAM_NEAR_CLIP(cameraHandle, 0.05f);
    }
    // 10km in city, 15km outside
    CAM::SET_CAM_FAR_CLIP(cameraHandle, 12500.0f);

    auto attachId = g_settings.Misc.Camera.AttachId;
    float pitch = g_settings.Misc.Camera.Pitch;
    float fov = g_settings.Misc.Camera.FOV;
    Vector3 additionalOffset{};

    if (bikeSeat) {
        attachId = g_settings.Misc.Camera.Bike.AttachId;
        pitch = g_settings.Misc.Camera.Bike.Pitch;
        fov = g_settings.Misc.Camera.Bike.FOV;
        additionalOffset.x = g_settings.Misc.Camera.Bike.OffsetSide;
        additionalOffset.y = g_settings.Misc.Camera.Bike.OffsetForward;
        additionalOffset.z = g_settings.Misc.Camera.Bike.OffsetHeight;
    }

    switch(attachId) {
        case 2: {
            Hash modelHash = ENTITY::GET_ENTITY_MODEL(g_playerVehicle);
            int index = 0xFFFF;
            uintptr_t pModelInfo = mem::GetModelInfo(modelHash, &index);

            // offset from seat?
            // These offsets don't seem very version-sturdy. Oh well, hope R* doesn't knock em over.
            Vector3 camSeatOffset;
            camSeatOffset.x = *reinterpret_cast<float*>(pModelInfo + fpvCamOffsetXOffset);
            camSeatOffset.y = *reinterpret_cast<float*>(pModelInfo + fpvCamOffsetXOffset + 4);
            camSeatOffset.z = *reinterpret_cast<float*>(pModelInfo + fpvCamOffsetXOffset + 8);
            float rollbarOffset = 0.0f;

            if (VEHICLE::GET_VEHICLE_MOD(g_playerVehicle, eVehicleMod::VehicleModFrame) != -1)
                rollbarOffset = *reinterpret_cast<float*>(pModelInfo + fpvCamOffsetXOffset + 0x30);

            Vector3 seatCoords = ENTITY::GET_WORLD_POSITION_OF_ENTITY_BONE(
                g_playerVehicle, ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(
                    g_playerVehicle, bikeSeat ? "seat_f" : "seat_dside_f"));
            Vector3 seatOffset = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(
                g_playerVehicle, seatCoords.x, seatCoords.y, seatCoords.z);

            if (bikeSeat) {
                Vector3 headBoneCoord = PED::GET_PED_BONE_COORDS(g_playerPed, 0x796E, 0.0f, 0.0f, 0.0f);
                Vector3 headBoneOff = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(
                    g_playerPed, headBoneCoord.x, headBoneCoord.y, headBoneCoord.z);
                // SKEL_Spine_Root
                Vector3 spinebaseCoord = PED::GET_PED_BONE_COORDS(g_playerPed, 0xe0fd, 0.0f, 0.0f, 0.0f);
                Vector3 spinebaseOff = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(
                    g_playerPed, spinebaseCoord.x, spinebaseCoord.y, spinebaseCoord.z);
                Vector3 offHead = headBoneOff - spinebaseOff;

                camSeatOffset = camSeatOffset + offHead;
            }
            
            CAM::ATTACH_CAM_TO_ENTITY(cameraHandle, g_playerVehicle,
                seatOffset.x + camSeatOffset.x + additionalOffset.x + g_settings.Misc.Camera.OffsetSide + offsetX,
                seatOffset.y + camSeatOffset.y + additionalOffset.y + g_settings.Misc.Camera.OffsetForward + offsetY,
                seatOffset.z + camSeatOffset.z + additionalOffset.z + g_settings.Misc.Camera.OffsetHeight + rollbarOffset, true);
            break;
        }
        case 1: {
            if (Math::Near(Length(g_vehData.mVelocity), 0.0f, 0.01f) &&
                Math::Near(g_vehData.mSteeringAngle, 0.0f, 0.01f)) {
                updateDriverHeadOffset();
            }

            CAM::ATTACH_CAM_TO_ENTITY(cameraHandle, g_playerVehicle,
                driverHeadOffsetStatic.x + additionalOffset.x + g_settings.Misc.Camera.OffsetSide + offsetX,
                driverHeadOffsetStatic.y + additionalOffset.y + g_settings.Misc.Camera.OffsetForward + offsetY,
                driverHeadOffsetStatic.z + additionalOffset.z + g_settings.Misc.Camera.OffsetHeight, true);
            break;
        }
        default:
        case 0: {
            // 0x796E skel_head id
            CAM::ATTACH_CAM_TO_PED_BONE(cameraHandle, g_playerPed, 0x796E,
                additionalOffset.x + g_settings.Misc.Camera.OffsetSide + offsetX,
                additionalOffset.y + g_settings.Misc.Camera.OffsetForward + offsetY,
                additionalOffset.z + g_settings.Misc.Camera.OffsetHeight, true);
            break;
        }
    }

    auto rot = ENTITY::GET_ENTITY_ROTATION(g_playerPed, 0);
    float pitchLookComp = -rot.x * 2.0f * abs(camRot.z) / 180.0f;
    float rollLookComp = -rot.y * 2.0f * abs(camRot.z) / 180.0f;
    float rollPitchComp = sin(deg2rad(camRot.z)) * rot.y;

    CAM::SET_CAM_ROT(
        cameraHandle,
        rot.x + camRot.x + pitch + pitchLookComp + rollPitchComp,
        rot.y + rollLookComp,
        rot.z + camRot.z - directionLookAngle,
        0);

    CAM::SET_CAM_FOV(cameraHandle, fov);

    float minimapAngle = rot.z + camRot.z - directionLookAngle;
    if (minimapAngle > 360.0f) minimapAngle = minimapAngle - 360.0f;
    if (minimapAngle < 0.0f) minimapAngle = minimapAngle + 360.0f;

    HUD::LOCK_MINIMAP_ANGLE(static_cast<int>(minimapAngle));
}

void updateControllerLook(bool& lookingIntoGlass) {
    float lookLeftRight = PAD::GET_CONTROL_NORMAL(0, eControl::ControlLookLeftRight);
    float lookUpDown = PAD::GET_CONTROL_NORMAL(0, eControl::ControlLookUpDown);

    if (g_vehData.mIsRhd && lookLeftRight > 0.01f) {
        lookingIntoGlass = true;
    }

    if (!g_vehData.mIsRhd && lookLeftRight < -0.01f) {
        lookingIntoGlass = true;
    }

    float maxAngle = lookingIntoGlass ? 135.0f : 179.0f;

    if (lookingIntoGlass) {
        if (abs(lookLeftRight * 179.0f) > maxAngle) {
            lookLeftRight = sgn(lookLeftRight) * (maxAngle / 179.0f);
        }
    }

    camRot.x = lerp(camRot.x, 90.0f * -lookUpDown,
        1.0f - pow(g_settings.Misc.Camera.LookTime, MISC::GET_FRAME_TIME()));

    if (PAD::GET_CONTROL_NORMAL(0, eControl::ControlVehicleLookBehind) != 0.0f) {
        float lookBackAngle = -179.0f; // Look over right shoulder
        if (g_vehData.mIsRhd) {
            lookBackAngle = 179.0f; // Look over left shoulder
        }
        camRot.z = lerp(camRot.z, lookBackAngle,
            1.0f - pow(g_settings.Misc.Camera.LookTime, MISC::GET_FRAME_TIME()));
    }
    else {
        // Manual look
        camRot.z = lerp(camRot.z, 179.0f * -lookLeftRight,
            1.0f - pow(g_settings.Misc.Camera.LookTime, MISC::GET_FRAME_TIME()));
    }
}

void updateMouseLook(bool& lookingIntoGlass) {
    float lookLeftRight = 
        PAD::GET_CONTROL_NORMAL(0, eControl::ControlLookLeftRight) * g_settings.Misc.Camera.MouseSensitivity;
    float lookUpDown = 
        PAD::GET_CONTROL_NORMAL(0, eControl::ControlLookUpDown) * g_settings.Misc.Camera.MouseSensitivity;

    bool lookBehind = PAD::GET_CONTROL_NORMAL(0, eControl::ControlVehicleLookBehind) != 0.0f;

    if (g_vehData.mIsRhd && lookLeftRightAcc > 0.01f) {
        lookingIntoGlass = true;
    }

    if (!g_vehData.mIsRhd && lookLeftRightAcc < -0.01f) {
        lookingIntoGlass = true;
    }

    float maxAngle = lookingIntoGlass ? 135.0f : 179.0f;

    // Re-center on no input
    if (lookLeftRight != 0.0f || lookUpDown != 0.0f) {
        lastMouseLookInputTimer.Reset(g_settings.Misc.Camera.MouseCenterTimeout);
    }

    if (lastMouseLookInputTimer.Expired() && Length(g_vehData.mVelocity) > 1.0f && !lookBehind) {
        lookUpDownAcc = lerp(lookUpDownAcc, 0.0f,
            1.0f - pow(g_settings.Misc.Camera.MouseLookTime, MISC::GET_FRAME_TIME()));
        lookLeftRightAcc = lerp(lookLeftRightAcc, 0.0f,
            1.0f - pow(g_settings.Misc.Camera.MouseLookTime, MISC::GET_FRAME_TIME()));
    }
    else {
        lookUpDownAcc += lookUpDown;

        if (lookingIntoGlass) {
            if (sgn(lookLeftRight) != sgn(lookLeftRightAcc) || abs(camRot.z) + abs(lookLeftRight * 179.0f) < maxAngle) {
                lookLeftRightAcc += lookLeftRight;
            }

            if (abs(lookLeftRightAcc * 179.0f) > maxAngle) {
                lookLeftRightAcc = sgn(lookLeftRightAcc) * (maxAngle / 179.0f);
            }
        }
        else {
            lookLeftRightAcc += lookLeftRight;
        }

        lookUpDownAcc = std::clamp(lookUpDownAcc, -1.0f, 1.0f);
        lookLeftRightAcc = std::clamp(lookLeftRightAcc, -1.0f, 1.0f);
    }

    camRot.x = lerp(camRot.x, 90 * -lookUpDownAcc,
        1.0f - pow(g_settings.Misc.Camera.MouseLookTime, MISC::GET_FRAME_TIME()));

    // Override any camRot.z changes while looking back 
    if (lookBehind) {
        float lookBackAngle = -179.0f; // Look over right shoulder
        if (g_vehData.mIsRhd) {
            lookBackAngle = 179.0f; // Look over left shoulder
        }
        camRot.z = lerp(camRot.z, lookBackAngle,
            1.0f - pow(g_settings.Misc.Camera.MouseLookTime, MISC::GET_FRAME_TIME()));
    }
    else {
        camRot.z = lerp(camRot.z, 179.0f * -lookLeftRightAcc,
            1.0f - pow(g_settings.Misc.Camera.MouseLookTime, MISC::GET_FRAME_TIME()));
    }
}

void updateWheelLook(bool& lookingIntoGlass) {
    if (MT_LookingLeft() && MT_LookingRight()) {
        if (g_vehData.mIsRhd && g_peripherals.LookBackRShoulder) {
            lookingIntoGlass = true;
        }
        if (!g_vehData.mIsRhd && !g_peripherals.LookBackRShoulder) {
            lookingIntoGlass = true;
        }

        float maxAngle = lookingIntoGlass ? 135.0f : 179.0f;
        float lookBackAngle = g_peripherals.LookBackRShoulder ? -1.0f * maxAngle : maxAngle;
        camRot.z = lerp(camRot.z, lookBackAngle,
            1.0f - pow(g_settings.Misc.Camera.MouseLookTime, MISC::GET_FRAME_TIME()));
    }
    else {
        float angle;
        if (MT_LookingLeft()) {
            angle = 90.0f;
        }
        else {
            angle = -90.0f;
        }
        camRot.z = lerp(camRot.z, angle,
            1.0f - pow(g_settings.Misc.Camera.MouseLookTime, MISC::GET_FRAME_TIME()));
    }
}