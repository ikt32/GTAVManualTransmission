#include "AWD.h"

#include "ScriptSettings.hpp"

#include "Util/MathExt.h"
#include "Util/Strings.hpp"
#include "Util/UIUtils.h"

#include "Memory/HandlingReplace.h"
#include "Memory/Offsets.hpp"
#include "Memory/VehicleExtensions.hpp"

#include <inc/types.h>
#include <inc/natives.h>
#include <fmt/format.h>

#include <algorithm>
#include <cstdint>
#include <unordered_map>

extern float g_DriveBiasTransfer;
extern Vehicle g_playerVehicle;
extern ScriptSettings g_settings;

using VExt = VehicleExtensions;

float GetDriveBiasFront(Vehicle vehicle) {
    auto* pHandling = (uint8_t*)HandlingReplace::GetOriginalHandling(vehicle);

    if (!pHandling)
        return -1.0f;

    float fDriveBiasFront = *(float*)(pHandling + hOffsets1604.fDriveBiasFront);
    float fDriveBiasRear = *(float*)(pHandling + hOffsets1604.fDriveBiasRear);

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
    float driveBiasFOriginal = GetDriveBiasFront(g_playerVehicle);
    float driveBiasFCustom = g_settings().DriveAssists.AWD.CustomBaseBias;
    float driveBiasF = driveBiasFOriginal;

    if (driveBiasFOriginal <= 0.0f || driveBiasFOriginal >= 1.0f ||
        driveBiasFCustom <= 0.0f || driveBiasFCustom >= 1.0f) {
        UI::ShowText(0.5f, 0.000f, 0.5f, "Unsupported (change handling)");
        UI::ShowText(0.5f, 0.025f, 0.5f, fmt::format("F: {:.2f}", driveBiasFOriginal));
        return;
    }

    if (VExt::GetNumWheels(g_playerVehicle) != 4) {
        UI::ShowText(0.5f, 0.000f, 0.5f, "Unsupported (need 4 wheels)");
        return;
    }

    float MaxTransfer = g_settings().DriveAssists.AWD.TransferMax;

    if (g_settings().DriveAssists.AWD.UseCustomBaseBias) {
        driveBiasF = driveBiasFCustom;
    }

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

        g_DriveBiasTransfer = map(maxSpeed, avgFrontSpeed * tlMin, avgFrontSpeed * tlMax, 0.0f, 1.0f) * throttle;
        g_DriveBiasTransfer = std::clamp(g_DriveBiasTransfer, 0.0f, 1.0f);

        driveBiasF = map(g_DriveBiasTransfer, 0.0f, 1.0f, driveBiasF, MaxTransfer);

        driveBiasF = std::clamp(driveBiasF, 0.0f, MaxTransfer);
    }
    // front biased
    else if (driveBiasF > 0.5f && avgRearSpeed > 1.0f &&
        (wheelSpeeds[0] > avgRearSpeed * tlMin ||
         wheelSpeeds[1] > avgRearSpeed * tlMin)) {

        float maxSpeed = std::max(wheelSpeeds[0], wheelSpeeds[1]);
        float throttle = VExt::GetThrottle(g_playerVehicle);

        g_DriveBiasTransfer = map(maxSpeed, avgRearSpeed * tlMin, avgRearSpeed * tlMax, 0.0f, 1.0f) * throttle;
        g_DriveBiasTransfer = std::clamp(g_DriveBiasTransfer, 0.0f, 1.0f);

        driveBiasF = map(g_DriveBiasTransfer, 0.0f, 1.0f, driveBiasF, MaxTransfer);

        driveBiasF = std::clamp(driveBiasF, MaxTransfer, 1.0f);
    }
    else {
        g_DriveBiasTransfer = 0.0f;
    }

    UI::ShowText(0.5f, 0.000f, 0.5f, fmt::format("T: {:.2f}", g_DriveBiasTransfer));
    UI::ShowText(0.5f, 0.025f, 0.5f, fmt::format("F: {:.2f}", driveBiasF));

    // replace value in (current) handling
    auto handlingAddr = VExt::GetHandlingPtr(g_playerVehicle);

    // Don't care about 0.0 or 1.0, as it never occurs.
    *(float*)(handlingAddr + hOffsets1604.fDriveBiasFront) = driveBiasF * 2.0f;
    *(float*)(handlingAddr + hOffsets1604.fDriveBiasRear) = 2.0f * (1.0f - (driveBiasF));
}
