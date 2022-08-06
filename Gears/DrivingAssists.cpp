#include "DrivingAssists.h"
#include "ScriptSettings.hpp"
#include "VehicleData.hpp"
#include "WheelInput.h"

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
    std::vector<float> slips(g_vehData.mWheelCount);
    bool tractionLoss = false;
    float averageLoss = 0.0f;
    float numLoss = 0.0f;
    float minLoss = std::min(g_settings().DriveAssists.TCS.SlipMin, g_settings().DriveAssists.LaunchControl.SlipMin);
    auto pows = VExt::GetWheelPower(g_playerVehicle);

    auto posWorld = ENTITY::GET_ENTITY_COORDS(g_playerVehicle, 0);
    auto boneVels = VExt::GetWheelBoneVelocity(g_playerVehicle);
    auto steeringAngles = VExt::GetWheelSteeringAngles(g_playerVehicle);

    for (int i = 0; i < g_vehData.mWheelCount; i++) {
        auto boneVelProjection = posWorld + boneVels[i];
        auto boneVelRel = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(
            g_playerVehicle, boneVelProjection.x, boneVelProjection.y, boneVelProjection.z);

        float rotatedY = boneVelRel.y / cos(steeringAngles[i]);
        slips[i] = g_vehData.mWheelTyreSpeeds[i] - rotatedY;

        bool shouldCheck = g_vehData.mSuspensionTravel[i] > 0.0f &&
            g_vehData.mWheelsDriven[i] &&
            pows[i] > 0.1f;

        if (shouldCheck &&
            slips[i] > g_settings().DriveAssists.TCS.SlipMin) {
            tractionLoss = true;
        }
        if (shouldCheck &&
            slips[i] > minLoss) {
            averageLoss += slips[i];
            numLoss += 1.0f;
        }
    }
    if (g_vehData.mHandbrake || VEHICLE::IS_VEHICLE_IN_BURNOUT(g_playerVehicle))
        tractionLoss = false;

    if (tractionLoss) {
        averageLoss /= numLoss;
    }

    return {
        g_settings().DriveAssists.TCS.Enable && tractionLoss,
        slips,
        averageLoss
    };
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

    float avgAngle = VExt::GetWheelAverageAngle(g_playerVehicle);

    // understeer
    {
        auto slipInfos = WheelInput::CalculateSlipInfo();

        Vector3 vecNextRot = (vecNextSpd + rotRelative) * 0.5f;
        Vector3 vecPredStr{
            speed * -sin(avgAngle), 0,
            speed * cos(avgAngle), 0,
            0, 0
        };

        float div = 0.0f;
        float avgSlip = 0.0f;

        const float latSlipOpt = *(float*)(VExt::GetHandlingPtr(g_playerVehicle) + hOffsets.fTractionCurveLateral);

        for (uint8_t i = 0; i < slipInfos.size(); ++i) {
            // Current slip, relative to optimal slip angle.
            float slipRatio = map(abs(slipInfos[i].Angle),
                0.0f, latSlipOpt,
                0.0f, 1.0f);

            float understeerAngle;
            if (slipRatio > 1.0f) {
                understeerAngle = (slipInfos[i].Angle - sgn(slipInfos[i].Angle) * latSlipOpt);
            }
            else {
                understeerAngle = 0.0f;
            }
            
            if (VExt::IsWheelSteered(g_playerVehicle, i)) {
                avgSlip += understeerAngle;
                div += 1.0f;
            }
        }
        if (div != 0.0f) {
            avgSlip /= div;
        }

        espData.UndersteerAngle = avgSlip;

        if (div != 0.0f && vecNextSpd.y > 5.0f &&
            rad2deg(abs(avgSlip)) > g_settings().DriveAssists.ESP.UnderMin) {
            espData.Understeer = true;
        }
    }

    // oversteer
    {
        espData.OversteerAngle = acos(g_vehData.mVelocity.y / speed);
        if (isnan(espData.OversteerAngle))
            espData.OversteerAngle = 0.0;

        if (espData.OversteerAngle > deg2rad(g_settings().DriveAssists.ESP.OverMin) && g_vehData.mVelocity.y > 10.0f) {
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

    if (g_settings().DriveAssists.LSD.Enable &&
        g_vehData.mWheelCount == 4 &&
        g_vehData.mDiffSpeed > 0.0f &&
        !VExt::GetHandbrake(g_playerVehicle) &&
        !VEHICLE::IS_VEHICLE_IN_BURNOUT(g_playerVehicle)) {
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
        if (WheelSpeedLF == 0.0f || WheelSpeedRF == 0.0f)
            frontDiffDiff = 0.0f;
        lsdData.BrakeLF = frontDiffDiff / 2.0f * dbalF * visc * g_vehData.mThrottle * clutch;
        lsdData.BrakeRF = -frontDiffDiff / 2.0f * dbalF * visc * g_vehData.mThrottle * clutch;
        lsdData.FDD = frontDiffDiff;

        float rearDiffDiff = (WheelSpeedLR - WheelSpeedRR) / (WheelSpeedLR + WheelSpeedRR);
        if (WheelSpeedLR == 0.0f || WheelSpeedRR == 0.0f)
            rearDiffDiff = 0.0f;
        lsdData.BrakeLR = rearDiffDiff / 2.0f * dbalR * visc * g_vehData.mThrottle * clutch;
        lsdData.BrakeRR = -rearDiffDiff / 2.0f * dbalR * visc * g_vehData.mThrottle * clutch;
        lsdData.RDD = rearDiffDiff;

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
    return lsdData;
}

std::vector<float> DrivingAssists::GetESPBrakes(ESPData espData) {
    std::vector<float> brakeVals(g_vehData.mWheelCount); // only works for 4 wheels but ok

    float steerMult = g_settings().Steering.CustomSteering.SteeringMult;
    if (g_controls.PrevInput == CarControls::InputDevices::Wheel)
        steerMult = g_settings().Steering.Wheel.SteeringMult;
    float avgAngle = VExt::GetWheelAverageAngle(g_playerVehicle) * steerMult;

    float handlingBrakeForce = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeForce);
    float bbalF = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeBiasFront);
    float bbalR = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeBiasRear);
    float inpBrakeForce = handlingBrakeForce * g_controls.BrakeVal;

    for (auto i = 0; i < g_vehData.mWheelCount; ++i) {
        g_vehData.mWheelsAbs[i] = false;
    }
    float avgAngle_ = -avgAngle;
    if (espData.OppositeLock) {
        avgAngle_ = avgAngle;
    }
    float oversteerAngleDeg = abs(rad2deg(espData.OversteerAngle));
    float overMin = g_settings().DriveAssists.ESP.OverMin;
    float overMax = g_settings().DriveAssists.ESP.OverMax;
    float overMinComp = g_settings().DriveAssists.ESP.OverMinComp;
    float overMaxComp = g_settings().DriveAssists.ESP.OverMaxComp;
    float oversteerComp = map(oversteerAngleDeg,
        overMin, overMax,
        overMinComp, overMaxComp);

    float oversteerAdd = handlingBrakeForce * oversteerComp;


    float oversteerRearAdd = handlingBrakeForce * map(
        oversteerAngleDeg, overMax, overMax * 2.0f,
        overMinComp, overMaxComp);
    oversteerRearAdd = std::clamp(oversteerRearAdd, 0.0f, overMaxComp);

    float understeerAngleDeg(abs(rad2deg(espData.UndersteerAngle)));

    float underMin = g_settings().DriveAssists.ESP.UnderMin;
    float underMax = g_settings().DriveAssists.ESP.UnderMax;
    float underMinComp = g_settings().DriveAssists.ESP.UnderMinComp;
    float underMaxComp = g_settings().DriveAssists.ESP.UnderMaxComp;

    float understeerComp = map(understeerAngleDeg,
        underMin, underMax,
        underMinComp, underMaxComp);

    float understeerAdd = handlingBrakeForce * understeerComp;

    float brkFBase = inpBrakeForce * bbalF;
    brakeVals[0] = brkFBase + (avgAngle_ < 0.0f && espData.Oversteer ? oversteerAdd : 0.0f);
    brakeVals[1] = brkFBase + (avgAngle_ > 0.0f && espData.Oversteer ? oversteerAdd : 0.0f);

    float brkRBase = inpBrakeForce * bbalR;
    float brkRUnderL = (avgAngle > 0.0f && espData.Understeer ? understeerAdd : 0.0f);
    float brkRUnderR = (avgAngle < 0.0f && espData.Understeer ? understeerAdd : 0.0f);

    float brkROverL = (avgAngle_ < 0.0f && espData.Oversteer ? oversteerRearAdd : 0.0f);
    float brkROverR = (avgAngle_ > 0.0f && espData.Oversteer ? oversteerRearAdd : 0.0f);

    brakeVals[2] = brkRBase + brkRUnderL + brkROverL;
    brakeVals[3] = brkRBase + brkRUnderR + brkROverR;

    g_vehData.mWheelsEspO[0] = avgAngle_ < 0.0f && espData.Oversteer ? true : false;
    g_vehData.mWheelsEspO[1] = avgAngle_ > 0.0f && espData.Oversteer ? true : false;
    g_vehData.mWheelsEspU[2] = avgAngle > 0.0f && espData.Understeer ? true : false;
    g_vehData.mWheelsEspU[3] = avgAngle < 0.0f && espData.Understeer ? true : false;

    g_vehData.mWheelsEspO[2] = avgAngle_ < 0.0f && oversteerRearAdd > 0.0f ? true : false;
    g_vehData.mWheelsEspO[3] = avgAngle_ > 0.0f && oversteerRearAdd > 0.0f ? true : false;

    return brakeVals;
}

