#include "../../ScriptHookV_SDK/inc/natives.h"

#include "VehicleData.hpp"

VehicleData::VehicleData() { }

void VehicleData::UpdateValues(VehicleExtensions& ext, Vehicle vehicle) {
    Class = findClass(ENTITY::GET_ENTITY_MODEL(vehicle));
    IsTruck = isBadTruck(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(ENTITY::GET_ENTITY_MODEL(vehicle)));

    PrevRPM = RPM;
    RPM = VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) ? ext.GetCurrentRPM(vehicle) : 0.01f;
    SpeedVector = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true);
    WheelCompressions = ext.GetWheelCompressions(vehicle);
    if (prevCompressions.size() != WheelCompressions.size()) {
        prevCompressions = WheelCompressions;
    }
    if (!HasSpeedo && ext.GetDashSpeed(vehicle) > 0.0f) {
        HasSpeedo = true;
    }

    NoClutch = ext.GetTopGear(vehicle) == 1;

    updateAcceleration();
    updateAverageAcceleration();
}

std::vector<float> VehicleData::GetWheelCompressionSpeeds() {
    long long time = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns

    std::vector<float> result;
    for (int i = 0; i < WheelCompressions.size(); i++) {
        result.push_back((WheelCompressions[i] - prevCompressions[i]) / ((time - prevCompressTime) / 1e9f));
    }

    prevCompressTime = time;
    prevCompressions = WheelCompressions;

    return result;
}

void VehicleData::updateAcceleration() {
    long long time = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns

    Vector3 result;
    result.x = (SpeedVector.x - SpeedVectorPrev.x) / ((time - prevAccelTime) / 1e9f);
    result.y = (SpeedVector.y - SpeedVectorPrev.y) / ((time - prevAccelTime) / 1e9f);
    result.z = (SpeedVector.z - SpeedVectorPrev.z) / ((time - prevAccelTime) / 1e9f);
    result._paddingx = 0;
    result._paddingy = 0;
    result._paddingz = 0;

    prevAccelTime = time;
    SpeedVectorPrev = SpeedVector;

    acceleration = result;
}

void VehicleData::updateAverageAcceleration() {
    accelerationSamples[averageAccelIndex] = acceleration;
    averageAccelIndex = (averageAccelIndex + 1) % (SAMPLES - 1);

}

bool VehicleData::isBadTruck(char* name) {
    for (int i = 0; i < badModelNames.size(); i++) {
        if (strcmp(name, badModelNames[i]) == 0)
            return true;
    }
    return false;
}


VehicleData::VehicleClass VehicleData::findClass(Hash model) {
    if (VEHICLE::IS_THIS_MODEL_A_CAR(model))
        return VehicleClass::Car;
    if (VEHICLE::IS_THIS_MODEL_A_BICYCLE(model))
        return VehicleClass::Bicycle;
    if (VEHICLE::IS_THIS_MODEL_A_BIKE(model))
        return VehicleClass::Bike;
    if (VEHICLE::IS_THIS_MODEL_A_BOAT(model))
        return VehicleClass::Boat;
    if (VEHICLE::IS_THIS_MODEL_A_PLANE(model))
        return VehicleClass::Plane;
    if (VEHICLE::IS_THIS_MODEL_A_HELI(model))
        return VehicleClass::Heli;
    if (VEHICLE::IS_THIS_MODEL_A_QUADBIKE(model))
        return VehicleClass::Quad;

    return VehicleClass::Unknown;
}

Vector3 VehicleData::GetRelativeAcceleration() {
    return acceleration;
}

Vector3 VehicleData::GetRelativeAccelerationAverage() const {
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

