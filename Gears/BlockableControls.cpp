#include "BlockableControls.h"

#include "inc/enums.h"

namespace {
    const std::vector<SControlText<int>> controls {
        { -1,                             "None" },
        { ControlVehicleAim,              "Aim" },
        { ControlVehicleHandbrake,        "Handbrake" },
        { ControlVehicleAttack,           "Attack" },
        { ControlVehicleDuck,             "Duck" },
        { ControlVehicleSelectNextWeapon, "NextWeapon" },
        { ControlVehicleCinCam,           "Cinematic Cam" },
        { ControlVehicleExit,             "Exit Car" },
    };
}

const std::vector<SControlText<int>>& BlockableControls::GetList() {
    return controls;
}

int BlockableControls::GetIndex(int control) {
    for (size_t i = 0; i < controls.size(); i++) {
        if (control == controls[i].Control)
            return static_cast<int>(i);
    }
    return 0;
}
