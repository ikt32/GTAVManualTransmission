#include <inc/natives.h>

#include "VehicleData.hpp"
#include "Memory/VehicleFlags.h"

VehicleData::VehicleData() { }

void VehicleData::UpdateValues(VehicleExtensions& ext, Vehicle vehicle) {
    auto thisTick = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns
    Class = findClass(ENTITY::GET_ENTITY_MODEL(vehicle));
    Domain = findDomain(Class);
    Amphibious = isAmphibious(ext, vehicle);

    PrevRPM = RPM;
    RPM = VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) ? ext.GetCurrentRPM(vehicle) : 0.01f;
    speedVector = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true);
    wheelCompressions = ext.GetWheelCompressions(vehicle);
    if (prevCompressions.size() != wheelCompressions.size()) {
        prevCompressions = wheelCompressions;
        compressionSpeeds = std::vector<float>(wheelCompressions.size());
    }
    if (!HasSpeedo && ext.GetDashSpeed(vehicle) > 0.0f) {
        HasSpeedo = true;
    }

    NoClutch = ext.GetTopGear(vehicle) == 1 || ext.GetVehicleFlags(vehicle)[1] & eVehicleFlag2::FLAG_IS_ELECTRIC;

    updateAverages(thisTick);
    prevTick = thisTick;
}

Vector3 VehicleData::GetRelativeAcceleration() {
    return acceleration;
}

Vector3 VehicleData::GetRelativeAccelerationAverage() {
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

std::vector<float> VehicleData::GetWheelCompressionSpeeds() {
    return compressionSpeeds;
}

void VehicleData::updateAverages(long long thisTick) {
    acceleration.x = (speedVector.x - prevSpeedVector.x) / ((thisTick - prevTick) / 1e9f);
    acceleration.y = (speedVector.y - prevSpeedVector.y) / ((thisTick - prevTick) / 1e9f);
    acceleration.z = (speedVector.z - prevSpeedVector.z) / ((thisTick - prevTick) / 1e9f);

    prevSpeedVector = speedVector;

    accelerationSamples[averageAccelIndex] = acceleration;
    averageAccelIndex = (averageAccelIndex + 1) % SAMPLES;

    std::vector<float> result;
    for (int i = 0; i < wheelCompressions.size(); i++) {
        compressionSpeeds[i] = (wheelCompressions[i] - prevCompressions[i]) / ((thisTick - prevTick) / 1e9f);
    }
    prevCompressions = wheelCompressions;
}

VehicleClass VehicleData::findClass(Hash model) {
    if (VEHICLE::IS_THIS_MODEL_A_CAR(model))        return VehicleClass::Car;
    if (VEHICLE::IS_THIS_MODEL_A_BICYCLE(model))    return VehicleClass::Bicycle;
    if (VEHICLE::IS_THIS_MODEL_A_BIKE(model))       return VehicleClass::Bike;
    if (VEHICLE::IS_THIS_MODEL_A_BOAT(model))       return VehicleClass::Boat;
    if (VEHICLE::IS_THIS_MODEL_A_PLANE(model))      return VehicleClass::Plane;
    if (VEHICLE::IS_THIS_MODEL_A_HELI(model))       return VehicleClass::Heli;
    if (VEHICLE::IS_THIS_MODEL_A_QUADBIKE(model))   return VehicleClass::Quad;
    return VehicleClass::Unknown;
}

VehicleDomain VehicleData::findDomain(VehicleClass vehicleClass) {
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

bool VehicleData::isAmphibious(VehicleExtensions &ext, Vehicle vehicle) {
    auto type = ext.GetModelType(vehicle);
    return (type == 6 || type == 7);
}
