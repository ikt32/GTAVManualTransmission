#define NOMINMAX

#include "script.h"
#include "inc/natives.h"

#include "Input/NativeController.h"
#include "Memory/VehicleExtensions.hpp"
#include "Memory/MemoryPatcher.hpp"
#include "ScriptSettings.hpp"
#include "Util/MathExt.h"
#include "Util/Util.hpp"

extern ScriptSettings settings;

extern VehicleExtensions ext;
extern Ped playerPed;
extern Vehicle vehicle;

DWORD	npcVehicleUpdateTime;

void showNPCInfo(Vehicle vehicles[1024], int count) {
    showText(0.9, 0.5, 0.4, "NPC Vehs: " + std::to_string(count));
    bool lookBack = CONTROLS::IS_CONTROL_PRESSED(2, ControlVehicleLookBehind) == TRUE;
    auto vehPos = ENTITY::GET_ENTITY_COORDS(vehicle, true);
    float searchdist = 50.0f;
    float searchfov = 15.0f;
    for (int i = 0; i < count; i++) {
        if (vehicles[i] == 0) continue;
        if (vehicles[i] == vehicle) continue;
        Vector3 targetPos = ENTITY::GET_ENTITY_COORDS(vehicles[i], true);
        Vector3 direction = Normalize(targetPos - vehPos);
        float vehDirection = atan2(direction.y, direction.x) * (180.0f / 3.14159f);
        float myHeading = ENTITY::GET_ENTITY_HEADING(vehicle) + 90.0f;
        if (lookBack) myHeading += 180.0f;
        float latDist = GetAngleBetween(vehDirection, myHeading, searchfov);
        if (latDist < searchfov) {
            float dist = Distance(vehPos, targetPos);
            if (dist < searchdist) {
                float meCloseness = pow(2.0f * (searchdist - dist) / searchdist, 2.0f);
                int drawAlpha = std::min((int)(255 * latDist * meCloseness), 255);
                Color bgColor = transparentGray;
                Color fgColor = solidWhite;
                if (drawAlpha < bgColor.A) bgColor.A = drawAlpha;
                if (drawAlpha < fgColor.A) fgColor.A = drawAlpha;
                auto plate = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(vehicles[i]);

                Color thColor = fgColor;
                float throttle = ext.GetThrottleP(vehicles[i]);

                if (throttle >= 0.0f) {
                    thColor.R = (int)map(throttle, 0.0f, 1.0f, 255.0f, 0.0f);
                    thColor.B = (int)map(throttle, 0.0f, 1.0f, 255.0f, 0.0f);
                }
                else {
                    thColor.B = (int)map(throttle, 0.0f, 1.0f, 255.0f, 0.0f);
                    thColor.G = (int)map(throttle, 0.0f, 1.0f, 255.0f, 0.0f);
                }

                Color brColor = fgColor;
                float brake = ext.GetBrakeP(vehicles[i]);

                brColor.B = (int)map(brake, 0.0f, 1.0f, 255.0f, 0.0f);
                brColor.G = (int)map(brake, 0.0f, 1.0f, 255.0f, 0.0f);

                Color rpmColor = fgColor;
                float rpm = ext.GetCurrentRPM(vehicles[i]);
                rpmColor.G = (int)map(rpm, 0.0f, 1.0f, 255.0f, 165.0f);
                rpmColor.B = (int)map(rpm, 0.0f, 1.0f, 255.0f, 0.0f);


                showDebugInfo3DColors(targetPos,
                { { UI::_GET_LABEL_TEXT(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(ENTITY::GET_ENTITY_MODEL(vehicles[i]))), fgColor },
                { plate, fgColor },
                { "Throttle: " + std::to_string(throttle), thColor },
                { "Brake: " + std::to_string(brake), brColor },
                { "Steer:" + std::to_string(ext.GetSteeringAngle(vehicles[i])), fgColor },
                { "RPM: " + std::to_string(rpm), rpmColor },
                { "Gear: " + std::to_string(ext.GetGearCurr(vehicles[i])), fgColor },
                { "Top Gear: " + std::to_string(ext.GetTopGear(vehicles[i])), fgColor }, },
                    bgColor);
            }
        }
    }
}

void updateNPCVehicles(Vehicle vehicles[1024], int count) {
    if (npcVehicleUpdateTime + 200 < GetTickCount()) {
        npcVehicleUpdateTime = GetTickCount();
        for (int i = 0; i < count; i++) {
            if (vehicles[i] == 0) continue;
            if (vehicles[i] == vehicle) continue;
            if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicles[i])) continue;
            if (ext.GetTopGear(vehicles[i]) == 1) continue;

            int currGear = ext.GetGearCurr(vehicles[i]);
            if (currGear == 0) continue;

            // common
            float currSpeed = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicles[i], true).y;
            auto ratios = ext.GetGearRatios(vehicles[i]);
            float DriveMaxFlatVel = ext.GetDriveMaxFlatVel(vehicles[i]);
            float InitialDriveMaxFlatVel = ext.GetInitialDriveMaxFlatVel(vehicles[i]);

            // Shift up
            float maxSpeedUpShiftWindow = DriveMaxFlatVel / ratios[currGear];
            float minSpeedUpShiftWindow = InitialDriveMaxFlatVel / ratios[currGear];
            float upshiftSpeed = map(ext.GetThrottleP(vehicles[i]), 0.0f, 1.0f, minSpeedUpShiftWindow, maxSpeedUpShiftWindow);

            if (currGear < ext.GetTopGear(vehicles[i])) {
                if (currSpeed > upshiftSpeed) {
                    ext.SetGearNext(vehicles[i], ext.GetGearCurr(vehicles[i]) + 1);
                    continue;
                }
            }

            // Shift down
            float prevGearTopSpeed = DriveMaxFlatVel / ratios[currGear - 1];
            float prevGearMinSpeed = InitialDriveMaxFlatVel / ratios[currGear - 1];
            float highEndShiftSpeed = fminf(minSpeedUpShiftWindow, prevGearTopSpeed);
            float prevGearDelta = prevGearTopSpeed - prevGearMinSpeed;
            float downshiftSpeed = map(ext.GetThrottleP(vehicles[i]), 0.0f, 1.0f, prevGearMinSpeed, highEndShiftSpeed);

            if (currGear > 1) {
                if (currSpeed < downshiftSpeed - prevGearDelta) {
                    ext.SetGearNext(vehicles[i], ext.GetGearCurr(vehicles[i]) - 1);
                }
            }
        }
    }
}

void update_npc() {
    bool mtActive = MemoryPatcher::ShiftUpPatched;

    if (!vehicle || !ENTITY::DOES_ENTITY_EXIST(vehicle) ||
        playerPed != VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1)) {
        return;
    }

    if (!settings.ShowNPCInfo && !(settings.EnableManual && mtActive)) return;

    const int ARR_SIZE = 1024;
    Vehicle vehicles[ARR_SIZE];
    int count = worldGetAllVehicles(vehicles, ARR_SIZE);

    if (settings.ShowNPCInfo) {
        showNPCInfo(vehicles, count);
    }

    if (settings.EnableManual && mtActive) {
        updateNPCVehicles(vehicles, count);
    }
}
