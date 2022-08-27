#include "GearRattle.h"
#include "ScriptSettings.hpp"
#include "VehicleData.hpp"

#include "Input/CarControls.hpp"
#include "Util/GameSound.h"

#include <inc/natives.h>

extern Vehicle g_playerVehicle;
extern CarControls g_controls;
extern ScriptSettings g_settings;

extern VehicleGearboxStates g_gearStates;

namespace {
    GameSound g_gearRattle1("DAMAGED_TRUCK_IDLE", "", ""); // sadly there's no initial BONK, but the sound works.
    GameSound g_gearRattle2("DAMAGED_TRUCK_IDLE", "", ""); // primitively double the volume...
}

void GearRattle::Start() {
    g_gearRattle1.Play(g_playerVehicle);
    g_gearRattle2.Play(g_playerVehicle);
}

void GearRattle::Stop() {
    g_gearRattle1.Stop();
    g_gearRattle2.Stop();
}

bool GearRattle::IsPlaying() {
    return g_gearRattle1.Playing() || g_gearRattle2.Playing();
}

void GearRattle::Update() {
    if (!GearRattle::IsPlaying()) {
        // play on misshift in wheel
        if (g_controls.PrevInput == CarControls::Wheel && g_settings().MTOptions.ShiftMode == EShiftMode::HPattern) {
            // When the shifter is in a slot, but the gearbox is neutral.
            // Also grinds for nonexisting gears on that car, but you shouldn't be shifting in there anyway.
            if (g_gearStates.FakeNeutral &&
                g_controls.IsHShifterInGear() &&
                !g_controls.IsClutchPressed()) {
                GearRattle::Start();
            }
        }
        // No keeb support, as I'd need to mutilate the code even more to jam it in.
    }
    else {
        // no control check, since we wanna be able to cancel easily
        if (g_settings().MTOptions.ShiftMode == EShiftMode::HPattern) {
            // Stop when popping out of any gear
            // Stop when engine is off
            // Stop when clutch is pressed
            if (g_controls.IsHShifterJustNeutral() ||
                !VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(g_playerVehicle) ||
                g_controls.IsClutchPressed()) {
                GearRattle::Stop();
            }
        }
        else {
            // Stop when switching gearboxes
            GearRattle::Stop();
        }
    }
}
