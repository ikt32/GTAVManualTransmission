#pragma once
#include <cstdint>

#include "VehicleExtensions.hpp"

namespace HandlingReplace {
    class CHandlingData {
        uint32_t m_name;
        char m_pad[332]; // 1290, 1365, 1493, 1604
        char m_subHandlingDataPad[8+2+2+4];
        // ^ find offset using a variant of 48 85 C9 74 13 BA 04 00 00 00 E8 (and go to the call in there)
        char m_pad2[1000];
    };

    void UpdateVehicles(Vehicle vehicle);

    HandlingReplace::CHandlingData* GetOriginalHandling(Vehicle vehicle);
    HandlingReplace::CHandlingData* GetReplacedHandling(Vehicle vehicle);
}
