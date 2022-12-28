// I wish I never considered touching the camera.

#include "Camera.h"
#include "Compatibility.h"
#include "ScriptSettings.hpp"
#include "VehicleData.hpp"
#include "Input/CarControls.hpp"
#include "Util/ScriptUtils.h"
#include "Util/MathExt.h"
#include "Util/Timer.h"
#include "Memory/NativeMemory.hpp"
#include "Memory/Versions.h"
#include "ManualTransmission.h"

#include <inc/enums.h>
#include <inc/natives.h>
#include <fmt/format.h>

#include "Util/UIUtils.h"

using VExt = VehicleExtensions;

extern Vehicle g_playerVehicle;
extern Ped g_playerPed;
extern ScriptSettings g_settings;
extern CarControls g_controls;
extern VehicleData g_vehData;
extern eGameVersion g_gameVersion;
extern VehiclePeripherals g_peripherals;

enum class ePedPropPosition {
    AnchorHead = 0,
    AnchorEyes,
    AnchorEars,
    AnchorMouth,
    AnchorLeftHand,
    AnchorRightHand,
    AnchorLeftWrist,
    AnchorRightWrist,
    AnchorHip
};

namespace {
    Cam cameraHandle = -1;
    Vector3 camRot{};

    // Distance in meters to lean over to the center when looking back
    const float lookLeanCenterDist = 0.25f;

    // Distance in meters to lean forward when looking back/sideways
    const float lookLeanFrontDist = 0.05f;

    // Distance in meters to peek up when looking back
    const float lookLeanUpDist = 0.08f;

    Timer lastMouseLookInputTimer(500);

    // Accumulated values for mouse look
    float lookUpDownAcc = 0.0f;
    float lookLeftRightAcc = 0.0f;

    bool headRemoved = false;

    // 1290+?
    unsigned fpvCamOffsetXOffset = 0x450;

    // rotation camera movement
    float directionLookAngle = 0.0f;

    // forward camera movement
    float accelMoveFwd = 0.0f;
    float accelMoveLat = 0.0f;
    float accelMoveVert = 0.0f;
    float accelPitchDeg = 0.0f;

    float lowpassPitch = 0.0f;

    int savedHeadProp = -1;
    int savedHeadPropTx = -1;
    int savedEyesProp = -1;
    int savedEyesPropTx = -1;
}

namespace FPVCam {
    void initCam();
    void updateControllerLook(bool& lookingIntoGlass);
    void updateMouseLook(bool& lookingIntoGlass);
    void updateWheelLook(bool& lookingIntoGlass);
    void updateRotationCameraMovement(VehicleConfig::SMovement& movement);
    void updateLongitudinalCameraMovement(VehicleConfig::SMovement& movement);
    void updateLateralCameraMovement(VehicleConfig::SMovement& movement);
    void updateVerticalCameraMovement(VehicleConfig::SMovement& movement);
    void updatePitchCameraMovement(VehicleConfig::SMovement& movement);
}
void FPVCam::InitOffsets() {
    if (g_gameVersion < G_VER_1_0_1290_1_STEAM) {
        fpvCamOffsetXOffset = 0x450 - 40;
    }
}

