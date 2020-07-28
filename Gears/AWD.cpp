#include "AWD.h"

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

using VExt = VehicleExtensions;

namespace {
    // 0.0 <=> 0.5
    float MaxTransfer = 0.5f;
}

void AWD::Update() {
    // When we're in here, we can assume g_driveBiasMap contains the thing we should have.
    Hash model = ENTITY::GET_ENTITY_MODEL(g_playerVehicle);
    auto* pHandling = (uint8_t*)HandlingReplace::GetOriginalHandling(g_playerVehicle);

    if (!pHandling)
        return;
   
    float driveBiasF;
    {
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
        driveBiasF = fDriveBiasFrontNorm;
    }

    if (driveBiasF == 0.0f || driveBiasF == 1.0f || driveBiasF == 0.5f) {
        UI::ShowText(0.5f, 0.000f, 0.5f, "Unsupported");
        UI::ShowText(0.5f, 0.025f, 0.5f, fmt::format("F: {:.2f}", driveBiasF));
        return;
    }

    auto wheelSpeeds = VExt::GetTyreSpeeds(g_playerVehicle);

    float avgFrontSpeed = (wheelSpeeds[0] + wheelSpeeds[1]) / 2.0f;
    float avgRearSpeed = (wheelSpeeds[2] + wheelSpeeds[3]) / 2.0f;

    // g_DriveBiasTransfer may range from 0.0 to 1.0
    // rear biased
    if (driveBiasF < 0.5f && avgFrontSpeed > 1.0f &&
        (wheelSpeeds[2] > avgFrontSpeed * 1.05f ||
         wheelSpeeds[3] > avgFrontSpeed * 1.05f)) {

        float maxSpeed = std::max(wheelSpeeds[2], wheelSpeeds[3]);
        float throttle = VExt::GetThrottle(g_playerVehicle);

        g_DriveBiasTransfer = map(maxSpeed, avgFrontSpeed * 1.05f, avgFrontSpeed * 1.50f, 0.0f, 1.0f) * throttle;
        g_DriveBiasTransfer = std::clamp(g_DriveBiasTransfer, 0.0f, 1.0f);

        driveBiasF = map(g_DriveBiasTransfer, 0.0f, 1.0f, driveBiasF, MaxTransfer);

        driveBiasF = std::clamp(driveBiasF, 0.0f, MaxTransfer);
    }
    // front biased
    else if (driveBiasF > 0.5f && avgRearSpeed > 1.0f &&
        (wheelSpeeds[0] > avgRearSpeed * 1.05f ||
         wheelSpeeds[1] > avgRearSpeed * 1.05f)) {

        float maxSpeed = std::max(wheelSpeeds[0], wheelSpeeds[1]);
        float throttle = VExt::GetThrottle(g_playerVehicle);

        g_DriveBiasTransfer = map(maxSpeed, avgRearSpeed * 1.05f, avgRearSpeed * 1.50f, 0.0f, 1.0f) * throttle;
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
