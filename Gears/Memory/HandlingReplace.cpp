#include "HandlingReplace.h"

#include <inc/natives.h>
#include <unordered_map>
#include <fmt/format.h>

#include "../Util/Logger.hpp"
#include "../Util/UIUtils.h"

struct HandlingContext {
    HandlingContext() = default;
    HandlingContext(Vehicle v, HandlingReplace::CHandlingData* o, HandlingReplace::CHandlingData* r)
        : Vehicle(v)
        , OriginalHandling(o)
        , ReplacedHandling(r) {}

    // Copy constructor
    HandlingContext(const HandlingContext& hc)
        : Vehicle(hc.Vehicle)
        , OriginalHandling(hc.OriginalHandling)
        , ReplacedHandling(hc.ReplacedHandling) {
        logger.Write(DEBUG, "[Handling] Copy ctor [%X]?", hc.Vehicle);
    }
    // Move constructor
    HandlingContext(HandlingContext&& hc)
        : Vehicle(hc.Vehicle)
        , OriginalHandling(hc.OriginalHandling)
        , ReplacedHandling(hc.ReplacedHandling) {
        logger.Write(DEBUG, "[Handling] Move ctor [%X]?", hc.Vehicle);
        hc.Vehicle = 0;
        hc.OriginalHandling = 0;
        hc.ReplacedHandling = 0;
    }
    // cpy
    HandlingContext& operator=(const HandlingContext& other) {
        logger.Write(DEBUG, "[Handling] Copy assign [%X]?", other.Vehicle);
        Vehicle = other.Vehicle;
        OriginalHandling = other.OriginalHandling;
        ReplacedHandling = other.ReplacedHandling;
        return *this;
    }
    // move
    HandlingContext& operator=(HandlingContext&& other)
    {
        logger.Write(DEBUG, "[Handling] Move assign [%X]?", other.Vehicle);
        Vehicle = other.Vehicle;
        OriginalHandling = other.OriginalHandling;
        ReplacedHandling = other.ReplacedHandling;
        return *this;
    }

    ~HandlingContext() {
        if (!OriginalHandling || !ReplacedHandling || !Vehicle)
            return;
        logger.Write(DEBUG, "[Handling] Deleting handling for [%p] to [%p]", Vehicle, OriginalHandling);
        VehicleExtensions::SetHandlingPtr(Vehicle, (uint64_t)OriginalHandling);
        for (uint8_t idx = 0; idx < VehicleExtensions::GetNumWheels(Vehicle); ++idx) {
            VehicleExtensions::SetWheelHandlingPtr(Vehicle, idx, (uint64_t)OriginalHandling);
        }
        logger.Write(DEBUG, "[Handling] Restored handling for [%p] to [%p]", Vehicle, OriginalHandling);
        // delete ReplacedHandling;
        free(ReplacedHandling);
        Vehicle = 0;
        ReplacedHandling = 0;
        OriginalHandling = 0;
    }
    Vehicle Vehicle = 0;
    HandlingReplace::CHandlingData* OriginalHandling = 0;
    HandlingReplace::CHandlingData* ReplacedHandling = 0;
};

namespace {
    std::unordered_map<Vehicle, std::unique_ptr<HandlingContext>> handlings{};
}

void HandlingReplace::UpdateVehicles(Vehicle vehicle) {
    logger.Write(INFO, "numHandlings = %d", handlings.size());
    // clear stale entries
    auto vehGone = [&](const auto& handlingEntry) {
        if (!ENTITY::DOES_ENTITY_EXIST(handlingEntry.first)) {
            return true;
        }
        return false;
    };
    std::erase_if(handlings, vehGone);

    // new entry
    if (handlings.find(vehicle) == handlings.end()) {
        logger.Write(INFO, "[Handling] New for [%X]", vehicle);

        auto* origHandling = reinterpret_cast<HandlingReplace::CHandlingData*>(VehicleExtensions::GetHandlingPtr(vehicle));
        
        HandlingReplace::CHandlingData* newHandling = (CHandlingData*)malloc(sizeof(CHandlingData));

        memcpy(newHandling, origHandling, sizeof(*origHandling));

        CBaseSubHandlingData* shds[6] = {};
        
        for (int i = 0; i < newHandling->m_subHandlingData.GetCount(); i++)
        {
            if (newHandling->m_subHandlingData.Get(i))
            {
                shds[i] = (CBaseSubHandlingData*)rage::GetAllocator()->allocate(1024, 16, 0);
                memcpy(shds[i], newHandling->m_subHandlingData.Get(i), 1024);

                logger.Write(INFO, "[SubHandlingData] [%p] -> [%p]", newHandling->m_subHandlingData.Get(i), shds[i]);
            }
        }
        
        newHandling->m_subHandlingData.m_offset = nullptr;
        newHandling->m_subHandlingData.Clear();
        newHandling->m_subHandlingData.Set(0, shds[0]);
        newHandling->m_subHandlingData.Set(1, shds[1]);
        newHandling->m_subHandlingData.Set(2, shds[2]);
        newHandling->m_subHandlingData.Set(3, shds[3]);
        newHandling->m_subHandlingData.Set(4, shds[4]);
        newHandling->m_subHandlingData.Set(5, shds[5]);

        handlings[vehicle] = {
            std::make_unique<HandlingContext>(
            vehicle,
            origHandling,
            newHandling)
        };

        uint64_t oldAddr0 = (uint64_t)handlings[vehicle]->OriginalHandling;
        uint64_t newAddr0 = (uint64_t)handlings[vehicle]->ReplacedHandling;

        VehicleExtensions::SetHandlingPtr(vehicle, (uint64_t)handlings[vehicle]->ReplacedHandling);
        for (uint8_t idx = 0; idx < VehicleExtensions::GetNumWheels(vehicle); ++idx) {
            VehicleExtensions::SetWheelHandlingPtr(vehicle, idx, (uint64_t)handlings[vehicle]->ReplacedHandling);
        }

        logger.Write(DEBUG, fmt::format("[Handling] Changed handling for [{:X}]: [{:X}] -> [{:X}]", vehicle, oldAddr0, newAddr0));
    }
}

HandlingReplace::CHandlingData* HandlingReplace::GetOriginalHandling(Vehicle vehicle) {
    if (handlings.find(vehicle) == handlings.end())
        return nullptr;

    return handlings[vehicle]->OriginalHandling;
}

HandlingReplace::CHandlingData* HandlingReplace::GetReplacedHandling(Vehicle vehicle) {
    if (handlings.find(vehicle) == handlings.end())
        return nullptr;

    return handlings[vehicle]->ReplacedHandling;
}