void FPVCam::CancelCam() {
    if (CAM::DOES_CAM_EXIST(cameraHandle)) {
        CAM::RENDER_SCRIPT_CAMS(false, false, 0, true, false, 0);
        CAM::SET_CAM_ACTIVE(cameraHandle, false);
        CAM::DESTROY_CAM(cameraHandle, false);
        cameraHandle = -1;
        if (!g_settings.Debug.FPVCam.DisableRemoveHead) {
            HideHead(false);
        }
        HUD::UNLOCK_MINIMAP_ANGLE();
        GRAPHICS::SET_PARTICLE_FX_CAM_INSIDE_VEHICLE(false);
    }
    directionLookAngle = 0.0f;
    accelMoveFwd = 0.0f;
    accelMoveLat = 0.0f;
    accelMoveVert = 0.0f;
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

    if (!g_settings.Debug.FPVCam.DisableRemoveProps) {
        if (remove) {
            savedHeadProp = PED::GET_PED_PROP_INDEX(g_playerPed, static_cast<int>(ePedPropPosition::AnchorHead));
            savedHeadPropTx = PED::GET_PED_PROP_TEXTURE_INDEX(g_playerPed, static_cast<int>(ePedPropPosition::AnchorHead));

            savedEyesProp = PED::GET_PED_PROP_INDEX(g_playerPed, static_cast<int>(ePedPropPosition::AnchorEyes));
            savedEyesPropTx = PED::GET_PED_PROP_TEXTURE_INDEX(g_playerPed, static_cast<int>(ePedPropPosition::AnchorEyes));

            PED::CLEAR_PED_PROP(g_playerPed, static_cast<int>(ePedPropPosition::AnchorHead));
            PED::CLEAR_PED_PROP(g_playerPed, static_cast<int>(ePedPropPosition::AnchorEyes));
        }
        else {
            if (savedHeadProp != -1)
                PED::SET_PED_PROP_INDEX(g_playerPed, static_cast<int>(ePedPropPosition::AnchorHead), savedHeadProp, savedHeadPropTx, true);
            if (savedEyesProp != -1)
                PED::SET_PED_PROP_INDEX(g_playerPed, static_cast<int>(ePedPropPosition::AnchorEyes), savedEyesProp, savedEyesPropTx, true);
        }
    }
}

void FPVCam::initCam() {
    auto cV = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(g_playerVehicle, { 0.0f, 2.0f, 0.5f });
    cameraHandle = CAM::CREATE_CAM_WITH_PARAMS(
        "DEFAULT_SCRIPTED_CAMERA",
        cV,
        {},
        g_settings().Misc.Camera.Ped.FOV, 1, 2);

    // This should be named something else, like "_SET_VEHICLE_SPEED_JITTER" or something.
    // Thanks for finding it, Jitnaught!
    VEHICLE::SET_CAR_HIGH_SPEED_BUMP_SEVERITY_MULTIPLIER(0.0f);

    if (!g_settings.Debug.FPVCam.DisableRemoveHead) {
        HideHead(true);
    }
}

