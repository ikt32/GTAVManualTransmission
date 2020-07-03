#include "DrivingAssists.h"
#include "ScriptSettings.hpp"
#include "VehicleData.hpp"

#include "Memory/Offsets.hpp"
#include "Memory/VehicleExtensions.hpp"

#include "Util/MathExt.h"
#include "Util/UIUtils.h"

#include <inc/natives.h>
#include <fmt/format.h>

using VExt = VehicleExtensions;

extern ScriptSettings g_settings;
extern Vehicle g_playerVehicle;
extern VehicleData g_vehData;
extern CarControls g_controls;

DrivingAssists::ABSData DrivingAssists::GetABS() {
    bool lockedUp = false;
    auto brakePressures = VExt::GetWheelBrakePressure(g_playerVehicle);
    for (int i = 0; i < g_vehData.mWheelCount; i++) {
        if (g_vehData.mWheelsLockedUp[i] && g_vehData.mSuspensionTravel[i] > 0.0f && brakePressures[i] > 0.0f)
            lockedUp = true;
    }
    if (g_vehData.mHandbrake || VEHICLE::IS_VEHICLE_IN_BURNOUT(g_playerVehicle))
        lockedUp = false;

    bool absNativePresent = g_vehData.mHasABS && g_settings().DriveAssists.ABS.Filter;

    if (g_settings().DriveAssists.ABS.Enable && lockedUp && !absNativePresent)
        return { true };

    return { false };
}

DrivingAssists::TCSData DrivingAssists::GetTCS() {
    std::vector<bool> slipped(g_vehData.mWheelCount);
    bool tractionLoss = false;
    if (g_settings().DriveAssists.TCS.Enable) {
        auto pows = VExt::GetWheelPower(g_playerVehicle);
        for (int i = 0; i < g_vehData.mWheelCount; i++) {
            if (g_vehData.mWheelTyreSpeeds[i] > g_vehData.mVelocity.y + g_settings().DriveAssists.TCS.SlipMax &&
                g_vehData.mSuspensionTravel[i] > 0.0f &&
                g_vehData.mWheelsDriven[i] &&
                pows[i] > 0.1f) {
                tractionLoss = true;
                slipped[i] = true;
            }
        }
        if (g_vehData.mHandbrake || VEHICLE::IS_VEHICLE_IN_BURNOUT(g_playerVehicle))
            tractionLoss = false;
    }

    if (g_settings().DriveAssists.TCS.Enable && tractionLoss)
        return { true, slipped };
    return { false, slipped };
}

DrivingAssists::ESPData DrivingAssists::GetESP() {
    ESPData espData{};
    float speed = ENTITY::GET_ENTITY_SPEED(g_playerVehicle);
    Vector3 vecNextSpd = ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true);
    Vector3 rotVel = ENTITY::GET_ENTITY_ROTATION_VELOCITY(g_playerVehicle);
    Vector3 rotRelative{
        speed * -sin(rotVel.z), 0,
        speed * cos(rotVel.z), 0,
        0, 0
    };
    float steerMult = g_settings.CustomSteering.SteeringMult;
    if (g_controls.PrevInput == CarControls::InputDevices::Wheel)
        steerMult = g_settings.Wheel.Steering.SteerMult;

    float avgAngle = VExt::GetWheelAverageAngle(g_playerVehicle) * steerMult;

    Vector3 vecPredStr{
        speed * -sin(avgAngle / g_settings.Wheel.Steering.SteerMult), 0,
        speed * cos(avgAngle / g_settings.Wheel.Steering.SteerMult), 0,
        0, 0
    };

    // understeer
    {
        Vector3 vecNextRot = (vecNextSpd + rotRelative) * 0.5f;
        espData.UndersteerAngle = GetAngleBetween(vecNextRot, vecPredStr);

        float dSpdStr = Distance(vecNextSpd, vecPredStr);
        float aSpdStr = GetAngleBetween(vecNextSpd, vecPredStr);

        float dSpdRot = Distance(vecNextSpd, vecNextRot);
        float aSpdRot = GetAngleBetween(vecNextSpd, vecNextRot);

        if (dSpdStr > dSpdRot && sgn(aSpdRot) == sgn(aSpdStr)) {
            if (abs(espData.UndersteerAngle) > deg2rad(g_settings.DriveAssists.ESP.UnderMin) && g_vehData.mVelocity.y > 10.0f && abs(avgAngle) > deg2rad(2.0f)) {
                espData.Understeer = true;
            }
        }
    }

    // oversteer
    {
        espData.OversteerAngle = acos(g_vehData.mVelocity.y / speed);
        if (isnan(espData.OversteerAngle))
            espData.OversteerAngle = 0.0;

        if (espData.OversteerAngle > deg2rad(g_settings.DriveAssists.ESP.OverMin) && g_vehData.mVelocity.y > 10.0f) {
            espData.Oversteer = true;

            if (sgn(g_vehData.mVelocity.x) == sgn(avgAngle)) {
                espData.OppositeLock = true;
            }
        }
    }
    bool anyWheelOnGround = false;
    for (bool value : g_vehData.mWheelsOnGround) {
        anyWheelOnGround |= value;
    }
    if (g_settings().DriveAssists.ESP.Enable && g_vehData.mWheelCount == 4 && anyWheelOnGround) {
        if (espData.Oversteer || espData.Understeer) {
            espData.Use = true;
        }
    }

    return espData;
}

