#include "AWD.h"

#include "Compatibility.h"
#include "ScriptSettings.hpp"
#include "DrivingAssists.h"

#include "Util/MathExt.h"
#include "Util/Strings.hpp"
#include "Util/UIUtils.h"

#include "Memory/Offsets.hpp"
#include "Memory/VehicleExtensions.hpp"

#include <inc/types.h>
#include <inc/natives.h>
#include <fmt/format.h>

#include <algorithm>
#include <cstdint>

extern Vehicle g_playerVehicle;
extern ScriptSettings g_settings;

using VExt = VehicleExtensions;
namespace HR = HandlingReplacement;

struct STransferInfo {
    float TransferRatio;
    float NewDriveBiasF;
    std::string Source;
};

namespace {
    //  - 0.0: original drive bias
    //  - 1.0: max drive bias transfer (from the weak axle)
    float driveBiasTransferRatio = 0.0f;

    const float dbgX = 0.5f;
    const float dbgY = 0.0f;
}

STransferInfo GetTractionTransfer(float driveBiasF, float biasMax);
STransferInfo GetOversteerTransfer(float driveBiasF, float biasMax, DrivingAssists::ESPData espData);
STransferInfo GetUndersteerTransfer(float driveBiasF, float biasMax, DrivingAssists::ESPData espData);

float AWD::GetDriveBiasFront(void* pHandlingDataOrig) {
    if (!pHandlingDataOrig)
        return -1.0f;

    float fDriveBiasFront = *(float*)((uint8_t*)pHandlingDataOrig + hOffsets1604.fDriveBiasFront);
    float fDriveBiasRear = *(float*)((uint8_t*)pHandlingDataOrig + hOffsets1604.fDriveBiasRear);

    float fDriveBiasFrontNorm;

    // Full FWD
    if (fDriveBiasFront == 1.0f && fDriveBiasRear == 0.0f) {
        fDriveBiasFrontNorm = 1.0f;
    }
    // Full RWD
    else if (fDriveBiasFront == 0.0f && fDriveBiasRear == 1.0f) {
        fDriveBiasFrontNorm = 0.0f;
    }
    else {
        fDriveBiasFrontNorm = fDriveBiasFront / 2.0f;
    }
    return fDriveBiasFrontNorm;
}

void AWD::Update() {
    void* handlingDataOrig = nullptr;
    void* handlingDataReplace = nullptr;
    
    if (!HR::GetHandlingData(g_playerVehicle, &handlingDataOrig, &handlingDataReplace)) {
        if (!HR::Enable(g_playerVehicle, &handlingDataReplace)) {
            if (g_settings.Debug.DisplayInfo) {
                UI::ShowText(dbgX, dbgY, 0.5f, "Unsupported (check library)");
            }
            return;
        }
    }

    float driveBiasFOriginal = GetDriveBiasFront(handlingDataOrig);
    float driveBiasFCustom = g_settings().DriveAssists.AWD.CustomBaseBias;
    float driveBiasF = driveBiasFOriginal;

    if (driveBiasFOriginal <= 0.0f || driveBiasFOriginal >= 1.0f ||
        driveBiasFCustom <= 0.0f || driveBiasFCustom >= 1.0f) {
        if (g_settings.Debug.DisplayInfo) {
            UI::ShowText(dbgX, dbgY, 0.5f, "Unsupported (change handling)");
            UI::ShowText(dbgX, dbgY + 0.025f, 0.5f, fmt::format("F: {:.2f}", driveBiasFOriginal));
        }
        return;
    }

    if (VExt::GetNumWheels(g_playerVehicle) != 4) {
        if (g_settings.Debug.DisplayInfo) {
            UI::ShowText(dbgX, dbgY, 0.5f, "Unsupported (need 4 wheels)");
        }
        return;
    }

    float biasMax = g_settings().DriveAssists.AWD.BiasAtMaxTransfer;

    if (g_settings().DriveAssists.AWD.UseCustomBaseBias) {
        driveBiasF = driveBiasFCustom;
    }
    else {
        driveBiasF = driveBiasFOriginal;
    }

    std::vector<STransferInfo> transferInfos{
      { -1.0f, -1.0f, "N/A" }, // Traction
      { -1.0f, -1.0f, "N/A" }, // Oversteer
      { -1.0f, -1.0f, "N/A" }, // Understeer
    };

    if (g_settings().DriveAssists.AWD.UseTraction) {
        transferInfos[0] = GetTractionTransfer(driveBiasF, biasMax);
    }


    if (g_settings().DriveAssists.AWD.UseOversteer || g_settings().DriveAssists.AWD.UseUndersteer) {
        DrivingAssists::ESPData espData = DrivingAssists::GetESP();
        if (g_settings().DriveAssists.AWD.UseOversteer) {
            transferInfos[1] = GetOversteerTransfer(driveBiasF, biasMax, espData);
        }
        if (g_settings().DriveAssists.AWD.UseUndersteer) {
            transferInfos[2] = GetUndersteerTransfer(driveBiasF, biasMax, espData);
        }
    }

    STransferInfo maxTransferInfo = *std::max_element(transferInfos.begin(), transferInfos.end(),
        [](const auto& a, const auto& b) { return a.TransferRatio < b.TransferRatio; });

    if (maxTransferInfo.TransferRatio > 0.0f) {
        driveBiasTransferRatio = maxTransferInfo.TransferRatio;
        driveBiasF = maxTransferInfo.NewDriveBiasF;
    }
    else {
        driveBiasTransferRatio = 0.0f;
    }

    if (g_settings.Debug.DisplayInfo) {
        UI::ShowText(dbgX, dbgY, 0.5f, fmt::format("T: {:.2f}", driveBiasTransferRatio));
        UI::ShowText(dbgX, dbgY + 0.025f, 0.5f, fmt::format("F: {:.2f}", driveBiasF));
        UI::ShowText(dbgX, dbgY + 0.050f, 0.5f, fmt::format("Src: {}", maxTransferInfo.Source));
    }

    // replace value in (current) handling
    auto handlingAddr = VExt::GetHandlingPtr(g_playerVehicle);

    // Don't care about 0.0 or 1.0, as it never occurs.
    *(float*)(handlingAddr + hOffsets1604.fDriveBiasFront) = driveBiasF * 2.0f;
    *(float*)(handlingAddr + hOffsets1604.fDriveBiasRear) = 2.0f * (1.0f - (driveBiasF));
}

