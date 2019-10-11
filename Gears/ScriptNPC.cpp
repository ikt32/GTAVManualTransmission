#include "script.h"
#include <set>
#include <inc/natives.h>

#include "Input/NativeController.h"
#include "Memory/VehicleExtensions.hpp"
#include "Memory/MemoryPatcher.hpp"
#include "ScriptSettings.hpp"

#include "Util/MathExt.h"
#include "Util/UIUtils.h"
#include "VehicleData.hpp"
#include "Memory/Offsets.hpp"

#include <fmt/format.h>
extern ScriptSettings settings;

extern VehicleExtensions ext;
extern Ped playerPed;
extern Vehicle playerVehicle;

DWORD	npcVehicleUpdateTime = 0;
DWORD   raycastUpdateTime = 0;

std::vector<Vehicle> ignoredVehicles;

bool IsPedOnSeat(Vehicle vehicle, Ped ped, int seat) {
    Vehicle pedVehicle = PED::GET_VEHICLE_PED_IS_IN(ped, false);
    return vehicle == pedVehicle && VEHICLE::GET_PED_IN_VEHICLE_SEAT(pedVehicle, seat) == ped;
}

class NPCVehicle {
public:
    NPCVehicle(Vehicle vehicle)
        : mVehicle(vehicle)
        , mGearbox() { }
    Vehicle GetVehicle() const {
        return mVehicle;
    }
    VehicleGearboxStates& GetGearbox() {
        return mGearbox;
    }
protected:
    Vehicle mVehicle;
    VehicleGearboxStates mGearbox;
};

std::vector<NPCVehicle> npcVehicles;


void showNPCInfo(NPCVehicle _npcVehicle) {
    Vehicle npcVehicle = _npcVehicle.GetVehicle();
    if (npcVehicle == 0 ||
        !ENTITY::DOES_ENTITY_EXIST(npcVehicle))
        return;

    bool playerPassenger = !IsPedOnSeat(npcVehicle, playerPed, -1) && PED::GET_VEHICLE_PED_IS_IN(playerPed, false) == npcVehicle;

    bool lookBack = CONTROLS::IS_CONTROL_PRESSED(2, ControlVehicleLookBehind) == TRUE;
    auto vehPos = ENTITY::GET_ENTITY_COORDS(playerVehicle, true);
    float searchdist = 50.0f;
    float searchfov = 15.0f;

    Vector3 targetPos = ENTITY::GET_ENTITY_COORDS(npcVehicle, true);
    Vector3 direction = Normalize(targetPos - vehPos);
    float vehDirection = atan2(direction.y, direction.x) * (180.0f / 3.14159f);
    float myHeading = ENTITY::GET_ENTITY_HEADING(playerVehicle) + 90.0f;
    if (lookBack) myHeading += 180.0f;
    float latDist = GetAngleBetween(vehDirection, myHeading, searchfov);
    if (latDist < searchfov || playerPassenger) {
        float dist = Distance(vehPos, targetPos);
        if (dist < searchdist || playerPassenger) {
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

            auto gearStates = _npcVehicle.GetGearbox();
            auto load = gearStates.ThrottleHang - map(ext.GetCurrentRPM(npcVehicle), 0.2f, 1.0f, 0.0f, 1.0f);

            showDebugInfo3DColors(targetPos,
                {
                    //{ UI::_GET_LABEL_TEXT(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(ENTITY::GET_ENTITY_MODEL(npcVehicle))), fgColor },
                    //{ plate, fgColor },
                    { "Throttle: " + std::to_string(throttle), thColor },
                    { "Brake: " + std::to_string(brake), brColor },
                    { "Steer:" + std::to_string(ext.GetSteeringAngle(npcVehicle)), fgColor },
                    { "RPM: " + std::to_string(rpm), rpmColor },
                    { fmt::format("Gear: {}/{}", ext.GetGearCurr(npcVehicle), ext.GetTopGear(npcVehicle)), fgColor },
                    { fmt::format("Load: {}", load), fgColor },
                    { fmt::format("Shifting: {}", gearStates.Shifting), fgColor },
                    { fmt::format("TH: {}", gearStates.ThrottleHang), fgColor },
                },
                bgColor);
        }
    }
}

void showNPCsInfo(const std::vector<NPCVehicle>& vehicles) {
    for (const auto& vehicle : vehicles) {
        if (IsPedOnSeat(vehicle.GetVehicle(), playerPed, -1))
            continue;
        showNPCInfo(vehicle);
    }
}