DrivingAssists::LSDData DrivingAssists::GetLSD() {
    LSDData lsdData{};

    if (g_vehData.mWheelCount == 4 && g_settings().DriveAssists.LSD.Enable) {

        float fdd = 0.0f;
        float rdd = 0.0f;

        if (g_vehData.mWheelAverageDrivenTyreSpeed > 0.0f) {
            auto angularVelocities = VExt::GetWheelRotationSpeeds(g_playerVehicle);
            float WheelSpeedLF = angularVelocities[0];
            float WheelSpeedRF = angularVelocities[1];
            float WheelSpeedLR = angularVelocities[2];
            float WheelSpeedRR = angularVelocities[3];

            float visc = g_settings().DriveAssists.LSD.Viscosity;
            float dbalF = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fDriveBiasFront);
            float dbalR = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fDriveBiasRear);
            float clutch = std::clamp(g_vehData.mClutch, 0.0f, 1.0f);

            // pos: neg brake left, neg throttle right
            float frontDiffDiff = (WheelSpeedLF - WheelSpeedRF) / (WheelSpeedLF + WheelSpeedRF);
            fdd = frontDiffDiff;
            if (WheelSpeedLF == 0.0f || WheelSpeedRF == 0.0f)
                frontDiffDiff = 0.0f;
            lsdData.BrakeLF = frontDiffDiff / 2.0f * dbalF * visc * g_vehData.mThrottle * clutch;
            lsdData.BrakeRF = -frontDiffDiff / 2.0f * dbalF * visc * g_vehData.mThrottle * clutch;

            float rearDiffDiff = (WheelSpeedLR - WheelSpeedRR) / (WheelSpeedLR + WheelSpeedRR);
            rdd = rearDiffDiff;
            if (WheelSpeedLR == 0.0f || WheelSpeedRR == 0.0f)
                rearDiffDiff = 0.0f;
            lsdData.BrakeLR = rearDiffDiff / 2.0f * dbalR * visc * g_vehData.mThrottle * clutch;
            lsdData.BrakeRR = -rearDiffDiff / 2.0f * dbalR * visc * g_vehData.mThrottle * clutch;

            if (lsdData.BrakeLF > 0.0f) { lsdData.BrakeLF = 0.0f; }
            if (lsdData.BrakeRF > 0.0f) { lsdData.BrakeRF = 0.0f; }
            if (lsdData.BrakeLR > 0.0f) { lsdData.BrakeLR = 0.0f; }
            if (lsdData.BrakeRR > 0.0f) { lsdData.BrakeRR = 0.0f; }

            auto unBrakes = {
                lsdData.BrakeLF,
                lsdData.BrakeRF,
                lsdData.BrakeLR,
                lsdData.BrakeRR
            };

            if (*std::min_element(unBrakes.begin(), unBrakes.end()) < -0.05f) {
                lsdData.Use = true;
            }
            else {
                lsdData.Use = false;
                lsdData.BrakeLR = 0.0f;
                lsdData.BrakeRR = 0.0f;
                lsdData.BrakeLF = 0.0f;
                lsdData.BrakeRF = 0.0f;
            }
        }

        if (g_settings.Debug.DisplayInfo) {
            std::string fddcol;
            if (fdd > 0.1f) { fddcol = "~r~"; }
            if (fdd < -0.1f) { fddcol = "~b~"; }

            std::string rddcol;
            if (rdd > 0.1f) { rddcol = "~r~"; }
            if (rdd < -0.1f) { rddcol = "~b~"; }
            UI::ShowText(0.60f, 0.000f, 0.25f, fmt::format("LF LSD: {:.2f}", lsdData.BrakeLF));
            UI::ShowText(0.65f, 0.000f, 0.25f, fmt::format("RF LSD: {:.2f}", lsdData.BrakeRF));
            UI::ShowText(0.70f, 0.000f, 0.25f, fmt::format("{}L-R: {:.2f}", fddcol, fdd));
            UI::ShowText(0.60f, 0.025f, 0.25f, fmt::format("LR LSD: {:.2f}", lsdData.BrakeLR));
            UI::ShowText(0.65f, 0.025f, 0.25f, fmt::format("RR LSD: {:.2f}", lsdData.BrakeRR));
            UI::ShowText(0.70f, 0.025f, 0.25f, fmt::format("{}L-R: {:.2f}", rddcol, rdd));
            UI::ShowText(0.60f, 0.050f, 0.25f, fmt::format(
                "{}LSD: {}", lsdData.Use ? "~g~" : "~r~", lsdData.Use ? "Active" : "Idle/Off"));
        }
    }
    return lsdData;
}