std::vector<float> DrivingAssists::GetTCSBrakes(TCSData tcsData) {
    std::vector<float> brakeVals(g_vehData.mWheelCount);
    const auto offsets = VExt::GetWheelOffsets(g_playerVehicle);

    const float handlingBrakeForce = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeForce);
    const float bbalF = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeBiasFront);
    const float bbalR = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeBiasRear);

    float inpBrakeForce = handlingBrakeForce * g_controls.BrakeVal;
    float fullBrakePower = handlingBrakeForce * g_settings().DriveAssists.TCS.BrakeMult;

    float tcsSlipMin = g_settings().DriveAssists.TCS.SlipMin;
    float tcsSlipMax = g_settings().DriveAssists.TCS.SlipMax;

    for (int i = 0; i < g_vehData.mWheelCount; i++) {
        float bbal = offsets[i].y > 0.0f ? bbalF : bbalR;

        if (tcsData.LinearSlip[i] > tcsSlipMin &&
            g_vehData.mWheelTyreSpeeds[i] > 0.0f &&
            g_vehData.mSuspensionTravel[i] > 0.0f) {
            float mappedVal = map(
                tcsData.LinearSlip[i],
                tcsSlipMin, tcsSlipMax,
                0.0f, fullBrakePower);
            mappedVal = std::clamp(mappedVal, 0.0f, fullBrakePower);

            brakeVals[i] = std::max(inpBrakeForce * bbal, mappedVal * bbal);
        }
        else {
            brakeVals[i] = inpBrakeForce * bbal;
        }
    }

    return brakeVals;
}