void shiftTo(VehicleGearboxStates& gearStates, int gear, bool autoClutch) {
    if (autoClutch) {
        if (gearStates.Shifting)
            return;
        gearStates.NextGear = gear;
        gearStates.Shifting = true;
        gearStates.ClutchVal = 0.0f;
        gearStates.ShiftDirection = gear > gearStates.LockGear ? ShiftDirection::Up : ShiftDirection::Down;
    }
    else {
        gearStates.LockGear = gear;
    }
}

void updateShifting(Vehicle npcVehicle, VehicleGearboxStates& gearStates) {
    if (!gearStates.Shifting)
        return;

    auto handlingPtr = ext.GetHandlingPtr(npcVehicle);
    // This is x Clutch per second? e.g. changerate 2.5 -> clutch fully (dis)engages in 1/2.5 seconds? or whole thing?
    float rateUp = *reinterpret_cast<float*>(handlingPtr + hOffsets.fClutchChangeRateScaleUpShift);
    float rateDown = *reinterpret_cast<float*>(handlingPtr + hOffsets.fClutchChangeRateScaleDownShift);

    float shiftRate = gearStates.ShiftDirection == ShiftDirection::Up ? rateUp : rateDown;
    shiftRate *= settings.ClutchRateMult;

    /*
     * 4.0 gives similar perf as base - probably the whole shift takes 1/rate seconds
     * with my extra disengage step, the whole thing should take also 1/rate seconds
     */
    shiftRate = shiftRate * GAMEPLAY::GET_FRAME_TIME() * 4.0f;

    // Something went wrong, abort and just shift to NextGear.
    if (gearStates.ClutchVal > 1.5f) {
        gearStates.ClutchVal = 0.0f;
        gearStates.Shifting = false;
        gearStates.LockGear = gearStates.NextGear;
        return;
    }

    if (gearStates.NextGear != gearStates.LockGear) {
        gearStates.ClutchVal += shiftRate;
    }
    if (gearStates.ClutchVal >= 1.0f && gearStates.LockGear != gearStates.NextGear) {
        gearStates.LockGear = gearStates.NextGear;
        return;
    }
    if (gearStates.NextGear == gearStates.LockGear) {
        gearStates.ClutchVal -= shiftRate;
    }

    if (gearStates.ClutchVal < 0.0f && gearStates.NextGear == gearStates.LockGear) {
        gearStates.ClutchVal = 0.0f;
        gearStates.Shifting = false;
    }
}

