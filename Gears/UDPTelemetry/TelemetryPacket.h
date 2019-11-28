#pragma once
// Codemasters' udp packet for SimHub
struct TelemetryPacket
{
    float Time;
    float LapTime;
    float LapDistance;
    float Distance;

    float X;
    float Y;
    float Z;

    float Speed;

    float WorldSpeedX;
    float WorldSpeedY;
    float WorldSpeedZ;

    float XR;
    float Roll;
    float ZR;

    float XD;
    float YD;
    float ZD;

    float SuspensionPositionRearLeft;
    float SuspensionPositionRearRight;
    float SuspensionPositionFrontLeft;
    float SuspensionPositionFrontRight;

    float SuspensionVelocityRearLeft;
    float SuspensionVelocityRearRight;
    float SuspensionVelocityFrontLeft;
    float SuspensionVelocityFrontRight;

    float WheelSpeedRearLeft;
    float WheelSpeedRearRight;
    float WheelSpeedFrontLeft;
    float WheelSpeedFrontRight;

    float Throttle;
    float Steer;
    float Brake;
    float Clutch;
    float Gear;

    float LateralAcceleration;
    float LongitudinalAcceleration;

    float Lap;

    float EngineRevs;

    /* New Fields in Patch 12 */

    float SliProNativeSupport;     // Always 1?

    float RacePosition;     // Position in race

    float KersRemaining;     // Kers Remaining

    float KersMaxLevel;     // Always 400000?

    float DrsStatus;       // 0 = off, 1 = on

    float TractionControl;     // 0 (off) - 2 (high)

    float AntiLock;     // 0 = All assists are off.  1 = some assist is on.

    float FuelRemaining;      // Not sure if laps or Litres?

    float FuelCapacity;   // 9.5 = race, 10 = time trail / time attack, 170 = quali, practice, championsmode

    float InPits;  // 0 = none, 1 = pitting, 2 = in pit area

    float Sector;   // 0 = sector1, 1 = sector2; 2 = sector3

    float TimeSector1;    // Time Intermediate 1

    float TimeSector2;    // Time Intermediate 2

    float BrakeTemperatureRearLeft; // brakes temperature (centigrade)

    float BrakeTemperatureRearRight; // brakes temperature (centigrade)

    float BrakeTemperatureFrontLeft; // brakes temperature (centigrade)

    float BrakeTemperatureFrontRight; // brakes temperature (centigrade)

    float WheelPressureRearLeft; // wheels pressure PSI

    float WheelPressureRearRight;// wheels pressure PSI

    float WheelPressureFrontLeft; // wheels pressure PSI

    float WheelPressureFrontRight; // wheels pressure PSI

    float CompletedLapsInRace;    // Number of laps Completed (in GP only)

    float TotalLapsInRace;    // Number of laps in GP (GP only)

    float TrackLength;    // Track Length

    float PreviousLapTime;    // Lap time of previous lap

    float MaxRpm;    // Always 0?

    float IdleRpm;    // Always 0?

    float MaxGears;    // Always 0?

    float SessionType;   // 0 = unknown, 1 = practice, 2 = qualifying, 3 = race

    float DrsAllowed;    // 0 = not allowed, 1 = allowed, -1 = invalid / unknown

    float TrackNumber;   // -1 for unknown, 0-21 for tracks

    float FIAFlags;  // -1 = invalid/unknown, 0 = none, 1 = green, 2 = blue, 3 = yellow, 4 = red


};
