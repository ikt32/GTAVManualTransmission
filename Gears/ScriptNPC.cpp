#include "script.h"
#include <set>
#include <inc/natives.h>

#include "Input/NativeController.h"
#include "Memory/VehicleExtensions.hpp"
#include "Memory/MemoryPatcher.hpp"
#include "ScriptSettings.hpp"

#include "Util/MathExt.h"
#include "Util/UIUtils.h"

extern ScriptSettings settings;

extern VehicleExtensions ext;
extern Ped playerPed;
extern Vehicle playerVehicle;

DWORD	npcVehicleUpdateTime = 0;
DWORD   raycastUpdateTime = 0;

std::set<Vehicle> raycastVehicles;

std::vector<Vehicle> ignoredVehicles;

void showNPCInfo(Vehicle npcVehicle, bool allowOccupied) {
    bool lookBack = CONTROLS::IS_CONTROL_PRESSED(2, ControlVehicleLookBehind) == TRUE;
    auto vehPos = ENTITY::GET_ENTITY_COORDS(playerVehicle, true);
    float searchdist = 50.0f;
    float searchfov = 15.0f;
    if (npcVehicle == 0) return;

    if (npcVehicle == playerVehicle && allowOccupied) {
        Vector3 targetPos = ENTITY::GET_ENTITY_COORDS(npcVehicle, true);
        Color bgColor = transparentGray;
        Color fgColor = solidWhite;
        auto plate = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(npcVehicle);

        Color thColor = fgColor;
        float throttle = ext.GetThrottleP(npcVehicle);

        if (throttle >= 0.0f) {
            thColor.R = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
            thColor.B = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
        }
        else {
            thColor.B = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
            thColor.G = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
        }

        Color brColor = fgColor;
        float brake = ext.GetBrakeP(npcVehicle);

        brColor.B = static_cast<int>(map(brake, 0.0f, 1.0f, 255.0f, 0.0f));
        brColor.G = static_cast<int>(map(brake, 0.0f, 1.0f, 255.0f, 0.0f));

        Color rpmColor = fgColor;
        float rpm = ext.GetCurrentRPM(npcVehicle);
        rpmColor.G = static_cast<int>(map(rpm, 0.0f, 1.0f, 255.0f, 165.0f));
        rpmColor.B = static_cast<int>(map(rpm, 0.0f, 1.0f, 255.0f, 0.0f));



        showDebugInfo3DColors(targetPos,
            { { UI::_GET_LABEL_TEXT(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(ENTITY::GET_ENTITY_MODEL(npcVehicle))), fgColor },
                { plate, fgColor },
                { "Throttle: " + std::to_string(throttle), thColor },
                { "Brake: " + std::to_string(brake), brColor },
                { "Steer:" + std::to_string(ext.GetSteeringAngle(npcVehicle)), fgColor },
                { "RPM: " + std::to_string(rpm), rpmColor },
                { "Gear: " + std::to_string(ext.GetGearCurr(npcVehicle)), fgColor },
                { "Top Gear: " + std::to_string(ext.GetTopGear(npcVehicle)), fgColor }, },
            bgColor);
    }

    if (npcVehicle == playerVehicle) return;

    Vector3 targetPos = ENTITY::GET_ENTITY_COORDS(npcVehicle, true);
    Vector3 direction = Normalize(targetPos - vehPos);
    float vehDirection = atan2(direction.y, direction.x) * (180.0f / 3.14159f);
    float myHeading = ENTITY::GET_ENTITY_HEADING(playerVehicle) + 90.0f;
    if (lookBack) myHeading += 180.0f;
    float latDist = GetAngleBetween(vehDirection, myHeading, searchfov);
    if (latDist < searchfov) {
        float dist = Distance(vehPos, targetPos);
        if (dist < searchdist) {
            float meCloseness = pow(2.0f * (searchdist - dist) / searchdist, 2.0f);
            int drawAlpha = std::min(static_cast<int>(255 * latDist * meCloseness), 255);
            Color bgColor = transparentGray;
            Color fgColor = solidWhite;
            if (drawAlpha < bgColor.A) bgColor.A = drawAlpha;
            if (drawAlpha < fgColor.A) fgColor.A = drawAlpha;
            auto plate = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(npcVehicle);

            Color thColor = fgColor;
            float throttle = ext.GetThrottleP(npcVehicle);

            if (throttle >= 0.0f) {
                thColor.R = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
                thColor.B = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
            }
            else {
                thColor.B = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
                thColor.G = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
            }

            Color brColor = fgColor;
            float brake = ext.GetBrakeP(npcVehicle);

            brColor.B = static_cast<int>(map(brake, 0.0f, 1.0f, 255.0f, 0.0f));
            brColor.G = static_cast<int>(map(brake, 0.0f, 1.0f, 255.0f, 0.0f));

            Color rpmColor = fgColor;
            float rpm = ext.GetCurrentRPM(npcVehicle);
            rpmColor.G = static_cast<int>(map(rpm, 0.0f, 1.0f, 255.0f, 165.0f));
            rpmColor.B = static_cast<int>(map(rpm, 0.0f, 1.0f, 255.0f, 0.0f));


            showDebugInfo3DColors(targetPos,
                                  { { UI::_GET_LABEL_TEXT(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(ENTITY::GET_ENTITY_MODEL(npcVehicle))), fgColor },
                                      { plate, fgColor },
                                      { "Throttle: " + std::to_string(throttle), thColor },
                                      { "Brake: " + std::to_string(brake), brColor },
                                      { "Steer:" + std::to_string(ext.GetSteeringAngle(npcVehicle)), fgColor },
                                      { "RPM: " + std::to_string(rpm), rpmColor },
                                      { "Gear: " + std::to_string(ext.GetGearCurr(npcVehicle)), fgColor },
                                      { "Top Gear: " + std::to_string(ext.GetTopGear(npcVehicle)), fgColor }, },
                                  bgColor);
        }
    }
}