std::vector<float> DrivingAssists::GetABSBrakes(ABSData absData) {
    std::vector<float> brakeVals(g_vehData.mWheelCount);
    const auto offsets = VExt::GetWheelOffsets(g_playerVehicle);

    const float handlingBrakeForce = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeForce);
    const float bbalF = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeBiasFront);
    const float bbalR = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeBiasRear);

    float inpBrakeForce = handlingBrakeForce * g_controls.BrakeVal;

    for (uint8_t i = 0; i < g_vehData.mWheelsLockedUp.size(); i++) {
        float bbal = offsets[i].y > 0.0f ? bbalF : bbalR;

        if (g_vehData.mWheelsLockedUp[i]) {
            brakeVals[i] = 0.0f;
            g_vehData.mWheelsAbs[i] = true;
        }
        else {
            brakeVals[i] = inpBrakeForce * bbal;
            g_vehData.mWheelsAbs[i] = false;
        }
    }

    return brakeVals;
}

std::vector<float> DrivingAssists::GetLSDBrakes(LSDData lsdData) {
    std::vector<float> brakeVals(g_vehData.mWheelCount); // only works for 4 wheels but ok

    float handlingBrakeForce = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeForce);
    float bbalF = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeBiasFront);
    float bbalR = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeBiasRear);
    brakeVals[0] = lsdData.BrakeLF + g_controls.BrakeVal * bbalF * handlingBrakeForce;
    brakeVals[1] = lsdData.BrakeRF + g_controls.BrakeVal * bbalF * handlingBrakeForce;
    brakeVals[2] = lsdData.BrakeLR + g_controls.BrakeVal * bbalR * handlingBrakeForce;
    brakeVals[3] = lsdData.BrakeRR + g_controls.BrakeVal * bbalR * handlingBrakeForce;

    return brakeVals;
}
