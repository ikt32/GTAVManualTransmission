#include "AWD.h"



#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <inc/types.h>
#include <inc/natives.h>

#include "Util/MathExt.h"
#include "fmt/format.h"
#include "Memory/Offsets.hpp"
#include "Memory/VehicleExtensions.hpp"
#include "Util/Strings.hpp"
#include "Util/UIUtils.h"

extern float g_DriveBiasTransfer;
extern Vehicle g_playerVehicle;
extern std::unordered_map<uint32_t, std::pair<float, float>> g_driveBiasMap;

using VExt = VehicleExtensions;


// To control the ATTESA E - TS system, there is a 16 - bit computer that monitors the cars movements 10 times per second to sense traction loss by measuring the speed of each wheel via the ABS sensors.
// Also a three - axis G - Sensor mounted underneath the center console feeds lateral and longitudinal inputs into a computer, which controls both the ATTESA - ETS AWD system and the ABS system.
// The computer can then direct up to 50 % of the power to the front wheels.
// When slip is detected on one of the rear wheels(a rear wheel turn 5 % or more than the front wheels), the system directs torque to the front wheels which run a non - limited slip differential.
// Rather than locking the AWD in all the time or having a system that is "all or nothing", the ATTESA E - TS system can apportion different torque ratios to the front wheels as it sees fit.
// This provides the driver with an AWD vehicle that performs like a rear wheel drive vehicle in perfect conditions and can recover control when conditions aren't as perfect.

// From the factory, the system is set up to provide slight oversteer in handling, and in fact the harder the car is cornered, the LESS the AWD system engages the front wheels.
// This promotes the oversteer rather than understeer which is apparent in most AWD / 4WD vehicles.
// The advantage to a more traditional ATTESA(Viscous LSD) system is response in hundredths of a second.

void AWD::Update() {
    // TODO: Currently only runs for R32, might wanna enable for some... list? of vehicles? idk!
    // When we're in here, we can assume g_driveBiasMap contains the thing we should have.

    float driveBiasF = g_driveBiasMap[joaat("r32")].first;
    float driveBiasR = g_driveBiasMap[joaat("r32")].second;

    auto wheelSpeeds = VExt::GetWheelRotationSpeeds(g_playerVehicle);

    float avgFrontSpeed = (wheelSpeeds[0] + wheelSpeeds[1]) / 2.0f;

    // g_DriveBiasTransfer may range from 0.0 to 1.0
    if (avgFrontSpeed > 1.0f &&
        (wheelSpeeds[2] > avgFrontSpeed * 1.05f ||
         wheelSpeeds[3] > avgFrontSpeed * 1.05f)) {

        float maxSpeed = std::max(wheelSpeeds[2], wheelSpeeds[3]);
        float throttle = VExt::GetThrottle(g_playerVehicle);
        // 100% faster == 100%? Sure, why not
        g_DriveBiasTransfer = map(maxSpeed, avgFrontSpeed * 1.05f, avgFrontSpeed * 1.50f, 0.0f, 1.0f) * throttle;
        g_DriveBiasTransfer = std::clamp(g_DriveBiasTransfer, 0.0f, 1.0f);

        driveBiasF = map(g_DriveBiasTransfer, 0.0f, 1.0f, driveBiasF, 0.5f);
        driveBiasR = map(g_DriveBiasTransfer, 0.0f, 1.0f, driveBiasR, 0.5f);

        driveBiasF = std::clamp(driveBiasF, 0.0f, 0.5f);
        driveBiasR = std::clamp(driveBiasR, 0.5f, 1.0f);
    }
    else {
        g_DriveBiasTransfer = 0.0f;
    }

    UI::ShowText(0.5f, 0.000f, 0.5f, fmt::format("T: {:.2f}", g_DriveBiasTransfer));
    UI::ShowText(0.5f, 0.025f, 0.5f, fmt::format("F: {:.2f}", driveBiasF));
    UI::ShowText(0.5f, 0.050f, 0.5f, fmt::format("R: {:.2f}", driveBiasR));

    // replace value in handling
    auto handlingAddr = VExt::GetHandlingPtr(g_playerVehicle);
    *(float*)(handlingAddr + hOffsets1604.fDriveBiasFront) = driveBiasF;
    *(float*)(handlingAddr + hOffsets1604.fDriveBiasRear)  = driveBiasR;
}
