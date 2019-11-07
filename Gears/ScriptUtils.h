#pragma once
#include "inc/enums.h"
#include "inc/types.h"
#include <string>

/**
 * Script-specific utilities that put script-specific info into utility functions.
 * Thought this was cleaner than just pulling info into the more generic utility classes.
 */

namespace UI {
    void Notify(int level, const std::string& message);
    void Notify(int level, const std::string& message, bool removePrevious);
}

namespace Controls {
    void SetControlADZ(eControl control, float value, float adz);
}

namespace Util {
    bool PlayerAvailable(Player player, Ped playerPed);
    bool VehicleAvailable(Vehicle vehicle, Ped playerPed);
    bool IsPedOnSeat(Vehicle vehicle, Ped ped, int seat);
}
