#include "ScriptUtils.h"

#include "MathExt.h"
#include "../Memory/VehicleExtensions.hpp"

#include <inc/natives.h>
#include <fmt/format.h>

void Controls::SetControlADZ(eControl control, float value, float adz) {
    PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, control, sgn(value) * adz + (1.0f - adz) * value);
}

bool Util::PlayerAvailable(Player player, Ped playerPed) {
    if (!PLAYER::IS_PLAYER_CONTROL_ON(player) ||
        PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE) ||
        CUTSCENE::IS_CUTSCENE_PLAYING() ||
        !ENTITY::DOES_ENTITY_EXIST(playerPed) ||
        ENTITY::IS_ENTITY_DEAD(playerPed, 0)) {
        return false;
    }
    return true;
}

bool Util::VehicleAvailable(Vehicle vehicle, Ped playerPed) {
    return vehicle != 0 &&
        ENTITY::DOES_ENTITY_EXIST(vehicle) &&
        PED::IS_PED_SITTING_IN_VEHICLE(playerPed, vehicle) &&
        playerPed == VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1, 0);
}

bool Util::IsPedOnSeat(Vehicle vehicle, Ped ped, int seat) {
    Vehicle pedVehicle = PED::GET_VEHICLE_PED_IS_IN(ped, false);
    return vehicle == pedVehicle &&
        VEHICLE::GET_PED_IN_VEHICLE_SEAT(pedVehicle, seat, 0) == ped;
}

// TODO: Would be neat if natives could be ditched.
std::vector<Vector3> Util::GetWheelCoords(Vehicle handle) {
    std::vector<Vector3> worldCoords;
    std::vector<Vector3> positions = VehicleExtensions::GetWheelOffsets(handle);
    Vector3 position = ENTITY::GET_ENTITY_COORDS(handle, true);
    Vector3 rotation = ENTITY::GET_ENTITY_ROTATION(handle, 0);
    rotation.x = deg2rad(rotation.x);
    rotation.y = deg2rad(rotation.y);
    rotation.z = deg2rad(rotation.z);
    Vector3 direction = ENTITY::GET_ENTITY_FORWARD_VECTOR(handle);

    worldCoords.reserve(positions.size());
    for (const auto& wheelPos : positions) {
        worldCoords.emplace_back(GetOffsetInWorldCoords(position, rotation, direction, wheelPos));
    }
    return worldCoords;
}