void FPVCam::Update() {
    if (g_settings.Debug.FPVCam.Disable) {
        CancelCam();
        return;
    }

    bool enable = g_settings().Misc.Camera.Enable;
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

    bool disableBike = g_settings().Misc.Camera.Bike.Disable && bikeSeat;

    if (!enable || !fpv || !inCar || !hasControl || aiming || disableBike) {
        CancelCam();
        return;
    }

    PAD::DISABLE_CONTROL_ACTION(0, eControl::ControlVehicleCinCam, true);

    // Initialize camera
    if (cameraHandle == -1) {
        initCam();

        CAM::SET_CAM_ACTIVE(cameraHandle, true);
        CAM::SET_CAM_IS_INSIDE_VEHICLE(cameraHandle, true);
        GRAPHICS::SET_PARTICLE_FX_CAM_INSIDE_VEHICLE(true);
        CAM::SET_CAM_MOTION_BLUR_STRENGTH(cameraHandle, 1.0f);
        CAM::RENDER_SCRIPT_CAMS(true, false, 0, true, false, 0);
    }
    CAM::SET_SCRIPTED_CAMERA_IS_FIRST_PERSON_THIS_FRAME(true);

    if (MT_LookingLeft() || MT_LookingRight()) {
        updateWheelLook(lookingIntoGlass);
    }
    // Mouse look
    else if (PAD::IS_USING_KEYBOARD_AND_MOUSE(2) == TRUE) {
        updateMouseLook(lookingIntoGlass);
    }
    // Controller look
    else {
        updateControllerLook(lookingIntoGlass);
    }

    VehicleConfig::SMovement* pMovement = nullptr;

    if (!bikeSeat) {
        switch (g_settings().Misc.Camera.AttachId) {
            case 0: pMovement = &g_settings().Misc.Camera.Ped.Movement; break;
            case 1: pMovement = &g_settings().Misc.Camera.Vehicle1.Movement; break;
            case 2: pMovement = &g_settings().Misc.Camera.Vehicle2.Movement; break;
            default:
                break;
        }
    }
    else {
        pMovement = &g_settings().Misc.Camera.Bike.Movement;
    }

    if (pMovement != nullptr) {
        if (pMovement->Follow) {
            updateRotationCameraMovement(*pMovement);
            updateLongitudinalCameraMovement(*pMovement);
            updateLateralCameraMovement(*pMovement);
            updateVerticalCameraMovement(*pMovement);
            updatePitchCameraMovement(*pMovement);
        }
    }

    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float offsetZ = 0.0f;

    // Left
    if (camRot.z > 85.0f) {
        offsetX = map(camRot.z, 85.0f, 180.0f, 0.0f, -lookLeanCenterDist);
        offsetX = std::clamp(offsetX, -lookLeanCenterDist, 0.0f);

        float frontLean = map(camRot.z, 85.0f, 180.0f, 0.0f, lookLeanFrontDist);
        frontLean = std::clamp(frontLean, 0.0f, lookLeanFrontDist);
        offsetY += frontLean;
    }
    // Right
    if (camRot.z < -85.0f) {
        offsetX = map(camRot.z, -85.0f, -180.0f, 0.0f, lookLeanCenterDist);
        offsetX = std::clamp(offsetX, 0.0f, lookLeanCenterDist);

        float frontLean = map(camRot.z, -85.0f, -180.0f, 0.0f, lookLeanFrontDist);
        frontLean = std::clamp(frontLean, 0.0f, lookLeanFrontDist);
        offsetY += frontLean;
    }
    // Don't care
    if (!lookingIntoGlass && abs(camRot.z) > 85.0f) {
        float upPeek = map(abs(camRot.z), 85.0f, 160.0f, 0.0f, lookLeanUpDist);
        upPeek = std::clamp(upPeek, 0.0f, lookLeanUpDist);
        offsetZ += upPeek;
    }

    bool wearingHelmet =
        PED::IS_PED_WEARING_HELMET(g_playerPed) ||
        PED::IS_CURRENT_HEAD_PROP_A_HELMET(g_playerPed) ||
        PED::GET_PED_PROP_INDEX(g_playerPed, static_cast<int>(ePedPropPosition::AnchorHead)) > -1 ||
        PED::GET_PED_PROP_INDEX(g_playerPed, static_cast<int>(ePedPropPosition::AnchorEyes)) > -1;

    if (g_settings.Debug.FPVCam.OverrideNearClip) {
        CAM::SET_CAM_NEAR_CLIP(cameraHandle, g_settings.Debug.FPVCam.NearClip);
    }
    else if (wearingHelmet) {
        CAM::SET_CAM_NEAR_CLIP(cameraHandle, 0.200f);
    }
    else if (!headRemoved) {
        // FPV driving gameplay is 0.149
        // Add 2.6cm so the head model is entirely clipped out
        CAM::SET_CAM_NEAR_CLIP(cameraHandle, 0.175f);
    }
    else {
        // Same as FPV walking gameplay
        CAM::SET_CAM_NEAR_CLIP(cameraHandle, 0.05f);
    }
    // 10km in city, 15km outside
    CAM::SET_CAM_FAR_CLIP(cameraHandle, 12500.0f);

    auto attachId = g_settings().Misc.Camera.AttachId;
    Vector3 additionalOffset{};

    if (bikeSeat) {
        attachId = g_settings().Misc.Camera.Bike.AttachId;
        additionalOffset.x = g_settings().Misc.Camera.Bike.OffsetSide;
        additionalOffset.y = g_settings().Misc.Camera.Bike.OffsetForward;
        additionalOffset.z = g_settings().Misc.Camera.Bike.OffsetHeight;
    }

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
        g_playerVehicle, seatCoords);

    if (bikeSeat) {
        Vector3 headBoneCoord = PED::GET_PED_BONE_COORDS(g_playerPed, 0x796E, {});
        Vector3 headBoneOff = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(
            g_playerPed, headBoneCoord);
        // SKEL_Spine_Root
        Vector3 spinebaseCoord = PED::GET_PED_BONE_COORDS(g_playerPed, 0xe0fd, {});
        Vector3 spinebaseOff = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(
            g_playerPed, spinebaseCoord);
        Vector3 offHead = headBoneOff - spinebaseOff;

        camSeatOffset = camSeatOffset + offHead;
    }

    float pitch;
    float fov;
    float horizonLockPitch = 0.0f;
    float horizonLockRoll = 0.0f;
    switch(attachId) {
        case 2: {
            fov = g_settings().Misc.Camera.Vehicle2.FOV;
            CAM::ATTACH_CAM_TO_ENTITY(cameraHandle, g_playerVehicle, {
                seatOffset.x + camSeatOffset.x + additionalOffset.x + g_settings().Misc.Camera.Vehicle2.OffsetSide + offsetX + accelMoveLat,
                seatOffset.y + camSeatOffset.y + additionalOffset.y + g_settings().Misc.Camera.Vehicle2.OffsetForward + offsetY + accelMoveFwd,
                seatOffset.z + camSeatOffset.z + additionalOffset.z + g_settings().Misc.Camera.Vehicle2.OffsetHeight + offsetZ + accelMoveVert + rollbarOffset
                }, true);
            pitch = g_settings().Misc.Camera.Vehicle2.Pitch;
            break;
        }
        case 1: {
            fov = g_settings().Misc.Camera.Vehicle1.FOV;
            CAM::ATTACH_CAM_TO_ENTITY(cameraHandle, g_playerVehicle, {
                seatOffset.x + camSeatOffset.x + additionalOffset.x + g_settings().Misc.Camera.Vehicle1.OffsetSide + offsetX + accelMoveLat,
                seatOffset.y + camSeatOffset.y + additionalOffset.y + g_settings().Misc.Camera.Vehicle1.OffsetForward + offsetY + accelMoveFwd,
                seatOffset.z + camSeatOffset.z + additionalOffset.z + g_settings().Misc.Camera.Vehicle1.OffsetHeight + offsetZ + accelMoveVert + rollbarOffset
                }, true);
            pitch = g_settings().Misc.Camera.Vehicle1.Pitch;
            break;
        }
        default:
        case 0: {
            fov = g_settings().Misc.Camera.Ped.FOV;
            // 0x796E skel_head id
            CAM::ATTACH_CAM_TO_PED_BONE(cameraHandle, g_playerPed, 0x796E, {
                additionalOffset.x + g_settings().Misc.Camera.Ped.OffsetSide + offsetX + accelMoveLat,
                additionalOffset.y + g_settings().Misc.Camera.Ped.OffsetForward + offsetY + accelMoveFwd,
                additionalOffset.z + g_settings().Misc.Camera.Ped.OffsetHeight + offsetZ + accelMoveVert
                }, true);
            pitch = g_settings().Misc.Camera.Ped.Pitch;
            break;
        }
    }

    // Override
    if (bikeSeat) {
        pitch = g_settings().Misc.Camera.Bike.Pitch;
        fov = g_settings().Misc.Camera.Bike.FOV;
    }

    auto rot = ENTITY::GET_ENTITY_ROTATION(g_playerVehicle, 0);
    float pitchLookComp = -rot.x * 2.0f * abs(camRot.z) / 180.0f;
    float rollLookComp = -rot.y * 2.0f * abs(camRot.z) / 180.0f;
    float rollPitchComp = sin(deg2rad(camRot.z)) * rot.y;

    bool horLock = false;
    int horPitchMode;
    float horCenterSpeed;
    float horPitchLim;
    float horRollLim;

    if (attachId == 2 && g_settings().Misc.Camera.Vehicle2.HorizonLock.Lock) {
        horLock = true;
        horPitchMode = g_settings().Misc.Camera.Vehicle2.HorizonLock.PitchMode;
        horCenterSpeed = g_settings().Misc.Camera.Vehicle2.HorizonLock.CenterSpeed;
        horPitchLim = g_settings().Misc.Camera.Vehicle2.HorizonLock.PitchLim;
        horRollLim = g_settings().Misc.Camera.Vehicle2.HorizonLock.RollLim;
    }
    else if (attachId == 1 && g_settings().Misc.Camera.Vehicle1.HorizonLock.Lock) {
        horLock = true;
        horPitchMode = g_settings().Misc.Camera.Vehicle1.HorizonLock.PitchMode;
        horCenterSpeed = g_settings().Misc.Camera.Vehicle1.HorizonLock.CenterSpeed;
        horPitchLim = g_settings().Misc.Camera.Vehicle1.HorizonLock.PitchLim;
        horRollLim = g_settings().Misc.Camera.Vehicle1.HorizonLock.RollLim;
    }
    else if (attachId == 0 && g_settings().Misc.Camera.Ped.HorizonLock.Lock) {
        horLock = true;
        horPitchMode = g_settings().Misc.Camera.Ped.HorizonLock.PitchMode;
        horCenterSpeed = g_settings().Misc.Camera.Ped.HorizonLock.CenterSpeed;
        horPitchLim = g_settings().Misc.Camera.Ped.HorizonLock.PitchLim;
        horRollLim = g_settings().Misc.Camera.Ped.HorizonLock.RollLim;
    }
    else if (bikeSeat && g_settings().Misc.Camera.Bike.HorizonLock.Lock) {
        horLock = true;
        horPitchMode = g_settings().Misc.Camera.Bike.HorizonLock.PitchMode;
        horCenterSpeed = g_settings().Misc.Camera.Bike.HorizonLock.CenterSpeed;
        horPitchLim = g_settings().Misc.Camera.Bike.HorizonLock.PitchLim;
        horRollLim = g_settings().Misc.Camera.Bike.HorizonLock.RollLim;
    }

    if (horLock) {
        pitchLookComp = 0;
        rollPitchComp = 0;
        auto vehPitch = ENTITY::GET_ENTITY_PITCH(g_playerVehicle);
        auto vehRoll = ENTITY::GET_ENTITY_ROLL(g_playerVehicle);
        float dynamicPitch = 0.0f;

        vehPitch = std::clamp(vehPitch, -horPitchLim, horPitchLim);
        vehRoll = std::clamp(vehRoll, -horRollLim, horRollLim);

        switch (horPitchMode) {
            case 2: {
                float rate = MISC::GET_FRAME_TIME() * horCenterSpeed;
                lowpassPitch = rate * (vehPitch)+(1.0f - rate) * lowpassPitch;
                dynamicPitch = vehPitch - lowpassPitch;
                dynamicPitch = std::clamp(dynamicPitch, -horPitchLim, horPitchLim);
                lowpassPitch = std::clamp(lowpassPitch, -horPitchLim, horPitchLim);
                break;
            }
            case 1: {
                dynamicPitch = 0.0f;
                break;
            }
            case 0: [[fallthrough]];
            default: {
                dynamicPitch = vehPitch;
            }
        }

        horizonLockPitch = abs(camRot.z) <= 90.0f ?
            map(abs(camRot.z), 0.0f, 90.0f, dynamicPitch, 0.0f) :
            map(abs(camRot.z), 90.0f, 180.0f, 0.0f, -dynamicPitch + rot.x * 2.0f * abs(camRot.z) / 180.0f);

        horizonLockRoll = abs(camRot.z) <= 90.0f ?
            map(abs(camRot.z), 0.0f, 90.0f, vehRoll, 0.0f) :
            map(abs(camRot.z), 90.0f, 180.0f, 0.0f, -vehRoll);
    }

    CAM::SET_CAM_ROT(
        cameraHandle, {
            rot.x + camRot.x + pitch + pitchLookComp + rollPitchComp + accelPitchDeg - horizonLockPitch,
            rot.y + rollLookComp + horizonLockRoll,
            rot.z + camRot.z - directionLookAngle
        },
        0);

    CAM::SET_CAM_FOV(cameraHandle, fov);

    float minimapAngle = rot.z + camRot.z - directionLookAngle;
    if (minimapAngle > 360.0f) minimapAngle = minimapAngle - 360.0f;
    if (minimapAngle < 0.0f) minimapAngle = minimapAngle + 360.0f;

    HUD::LOCK_MINIMAP_ANGLE(static_cast<int>(minimapAngle));
}

