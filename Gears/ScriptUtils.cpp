#include "ScriptUtils.h"

#include "Constants.h"
#include "Util/MathExt.h"
#include "Util/UIUtils.h"
#include "inc/natives.h"
#include "fmt/format.h"


namespace {
    int notificationHandle = 0;
}

void UI::Notify(const std::string& message) {
    int* notifHandleAddr = &notificationHandle; // prevents optimizing away the pointery thing
    showNotification(fmt::format("{}\n{}", Constants::NotificationPrefix, message), notifHandleAddr);
}

void Controls::SetControlADZ(eControl control, float value, float adz) {
    CONTROLS::_SET_CONTROL_NORMAL(0, control, sgn(value) * adz + (1.0f - adz) * value);
}

bool Util::PlayerAvailable(Player player, Ped playerPed) {
    if (!PLAYER::IS_PLAYER_CONTROL_ON(player) ||
        PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE) ||
        CUTSCENE::IS_CUTSCENE_PLAYING() ||
        !ENTITY::DOES_ENTITY_EXIST(playerPed) ||
        ENTITY::IS_ENTITY_DEAD(playerPed)) {
        return false;
    }
    return true;
}

bool Util::VehicleAvailable(Vehicle vehicle, Ped playerPed) {
    return vehicle != 0 &&
        ENTITY::DOES_ENTITY_EXIST(vehicle) &&
        playerPed == VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1);
}

bool Util::IsPedOnSeat(Vehicle vehicle, Ped ped, int seat) {
    Vehicle pedVehicle = PED::GET_VEHICLE_PED_IS_IN(ped, false);
    return vehicle == pedVehicle &&
        VEHICLE::GET_PED_IN_VEHICLE_SEAT(pedVehicle, seat) == ped;
}
