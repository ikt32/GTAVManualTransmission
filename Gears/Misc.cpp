#include "Misc.h"
#include "Input/NativeInput.h"
#include "ScriptSettings.hpp"
#include "Input/CarControls.hpp"
#include "ScriptUtils.h"

#include <inc/natives.h>

extern ScriptSettings g_settings;
extern CarControls g_controls;

extern Ped g_playerPed;
extern Vehicle g_playerVehicle;

namespace {
    const int tapLim = 250;
    NativeInput nativeInput;
    bool wasEngineRunning;
}

void Misc::UpdateEngineOnOff() {
    nativeInput.Update();

    if (g_settings.GameAssists.DisableAutostart) {
        auto vehToEnter = PED::GET_VEHICLE_PED_IS_TRYING_TO_ENTER(g_playerPed);
        if (!Util::VehicleAvailable(g_playerVehicle, g_playerPed) && ENTITY::DOES_ENTITY_EXIST(vehToEnter)) {
            if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehToEnter)) {
                VEHICLE::SET_VEHICLE_ENGINE_ON(vehToEnter, false, false, g_settings.GameAssists.DisableAutostart);
            }
        }
    }

    auto tapStat = nativeInput.WasButtonTapped(eControl::ControlVehicleExit, tapLim);
    Vehicle currVehicle = PED::GET_VEHICLE_PED_IS_IN(g_playerPed, false);
    bool engineRunning = VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(currVehicle);

    if (g_settings.GameAssists.LeaveEngineRunning && ENTITY::DOES_ENTITY_EXIST(currVehicle)) {
        // Long press: Always turn off
        if (nativeInput.WasButtonHeldOverMs(eControl::ControlVehicleExit, tapLim)) {
            VEHICLE::SET_VEHICLE_ENGINE_ON(g_playerVehicle, false, false, g_settings.GameAssists.DisableAutostart);
        }

        // Short press: Always leave as-is
        if (tapStat == NativeInput::TapState::Tapped) {
            // Use the state from last frame, as the game has already turned the engine off by the time we get out.
            VEHICLE::SET_VEHICLE_ENGINE_ON(currVehicle, wasEngineRunning, true, g_settings.GameAssists.DisableAutostart);
        }
    }

    wasEngineRunning = engineRunning;

    // We're interacting with the car, but not ready for input yet
    if (ENTITY::DOES_ENTITY_EXIST(currVehicle) && !Util::VehicleAvailable(g_playerVehicle, g_playerPed)) {
        VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(currVehicle, g_controls.BrakeVal > 0.0f);
    }
}