void FPVCam::updateControllerLook(bool& lookingIntoGlass) {
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
        1.0f - pow(g_settings().Misc.Camera.LookTime, MISC::GET_FRAME_TIME()));

    if (PAD::GET_CONTROL_NORMAL(0, eControl::ControlVehicleLookBehind) != 0.0f) {
        float lookBackAngle = -179.0f; // Look over right shoulder
        if (g_vehData.mIsRhd) {
            lookBackAngle = 179.0f; // Look over left shoulder
        }
        camRot.z = lerp(camRot.z, lookBackAngle,
            1.0f - pow(g_settings().Misc.Camera.LookTime, MISC::GET_FRAME_TIME()));
    }
    else {
        // Manual look
        camRot.z = lerp(camRot.z, 179.0f * -lookLeftRight,
            1.0f - pow(g_settings().Misc.Camera.LookTime, MISC::GET_FRAME_TIME()));
    }
}

void FPVCam::updateMouseLook(bool& lookingIntoGlass) {
    float lookLeftRight = 
        PAD::GET_CONTROL_NORMAL(0, eControl::ControlLookLeftRight) * g_settings().Misc.Camera.MouseSensitivity;
    float lookUpDown = 
        PAD::GET_CONTROL_NORMAL(0, eControl::ControlLookUpDown) * g_settings().Misc.Camera.MouseSensitivity;

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
        lastMouseLookInputTimer.Reset(g_settings().Misc.Camera.MouseCenterTimeout);
    }

    if (lastMouseLookInputTimer.Expired() && Length(g_vehData.mVelocity) > 1.0f && !lookBehind) {
        lookUpDownAcc = lerp(lookUpDownAcc, 0.0f,
            1.0f - pow(g_settings().Misc.Camera.MouseLookTime, MISC::GET_FRAME_TIME()));
        lookLeftRightAcc = lerp(lookLeftRightAcc, 0.0f,
            1.0f - pow(g_settings().Misc.Camera.MouseLookTime, MISC::GET_FRAME_TIME()));
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
        1.0f - pow(g_settings().Misc.Camera.MouseLookTime, MISC::GET_FRAME_TIME()));

    // Override any camRot.z changes while looking back 
    if (lookBehind) {
        float lookBackAngle = -179.0f; // Look over right shoulder
        if (g_vehData.mIsRhd) {
            lookBackAngle = 179.0f; // Look over left shoulder
        }
        camRot.z = lerp(camRot.z, lookBackAngle,
            1.0f - pow(g_settings().Misc.Camera.MouseLookTime, MISC::GET_FRAME_TIME()));
    }
    else {
        camRot.z = lerp(camRot.z, 179.0f * -lookLeftRightAcc,
            1.0f - pow(g_settings().Misc.Camera.MouseLookTime, MISC::GET_FRAME_TIME()));
    }
}