void showNPCsInfo(Vehicle vehicles[1024], int count) {
    
    for (int i = 0; i < count; i++) {
        showNPCInfo(vehicles[i], false);
    }
}

void updateNPCVehicle(Vehicle npcVehicle) {
    if (npcVehicle == 0 || !ENTITY::DOES_ENTITY_EXIST(npcVehicle)) return;
    if (std::find(ignoredVehicles.begin(), ignoredVehicles.end(), playerVehicle) != ignoredVehicles.end()) return;
    if (npcVehicle == playerVehicle && VEHICLE::GET_PED_IN_VEHICLE_SEAT(playerVehicle, -1) == playerPed) return;

    if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(npcVehicle)) return;
    if (ext.GetTopGear(npcVehicle) == 1) return;

    int currGear = ext.GetGearCurr(npcVehicle);
    if (currGear == 0) return;

    // common
    float currSpeed = ENTITY::GET_ENTITY_SPEED_VECTOR(npcVehicle, true).y;
    auto ratios = ext.GetGearRatios(npcVehicle);
    float DriveMaxFlatVel = ext.GetDriveMaxFlatVel(npcVehicle);
    float InitialDriveMaxFlatVel = ext.GetInitialDriveMaxFlatVel(npcVehicle);

    // Shift up
    float maxSpeedUpShiftWindow = DriveMaxFlatVel / ratios[currGear];
    float minSpeedUpShiftWindow = InitialDriveMaxFlatVel / ratios[currGear];
    float upshiftSpeed = map(ext.GetThrottleP(npcVehicle), 0.0f, 1.0f, minSpeedUpShiftWindow, maxSpeedUpShiftWindow);

    if (currGear < ext.GetTopGear(npcVehicle)) {
        if (currSpeed > upshiftSpeed) {
            ext.SetGearNext(npcVehicle, ext.GetGearCurr(npcVehicle) + 1);
            return;
        }
    }

    // Shift down
    float prevGearTopSpeed = DriveMaxFlatVel / ratios[currGear - 1];
    float prevGearMinSpeed = InitialDriveMaxFlatVel / ratios[currGear - 1];
    float highEndShiftSpeed = fminf(minSpeedUpShiftWindow, prevGearTopSpeed);
    float prevGearDelta = prevGearTopSpeed - prevGearMinSpeed;
    float downshiftSpeed = map(ext.GetThrottleP(npcVehicle), 0.0f, 1.0f, prevGearMinSpeed, highEndShiftSpeed);

    if (currGear > 1) {
        if (currSpeed < downshiftSpeed - prevGearDelta) {
            ext.SetGearNext(npcVehicle, ext.GetGearCurr(npcVehicle) - 1);
        }
    }
}

