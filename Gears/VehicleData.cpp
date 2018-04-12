#include <inc/natives.h>

#include "VehicleData.hpp"
#include "Memory/VehicleFlags.h"

VehicleInfo::VehicleInfo() { }

void VehicleInfo::UpdateValues(VehicleExtensions &ext, Vehicle vehicle) {
    Class = findClass(ENTITY::GET_ENTITY_MODEL(vehicle));
    Domain = findDomain(Class);
    IsAmphibious = isAmphibious(ext, vehicle);

    if (!HasSpeedo && ext.GetDashSpeed(vehicle) > 0.0f) {
        HasSpeedo = true;
    }

    if (ext.GetTopGear(vehicle) == 1 || ext.GetVehicleFlags(vehicle)[1] & eVehicleFlag2::FLAG_IS_ELECTRIC)
        HasClutch = false;
    else
        HasClutch = true;
}

VehicleDerivatives::VehicleDerivatives() { }

void VehicleDerivatives::UpdateValues(VehicleExtensions& ext, Vehicle vehicle) {
    auto thisTick = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns

    speedVector = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true);
    wheelCompressions = ext.GetWheelCompressions(vehicle);
    if (prevCompressions.size() != wheelCompressions.size()) {
        prevCompressions.resize(wheelCompressions.size());
        compressionSpeeds.resize(wheelCompressions.size());
    }

    updateDeltas(thisTick);
    prevCompressions = wheelCompressions;
    prevSpeedVector = speedVector;
    prevTick = thisTick;
}

Vector3 VehicleDerivatives::GetRelativeAcceleration() {
    return acceleration;
}

Vector3 VehicleDerivatives::GetRelativeAccelerationAverage() {
    Vector3 result = {};

    for (int i = 0; i < SAMPLES; i++) {
        result.x += accelerationSamples[i].x;
        result.y += accelerationSamples[i].y;
        result.z += accelerationSamples[i].z;
    }

    result.x = result.x / SAMPLES;
    result.y = result.y / SAMPLES;
    result.z = result.z / SAMPLES;
    return result;
}

std::vector<float> VehicleDerivatives::GetWheelCompressionSpeeds() {
    return compressionSpeeds;
}

void VehicleDerivatives::updateDeltas(long long thisTick) {
    const float ns = 1e9f;
    acceleration.x = (speedVector.x - prevSpeedVector.x) / ((thisTick - prevTick) / ns);
    acceleration.y = (speedVector.y - prevSpeedVector.y) / ((thisTick - prevTick) / ns);
    acceleration.z = (speedVector.z - prevSpeedVector.z) / ((thisTick - prevTick) / ns);

    accelerationSamples[averageAccelIndex] = acceleration;
    averageAccelIndex = (averageAccelIndex + 1) % SAMPLES;

    for (int i = 0; i < wheelCompressions.size(); ++i) {
        compressionSpeeds[i] = (wheelCompressions[i] - prevCompressions[i]) / ((thisTick - prevTick) / ns);
    }
}

VehicleClass VehicleInfo::findClass(Hash model) {
    if (VEHICLE::IS_THIS_MODEL_A_CAR(model))        return VehicleClass::Car;
    if (VEHICLE::IS_THIS_MODEL_A_BICYCLE(model))    return VehicleClass::Bicycle;
    if (VEHICLE::IS_THIS_MODEL_A_BIKE(model))       return VehicleClass::Bike;
    if (VEHICLE::IS_THIS_MODEL_A_BOAT(model))       return VehicleClass::Boat;
    if (VEHICLE::IS_THIS_MODEL_A_PLANE(model))      return VehicleClass::Plane;
    if (VEHICLE::IS_THIS_MODEL_A_HELI(model))       return VehicleClass::Heli;
    if (VEHICLE::IS_THIS_MODEL_A_QUADBIKE(model))   return VehicleClass::Quad;
    return VehicleClass::Unknown;
}

VehicleDomain VehicleInfo::findDomain(VehicleClass vehicleClass) {
    switch(vehicleClass) {
        case VehicleClass::Bicycle: return VehicleDomain::Bicycle;
        case VehicleClass::Boat:    return VehicleDomain::Water;
        case VehicleClass:: Heli:
        case VehicleClass::Plane:   return VehicleDomain::Air;
        case VehicleClass::Train:   return VehicleDomain::Rail;
        case VehicleClass::Unknown: return VehicleDomain::Unknown;
        default: return VehicleDomain::Road;
    }
}

bool VehicleInfo::isAmphibious(VehicleExtensions &ext, Vehicle vehicle) {
    auto type = ext.GetModelType(vehicle);
    return (type == 6 || type == 7);
}

EngineValues::EngineValues() {}

void EngineValues::Update(VehicleExtensions &ext, Vehicle vehicle) {
    PrevRPM = RPM;

    if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle))
        RPM = ext.GetCurrentRPM(vehicle);
    else
        RPM = 0.01f;
}