void FPVCam::updateWheelLook(bool& lookingIntoGlass) {
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
            1.0f - pow(g_settings().Misc.Camera.MouseLookTime, MISC::GET_FRAME_TIME()));
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
            1.0f - pow(g_settings().Misc.Camera.MouseLookTime, MISC::GET_FRAME_TIME()));
    }
}

void FPVCam::updateRotationCameraMovement(VehicleConfig::SMovement& movement) {
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

    float velComponent = travelDir * movement.RotationDirectionMult;
    float rotComponent = rotationVelocity.z * movement.RotationRotationMult;
    float rotMax = deg2rad(movement.RotationMaxAngle);
    float totalMove = std::clamp(velComponent + rotComponent,
        -rotMax,
        rotMax);
    float newAngle = -rad2deg(totalMove);

    if (speedVector.y < 3.0f) {
        newAngle = map(speedVector.y, 0.0f, 3.0f, 0.0f, newAngle);
        newAngle = std::clamp(newAngle, 0.0f, newAngle);
    }

    bool isHeli = g_vehData.mClass == VehicleClass::Heli;
    bool isHover = VExt::GetHoverTransformRatio(g_playerVehicle) > 0.0f;
    // 0.0f: Forward, 1.0f: Vertical
    bool isAirHover = g_gameVersion >= G_VER_1_0_1180_2_STEAM && g_vehData.mDomain == VehicleDomain::Air &&
        VEHICLE::GET_VEHICLE_FLIGHT_NOZZLE_POSITION(g_playerVehicle) > 0.5f;

    if (isHeli || isHover || isAirHover) {
        newAngle = 0.0f;
    }

    directionLookAngle = lerp(directionLookAngle, newAngle,
        1.0f - pow(0.000001f, MISC::GET_FRAME_TIME()));
}

