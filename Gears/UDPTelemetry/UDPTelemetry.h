#pragma once

#include "Socket.h"
#include "../VehicleData.hpp"
#include "../Input/CarControls.hpp"

namespace UDPTelemetry {
    void UpdatePacket(Socket& socket, Vehicle vehicle, const VehicleData& vehData,
                      const CarControls& controls);
}