float AWD::GetTransferValue() {
    return driveBiasTransferRatio;
}

STransferInfo GetTractionTransfer(float driveBiasF, float biasMax) {
    std::string source = "N/A";
    float driveBiasTransferRatio = 0.0f;
    float outBias = driveBiasF;
    auto wheelSpeeds = VExt::GetTyreSpeeds(g_playerVehicle);

    float avgFrontSpeed = (wheelSpeeds[0] + wheelSpeeds[1]) / 2.0f;
    float avgRearSpeed = (wheelSpeeds[2] + wheelSpeeds[3]) / 2.0f;

    float tlMin = g_settings().DriveAssists.AWD.TractionLossMin;
    float tlMax = g_settings().DriveAssists.AWD.TractionLossMax;

    // g_DriveBiasTransfer may range from 0.0 to 1.0
    // rear biased
    if (driveBiasF < 0.5f && avgFrontSpeed > 1.0f &&
        (wheelSpeeds[2] > avgFrontSpeed * tlMin ||
            wheelSpeeds[3] > avgFrontSpeed * tlMin)) {

        float maxSpeed = std::max(wheelSpeeds[2], wheelSpeeds[3]);
        float throttle = VExt::GetThrottle(g_playerVehicle);

        driveBiasTransferRatio = map(maxSpeed, avgFrontSpeed * tlMin, avgFrontSpeed * tlMax, 0.0f, 1.0f) * throttle;
        driveBiasTransferRatio = std::clamp(driveBiasTransferRatio, 0.0f, 1.0f);

        outBias = map(driveBiasTransferRatio, 0.0f, 1.0f, driveBiasF, biasMax);

        outBias = std::clamp(outBias, 0.0f, biasMax);
        source = "Traction (RWD)";
    }
    // front biased
    else if (driveBiasF > 0.5f && avgRearSpeed > 1.0f &&
        (wheelSpeeds[0] > avgRearSpeed * tlMin ||
            wheelSpeeds[1] > avgRearSpeed * tlMin)) {

        float maxSpeed = std::max(wheelSpeeds[0], wheelSpeeds[1]);
        float throttle = VExt::GetThrottle(g_playerVehicle);

        driveBiasTransferRatio = map(maxSpeed, avgRearSpeed * tlMin, avgRearSpeed * tlMax, 0.0f, 1.0f) * throttle;
        driveBiasTransferRatio = std::clamp(driveBiasTransferRatio, 0.0f, 1.0f);

        outBias = map(driveBiasTransferRatio, 0.0f, 1.0f, driveBiasF, biasMax);

        outBias = std::clamp(outBias, biasMax, 1.0f);
        source = "Traction (FWD)";
    }
    else {
        driveBiasTransferRatio = 0.0f;
    }

    return { driveBiasTransferRatio, outBias, source };
}

STransferInfo GetOversteerTransfer(float driveBiasF, float biasMax, DrivingAssists::ESPData espData) {
    float transferRatio = 0.0f;
    float outBias = driveBiasF;

    float oversteerDeg = rad2deg(espData.OversteerAngle);
    if (abs(oversteerDeg) > g_settings().DriveAssists.AWD.OversteerMin &&
        ENTITY::GET_ENTITY_SPEED(g_playerVehicle) > 1.0f) {
        float throttle = VExt::GetThrottle(g_playerVehicle);
        float osMin = g_settings().DriveAssists.AWD.OversteerMin;
        float osMax = g_settings().DriveAssists.AWD.OversteerMax;

        transferRatio = map(
            oversteerDeg,
            osMin, osMax,
            0.0f, 1.0f) * throttle;
        transferRatio = std::clamp(transferRatio, 0.0f, 1.0f);

        outBias = map(transferRatio, 0.0f, 1.0f, driveBiasF, biasMax);
        outBias = std::clamp(outBias, 0.0f, 1.0f);
    }

    return { transferRatio, outBias, "Oversteer" };
}

STransferInfo GetUndersteerTransfer(float driveBiasF, float biasMax, DrivingAssists::ESPData espData) {
    float transferRatio = 0.0f;
    float outBias = driveBiasF;

    float understeerDeg = rad2deg(espData.UndersteerAngle);
    if (espData.UndersteerAngleValid &&
        understeerDeg > g_settings().DriveAssists.AWD.UndersteerMin) {
        float throttle = VExt::GetThrottle(g_playerVehicle);
        float usMin = g_settings().DriveAssists.AWD.UndersteerMin;
        float usMax = g_settings().DriveAssists.AWD.UndersteerMax;

        transferRatio = map(
            understeerDeg,
            usMin, usMax,
            0.0f, 1.0f) * throttle;
        transferRatio = std::clamp(transferRatio, 0.0f, 1.0f);

        outBias = map(transferRatio, 0.0f, 1.0f, driveBiasF, biasMax);
        outBias = std::clamp(outBias, 0.0f, 1.0f);
    }

    return { transferRatio, outBias, "Understeer" };
}