void FPVCam::updateLongitudinalCameraMovement(VehicleConfig::SMovement& movement) {
    float baseRoughnessExp = -3.0f;
    float roughnessExp = baseRoughnessExp - movement.Roughness;
    float roughness = pow(10.0f, roughnessExp);
    float lerpF = 1.0f - pow(roughness, MISC::GET_FRAME_TIME());

    float gForce = g_vehData.mAcceleration.y / 9.81f;

    //gForce = abs(pow(gForce, g_settings().Misc.Camera.Movement.LongGamma)) * sgn(gForce);

    float mappedAccel = 0.0f;
    float deadzone = movement.LongDeadzone;

    float mult = 0.0f;
    // Accelerate
    if (gForce > deadzone) {
        mappedAccel = map(gForce, deadzone, 10.0f, 0.0f, 10.0f);
        mult = movement.LongBackwardMult;
    }
    // Decelerate
    if (gForce < -deadzone) {
        mappedAccel = map(gForce, -deadzone, -10.0f, 0.0f, -10.0f);
        mult = movement.LongForwardMult;
    }
    float longBwLim = movement.LongBackwardLimit;
    float longFwLim = movement.LongForwardLimit;
    float accelVal = 
        std::clamp(-mappedAccel * mult,
            -longBwLim,
            longFwLim);
    accelMoveFwd = lerp(accelMoveFwd, accelVal, lerpF); // just for smoothness
}

