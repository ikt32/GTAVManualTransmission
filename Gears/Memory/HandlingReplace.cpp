#include "HandlingReplace.h"

#include <inc/natives.h>
#include <unordered_map>
#include <fmt/format.h>

#include "../Util/Logger.hpp"
#include "../Util/UIUtils.h"

struct HandlingContext {
    ~HandlingContext() {
        VehicleExtensions::SetHandlingPtr(Vehicle, (uint64_t)OriginalHandling);
        for (uint8_t idx = 0; idx < VehicleExtensions::GetNumWheels(Vehicle); ++idx) {
            VehicleExtensions::SetWheelHandlingPtr(Vehicle, idx, (uint64_t)OriginalHandling);
        }

        logger.Write(INFO, "Restored handling for [%X] to [%X]", Vehicle, (uint64_t)OriginalHandling);
    }
    Vehicle Vehicle;
    HandlingReplace::CHandlingData* OriginalHandling;
    HandlingReplace::CHandlingData ReplacedHandling;
};

std::unordered_map<Vehicle, HandlingContext> handlings;

void HandlingReplace::UpdateVehicles(Vehicle vehicle) {
    // clear thing first
    std::vector<Vehicle> clearedVehicles;

    auto vehGone = [&](const auto& handlingEntry) {
        if (!ENTITY::DOES_ENTITY_EXIST(handlingEntry.first)) {
            clearedVehicles.push_back(handlingEntry.first);
            return true;
        }
        return false;
    };

    std::erase_if(handlings, vehGone);

    logger.Write(INFO, fmt::format("Deleted {}", fmt::join(clearedVehicles, ", ")));

    if (handlings.find(vehicle) == handlings.end()) {
        auto origHandling = (HandlingReplace::CHandlingData*)VehicleExtensions::GetHandlingPtr(vehicle);
        HandlingReplace::CHandlingData newHandling;

        memcpy(&newHandling, origHandling, sizeof(HandlingReplace::CHandlingData));

        handlings[vehicle] = {
            vehicle,
            (HandlingReplace::CHandlingData*)VehicleExtensions::GetHandlingPtr(vehicle),
            newHandling
        };

        uint64_t oldAddr0 = (uint64_t)handlings[vehicle].OriginalHandling;
        uint64_t oldAddr1 = *reinterpret_cast<uint64_t*>(VehicleExtensions::GetAddress(vehicle) + 0x918);
        uint64_t newAddr0 = (uint64_t) & (handlings[vehicle].ReplacedHandling);

        VehicleExtensions::SetHandlingPtr(vehicle, (uint64_t)&(handlings[vehicle].ReplacedHandling));
        for (uint8_t idx = 0; idx < VehicleExtensions::GetNumWheels(vehicle); ++idx) {
            VehicleExtensions::SetWheelHandlingPtr(vehicle, idx, (uint64_t) & (handlings[vehicle].ReplacedHandling));
        }

        uint64_t newAddr1 = *reinterpret_cast<uint64_t*>(VehicleExtensions::GetAddress(vehicle) + 0x918);

        logger.Write(INFO, fmt::format("Changed handlings \nOld1 {:X}\nOld2 {:X}\nNew1 {:X}\nNew2 {:X}", oldAddr0, oldAddr1, newAddr0, newAddr1));
    }
}

HandlingReplace::CHandlingData* HandlingReplace::GetOriginalHandling(Vehicle vehicle) {
    if (handlings.find(vehicle) == handlings.end())
        return nullptr;

    return handlings[vehicle].OriginalHandling;
}

HandlingReplace::CHandlingData* HandlingReplace::GetReplacedHandling(Vehicle vehicle) {
    if (handlings.find(vehicle) == handlings.end())
        return nullptr;

    return &handlings[vehicle].ReplacedHandling;
}