std::set<Vehicle> updateRaycastVehicles() {
    uint32_t numCasts = 128;
    float distMin = 4.0f;
    float distMax = 50.0f;
    std::set<Vehicle> uniqueVehicles;
    for (uint32_t i = 0; i < numCasts; ++i) {
        auto angle = static_cast<float>(static_cast<double>(i) / static_cast<double>(numCasts) * 2.0 * M_PI);
        auto raycastCoordA = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(playerPed, distMin * cos(angle), distMin * sin(angle), 0.0f);
        auto raycastCoordB = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(playerPed, distMax * cos(angle), distMax * sin(angle), 0.0f);
        auto ray = WORLDPROBE::_START_SHAPE_TEST_RAY(raycastCoordA.x, raycastCoordA.y, raycastCoordA.z, raycastCoordB.x, raycastCoordB.y, raycastCoordB.z, 10, playerVehicle, 0);
        BOOL hit;
        Vector3 endCoords, surfaceNormal;
        Entity entity;
        WORLDPROBE::GET_SHAPE_TEST_RESULT(ray, &hit, &endCoords, &surfaceNormal, &entity);
        if (hit) {
            if (ENTITY::GET_ENTITY_TYPE(entity) == 2) {
                uniqueVehicles.insert(entity);
            }
        }
    }
    return uniqueVehicles;
}

void updateNPCVehicles(Vehicle vehicles[1024], int count) {
    if (npcVehicleUpdateTime + 200 < GetTickCount()) {
        npcVehicleUpdateTime = GetTickCount();
        for (int i = 0; i < count; ++i) {
            Vehicle npcVehicle = vehicles[i];
            updateNPCVehicle(npcVehicle);
        }
    }

    // ScriptHookV did not return any vehicles, check manually
    if (count == 0) {
        if (raycastUpdateTime + 1000 < GetTickCount()) {
            raycastUpdateTime = GetTickCount();
            raycastVehicles = updateRaycastVehicles();
        }

        // yeah this runs every tick
        for (const auto& rcVehicle : raycastVehicles) {
            updateNPCVehicle(rcVehicle);
            if (settings.ShowNPCInfo)
                showNPCInfo(rcVehicle, false);
        }

        Vehicle vehicle2 = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);
        if (VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle2, -1) != playerPed) {
            updateNPCVehicle(vehicle2);
            if (settings.ShowNPCInfo)
                showNPCInfo(vehicle2, true);
        }
    }
}

void update_npc() {
    bool mtActive = MemoryPatcher::NumGearboxPatched > 0;
    if (!settings.ShowNPCInfo && !mtActive) return;

    const int ARR_SIZE = 1024;
    Vehicle vehicles[ARR_SIZE];
    int count = worldGetAllVehicles(vehicles, ARR_SIZE);
    if (settings.ShowNPCInfo) {
        showText(0.9, 0.5, 0.4, "NPC Vehs: " + std::to_string(count));
        showNPCsInfo(vehicles, count);
    }

    if (mtActive) {
        updateNPCVehicles(vehicles, count);
    }
}

void NPCMain() {
    while (true) {
        update_npc();
        WAIT(0);
    }
}