void FPVCam::updateLateralCameraMovement(VehicleConfig::SMovement& movement) {
    float baseRoughnessExp = -3.0f;
    float roughnessExp = baseRoughnessExp - movement.Roughness;
    float roughness = pow(10.0f, roughnessExp);
    float lerpF = 1.0f - pow(roughness, MISC::GET_FRAME_TIME());

    auto accelVec = g_vehData.mAccelerationWithCentripetal;
    float gForce = accelVec.x / 9.8f;

    float mappedAccel = 0.0f;
    const float deadzone = movement.LatDeadzone;

    float mult = 0.0f;

    if (abs(gForce) > deadzone) {
        mappedAccel = map(gForce, deadzone, 10.0f, 0.0f, 10.0f);
        mult = movement.LatMult;
    }
    float latLim = movement.LatLimit;

    float accelVal =
        std::clamp(mappedAccel * mult,
            -latLim,
            latLim);
    accelMoveLat = lerp(accelMoveLat, accelVal, lerpF); // just for smoothness
}

void FPVCam::updateVerticalCameraMovement(VehicleConfig::SMovement& movement) {
    float baseRoughnessExp = -3.0f;
    float roughnessExp = baseRoughnessExp - movement.Roughness;
    float roughness = pow(10.0f, roughnessExp);
    float lerpF = 1.0f - pow(roughness, MISC::GET_FRAME_TIME());

    auto accelVec = g_vehData.mAccelerationWithCentripetal;
    float gForce = accelVec.z / 9.8f;

    float mappedAccel = 0.0f;
    const float deadzone = movement.VertDeadzone;

    float mult = 0.0f;

    // Up
    if (gForce > deadzone) {
        mappedAccel = map(gForce, deadzone, 10.0f, 0.0f, 10.0f);
        mult = movement.VertUpMult;
    }

    // Down
    if (gForce < -deadzone) {
        mappedAccel = map(gForce, -deadzone, -10.0f, 0.0f, -10.0f);
        mult = movement.VertDownMult;
    }

    float accelVal =
        std::clamp(-mappedAccel * mult,
            -movement.VertDownLimit.Value(),
            movement.VertUpLimit.Value());
    accelMoveVert = lerp(accelMoveVert, accelVal, lerpF); // just for smoothness
}

void FPVCam::updatePitchCameraMovement(VehicleConfig::SMovement& movement) {
    float lerpF = 1.0f - pow(0.001f, MISC::GET_FRAME_TIME());

    float gForce = g_vehData.mAcceleration.y / 9.81f;

    float mappedAccel = 0.0f;
    float deadzone = movement.PitchDeadzone;

    float mult = 0.0f;
    // Accelerate
    if (gForce > deadzone) {
        mappedAccel = map(gForce, deadzone, 10.0f, 0.0f, 10.0f);
        mult = movement.PitchUpMult;
    }
    // Decelerate
    if (gForce < -deadzone) {
        mappedAccel = map(gForce, -deadzone, -10.0f, 0.0f, -10.0f);
        mult = movement.PitchDownMult;
    }
    float pitchUpLim = movement.PitchUpMaxAngle;
    float pitchDownLim = movement.PitchDownMaxAngle;
    float pitchVal =
        std::clamp(mappedAccel * mult,
            -pitchDownLim, pitchUpLim);
    accelPitchDeg = lerp(accelPitchDeg, pitchVal, lerpF); // just for smoothness
}
