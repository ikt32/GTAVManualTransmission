#include "../../ScriptHookV_SDK/inc/natives.h"

#include "VehicleData.hpp"

VehicleData::VehicleData() {
    zeroSamples();
}

void VehicleData::Clear() {
    Class = VehicleClass::Car;
    IsTruck = false;
    Rpm = 0.0f;
    Clutch = 1.0f;
    Throttle = 0.0f;
    Turbo = 0.0f;
    Speed = 0.0f;
    Velocity = 0.0f;
    TopGear = 1;
    LockGear = 1;
    CurrGear = 1;
    NextGear = 1;
    PrevGear = 1;
    LockSpeed = 0.0f;
    LockTruck = false;
    SimulatedNeutral = false;
    SpeedVectorPrev.x = 0;
    SpeedVectorPrev.y = 0;
    SpeedVectorPrev.z = 0;
    WheelCompressions.clear();
    prevCompressions.clear();
    HasSpeedo = false;
    zeroSamples();
}

void VehicleData::UpdateRpm() {
    PrevRpm = Rpm;

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

// Updates values from memory and natives
void VehicleData::UpdateValues(VehicleExtensions& ext, Vehicle vehicle) {
    Hash model = ENTITY::GET_ENTITY_MODEL(vehicle);

    CurrGear = ext.GetGearCurr(vehicle);
    NextGear = ext.GetGearNext(vehicle);
    Rpm = VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) ? ext.GetCurrentRPM(vehicle) : 0.01f;
    Clutch = ext.GetClutch(vehicle);
    Throttle = ext.GetThrottle(vehicle);
    Turbo = ext.GetTurbo(vehicle);
    TopGear = ext.GetTopGear(vehicle);
    Speed = ENTITY::GET_ENTITY_SPEED(vehicle);
    SpeedVector = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true);
    Velocity = SpeedVector.y;
    RotationVelocity = ENTITY::GET_ENTITY_ROTATION_VELOCITY(vehicle);
    Class = findClass(model);
    IsTruck = isBadTruck(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(model));
    Pitch = ENTITY::GET_ENTITY_PITCH(vehicle);
    SteeringAngle = ext.GetSteeringAngle(vehicle);
    WheelCompressions = ext.GetWheelCompressions(vehicle);
    DriveBiasFront = ext.GetDriveBiasFront(vehicle);
    if (prevCompressions.size() != WheelCompressions.size()) {
        prevCompressions = WheelCompressions;
    }
    if (!HasSpeedo && ext.GetDashSpeed(vehicle) > 0.1f) {
        HasSpeedo = true;
    }

    NoClutch = TopGear == 1;

    updateAcceleration();
    updateAverageAcceleration();
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

Vector3 VehicleData::getAccelerationVectors() {
    return acceleration;
}

Vector3 VehicleData::getAccelerationVectorsAverage() const {
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


void VehicleData::zeroSamples() {
    for (int i = 0; i < SAMPLES; i++) {
        accelerationSamples[i].x = 0.0f;
        accelerationSamples[i].y = 0.0f;
        accelerationSamples[i].z = 0.0f;
    }
}