void updateNPCVehicle(NPCVehicle& _npcVehicle) {
    Vehicle npcVehicle = _npcVehicle.GetVehicle();
    auto& gearStates = _npcVehicle.GetGearbox();

    if (npcVehicle == 0 ||
        !ENTITY::DOES_ENTITY_EXIST(npcVehicle))
        return;

    if (gearStates.Shifting)
        return;

    if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(npcVehicle)) 
        return;

    auto topGear = ext.GetTopGear(npcVehicle);
    auto currGear = ext.GetGearCurr(npcVehicle);

    if (topGear == 1 || currGear == 0)
        return;

    float throttle = ext.GetThrottleP(npcVehicle);
    auto gearRatios = ext.GetGearRatios(npcVehicle);
    float driveMaxFlatVel = ext.GetDriveMaxFlatVel(npcVehicle);
    float rpm = ext.GetCurrentRPM(npcVehicle);

    if (throttle > gearStates.ThrottleHang)
        gearStates.ThrottleHang = throttle;
    else if (gearStates.ThrottleHang > 0.0f)
        gearStates.ThrottleHang -= GAMEPLAY::GET_FRAME_TIME() * 0.05f; // 0.50f was ecorate

    if (gearStates.ThrottleHang < 0.0f)
        gearStates.ThrottleHang = 0.0f;

    float currSpeed = ENTITY::GET_ENTITY_SPEED_VECTOR(npcVehicle, true).y;// ext.GetDashSpeed(npcVehicle);// vehData.mWheelAverageDrivenTyreSpeed;

    float nextGearMinSpeed = 0.0f; // don't care about top gear
    if (currGear < topGear) {
        nextGearMinSpeed = settings.NextGearMinRPM * driveMaxFlatVel / gearRatios[currGear + 1];
    }

    float currGearMinSpeed = settings.CurrGearMinRPM * driveMaxFlatVel / gearRatios[currGear];

    float engineLoad = gearStates.ThrottleHang - map(rpm, 0.2f, 1.0f, 0.0f, 1.0f);

    bool skidding = false;
    for (auto x : ext.GetWheelSkidSmokeEffect(npcVehicle)) {
        if (abs(x) > 3.5f)
            skidding = true;
    }

    // Shift up.
    if (currGear < topGear) {
        if (engineLoad < settings.UpshiftLoad && currSpeed > nextGearMinSpeed && !skidding) {
            shiftTo(gearStates, currGear + 1, true);
            gearStates.FakeNeutral = false;
            gearStates.LastUpshiftTime = GAMEPLAY::GET_GAME_TIMER();
        }
    }

    // Shift down later when ratios are far apart
    float gearRatioRatio = 1.0f;

    if (topGear > 1) {
        float baseGearRatio = 1.948768f / 3.333333f;
        float thisGearRatio = gearRatios[2] / gearRatios[1];
        gearRatioRatio = baseGearRatio / thisGearRatio;
        gearRatioRatio = map(gearRatioRatio, 1.0f, 2.0f, 1.0f, 4.0f);
    }

    float rateUp = *reinterpret_cast<float*>(ext.GetHandlingPtr(npcVehicle) + hOffsets.fClutchChangeRateScaleUpShift);
    float upshiftDuration = 1.0f / (rateUp * settings.ClutchRateMult);
    bool tpPassed = GAMEPLAY::GET_GAME_TIMER() > gearStates.LastUpshiftTime + static_cast<int>(1000.0f * upshiftDuration * settings.DownshiftTimeoutMult);

    // Shift down
    if (currGear > 1) {
        if (tpPassed && engineLoad > settings.DownshiftLoad * gearRatioRatio || currSpeed < currGearMinSpeed) {
            shiftTo(gearStates, currGear - 1, true);
            gearStates.FakeNeutral = false;
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

void updateNPCVehicles(std::vector<NPCVehicle>& vehicles) {
    for(auto& vehicle : vehicles) {
        if (IsPedOnSeat(vehicle.GetVehicle(), playerPed, -1))
            continue;

        bool ignored = std::find_if(ignoredVehicles.begin(), ignoredVehicles.end(), [&](const auto & ignoredVehicle) {
            return ignoredVehicle.GetVehicle() == vehicle;
        }) != ignoredVehicles.end();
        if (ignored)
            continue;

        updateNPCVehicle(vehicle);

        updateShifting(vehicle.GetVehicle(), vehicle.GetGearbox());
        ext.SetGearCurr(vehicle.GetVehicle(), vehicle.GetGearbox().LockGear);
        ext.SetGearNext(vehicle.GetVehicle(), vehicle.GetGearbox().LockGear);
    }
}

void updateNPCVehicleList(const std::vector<Vehicle>& newVehicles, std::vector<NPCVehicle>& manVehicles) {
    // Remove stale vehicles
    for (auto it = manVehicles.begin(); it != manVehicles.end();) {
        // vehicle disappeared from list
        if (std::find(newVehicles.begin(), newVehicles.end(), it->GetVehicle()) == newVehicles.end()) {
            it = manVehicles.erase(it);
        }
        else {
            ++it;
        }
    }

    // Add new vehicles
    for (const auto& vehicle : newVehicles) {
        if (std::find_if(manVehicles.begin(), manVehicles.end(), [&](const auto & npcVehicle) { return npcVehicle.GetVehicle() == vehicle; }) == manVehicles.end()) {
            manVehicles.emplace_back(vehicle);
        }
    }
}

void update_npc() {
    bool mtActive = MemoryPatcher::NumGearboxPatched > 0;
    if (!settings.ShowNPCInfo && !mtActive) 
        return;

    const int ARR_SIZE = 1024;
    std::vector<Vehicle> vehicles(ARR_SIZE);
    int count = worldGetAllVehicles(vehicles.data(), ARR_SIZE);
    vehicles.resize(count);

    // ScriptHookV did not return any vehicles, check manually
    if (count == 0) {
        if (raycastUpdateTime + 1000 < GetTickCount()) {
            raycastUpdateTime = GetTickCount();
            auto raycastVehicles = updateRaycastVehicles();
            auto raycastVehicles_ = std::vector<Vehicle>(raycastVehicles.begin(), raycastVehicles.end());
            updateNPCVehicleList(raycastVehicles_, npcVehicles);

            if (VEHICLE::GET_PED_IN_VEHICLE_SEAT(playerVehicle, -1) != playerPed) {
                npcVehicles.emplace_back(playerVehicle);
            }

        }
    }
    else {
        updateNPCVehicleList(vehicles, npcVehicles);
    }

    if (settings.ShowNPCInfo) {
        showText(0.9, 0.5, 0.4, "NPC Vehs: " + std::to_string(count));
        showNPCsInfo(npcVehicles);
    }

    if (mtActive) {
        updateNPCVehicles(npcVehicles);
    }
}

void NPCMain() {
    while (true) {
        update_npc();
        WAIT(0);
    }
}