#include "UDPTelemetry.h"
#include "TelemetryPacket.h"

#include <inc/natives.h>

void UDPTelemetry::UpdatePacket(Socket& socket, Vehicle vehicle, const VehicleData& vehData, 
    const CarControls& controls, VehicleExtensions& ext) {
    TelemetryPacket packet{};

    packet.Time = static_cast<float>(GAMEPLAY::GET_GAME_TIMER()) / 1000.0f;

    auto worldPos = ENTITY::GET_ENTITY_COORDS(vehicle, true);
    auto worldSpeed = ENTITY::GET_ENTITY_VELOCITY(vehicle);
    auto relRotation = ENTITY::GET_ENTITY_ROTATION(vehicle, 0);
    packet.X = worldPos.x;
    packet.Y = worldPos.y;
    packet.Z = worldPos.z;

    packet.Speed = vehData.mWheelAverageDrivenTyreSpeed;
    packet.WorldSpeedX = worldSpeed.x;
    packet.WorldSpeedY = worldSpeed.y;
    packet.WorldSpeedZ = worldSpeed.z;

    packet.XR = relRotation.x;
    packet.Roll = relRotation.y;
    packet.ZR = relRotation.z;

    if (vehData.mWheelCount == 4) {
        packet.SuspensionPositionRearLeft = vehData.mSuspensionTravel[2];
        packet.SuspensionPositionRearRight = vehData.mSuspensionTravel[3];
        packet.SuspensionPositionFrontLeft = vehData.mSuspensionTravel[0];
        packet.SuspensionPositionFrontRight = vehData.mSuspensionTravel[1];

        packet.SuspensionVelocityRearLeft = vehData.mSuspensionTravelSpeeds[2];
        packet.SuspensionVelocityRearRight = vehData.mSuspensionTravelSpeeds[3];
        packet.SuspensionVelocityFrontLeft = vehData.mSuspensionTravelSpeeds[0];
        packet.SuspensionVelocityFrontRight = vehData.mSuspensionTravelSpeeds[1];

        packet.WheelSpeedRearLeft = vehData.mWheelTyreSpeeds[2];
        packet.WheelSpeedRearRight = vehData.mWheelTyreSpeeds[3];
        packet.WheelSpeedFrontLeft = vehData.mWheelTyreSpeeds[0];
        packet.WheelSpeedFrontRight = vehData.mWheelTyreSpeeds[1];
    }

    packet.Throttle = controls.ThrottleVal;
    packet.Steer = controls.SteerVal;
    packet.Brake = controls.BrakeVal;
    packet.Clutch = controls.ClutchVal;
    packet.Gear = vehData.mGearCurr;

    packet.LateralAcceleration = vehData.mAcceleration.x;
    packet.LongitudinalAcceleration = vehData.mAcceleration.y;

    //packet.MaxRpm = 1.0f;
    //packet.IdleRpm = 0.2f;
    packet.EngineRevs = vehData.mRPM * 600.0f;
    packet.MaxGears = static_cast<float>(vehData.mGearTop);

    packet.FuelCapacity = 65.0f;
    packet.FuelRemaining = ext.GetFuelLevel(vehicle);

    socket.SendPacket(reinterpret_cast<char*>(&packet), sizeof(packet));
}
