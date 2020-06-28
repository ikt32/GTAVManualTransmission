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
#include "ScriptUtils.h"
#ifdef _DEBUG
#include "Dump.h"
#endif

class NPCVehicle;

extern ScriptSettings g_settings;
extern VehicleExtensions g_ext;
extern Ped g_playerPed;
extern Vehicle g_playerVehicle;

std::vector<Vehicle> g_ignoredVehicles;
std::vector<NPCVehicle> g_npcVehicles;

DWORD   raycastUpdateTime = 0;

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

void showNPCInfo(NPCVehicle _npcVehicle) {
    Vehicle npcVehicle = _npcVehicle.GetVehicle();
    if (npcVehicle == 0 || !ENTITY::DOES_ENTITY_EXIST(npcVehicle))
        return;

    bool playerPassenger = !Util::IsPedOnSeat(npcVehicle, g_playerPed, -1) && 
        PED::GET_VEHICLE_PED_IS_IN(g_playerPed, false) == npcVehicle;

    bool lookBack = PAD::IS_CONTROL_PRESSED(2, ControlVehicleLookBehind) == TRUE;
    auto vehPos = ENTITY::GET_ENTITY_COORDS(g_playerVehicle, true);
    float searchdist = 50.0f;
    float searchfov = 15.0f;

    Vector3 targetPos = ENTITY::GET_ENTITY_COORDS(npcVehicle, true);
    Vector3 direction = Normalize(targetPos - vehPos);
    float vehDirection = atan2(direction.y, direction.x) * (180.0f / 3.14159f);
    float myHeading = ENTITY::GET_ENTITY_HEADING(g_playerVehicle) + 90.0f;
    if (lookBack) myHeading += 180.0f;
    float latDist = GetAngleBetween(vehDirection, myHeading, searchfov);
    if (latDist < searchfov || playerPassenger) {
        float dist = Distance(vehPos, targetPos);
        if (dist < searchdist || playerPassenger) {
            float meCloseness = pow(2.0f * (searchdist - dist) / searchdist, 2.0f);
            int drawAlpha = std::min(static_cast<int>(255 * latDist * meCloseness), 255);
            Util::ColorI bgColor = Util::ColorsI::TransparentGray;
            Util::ColorI fgColor = Util::ColorsI::SolidWhite;
            if (drawAlpha < bgColor.A) bgColor.A = drawAlpha;
            if (drawAlpha < fgColor.A) fgColor.A = drawAlpha;
            auto plate = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(npcVehicle);

            Util::ColorI thColor = fgColor;
            float throttle = g_ext.GetThrottleP(npcVehicle);

            if (throttle >= 0.0f) {
                thColor.R = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
                thColor.B = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
            }
            else {
                thColor.B = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
                thColor.G = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
            }

            Util::ColorI brColor = fgColor;
            float brake = g_ext.GetBrakeP(npcVehicle);

            brColor.B = static_cast<int>(map(brake, 0.0f, 1.0f, 255.0f, 0.0f));
            brColor.G = static_cast<int>(map(brake, 0.0f, 1.0f, 255.0f, 0.0f));

            Util::ColorI rpmColor = fgColor;
            float rpm = g_ext.GetCurrentRPM(npcVehicle);
            rpmColor.G = static_cast<int>(map(rpm, 0.0f, 1.0f, 255.0f, 165.0f));
            rpmColor.B = static_cast<int>(map(rpm, 0.0f, 1.0f, 255.0f, 0.0f));

            auto gearStates = _npcVehicle.GetGearbox();
            auto load = gearStates.ThrottleHang - map(g_ext.GetCurrentRPM(npcVehicle), 0.2f, 1.0f, 0.0f, 1.0f);

            showDebugInfo3DColors(targetPos,
                {
                    //{ HUD::_GET_LABEL_TEXT(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(ENTITY::GET_ENTITY_MODEL(npcVehicle))), fgColor },
                    //{ plate, fgColor },
                    { "Throttle: " + std::to_string(throttle), thColor },
                    { "Brake: " + std::to_string(brake), brColor },
                    { "Steer:" + std::to_string(g_ext.GetSteeringAngle(npcVehicle)), fgColor },
                    { "RPM: " + std::to_string(rpm), rpmColor },
                    { fmt::format("Gear: {}/{}", g_ext.GetGearCurr(npcVehicle), g_ext.GetTopGear(npcVehicle)), fgColor },
                    { fmt::format("Load: {}", load), fgColor },
                    { fmt::format("{}Shifting", gearStates.Shifting ? "~g~" : ""), fgColor },
                    { fmt::format("TH: {}", gearStates.ThrottleHang), fgColor },
                },
                bgColor);
        }
    }
}

void showNPCsInfo(const std::vector<NPCVehicle>& vehicles) {
    for (const auto& vehicle : vehicles) {
        if (Util::IsPedOnSeat(vehicle.GetVehicle(), g_playerPed, -1))
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

    auto handlingPtr = g_ext.GetHandlingPtr(npcVehicle);
    // This is x Clutch per second? e.g. changerate 2.5 -> clutch fully (dis)engages in 1/2.5 seconds? or whole thing?
    float rateUp = *reinterpret_cast<float*>(handlingPtr + hOffsets.fClutchChangeRateScaleUpShift);
    float rateDown = *reinterpret_cast<float*>(handlingPtr + hOffsets.fClutchChangeRateScaleDownShift);

    float shiftRate = gearStates.ShiftDirection == ShiftDirection::Up ? rateUp : rateDown;
    shiftRate *= g_settings.ShiftOptions.ClutchRateMult;

    /*
     * 4.0 gives similar perf as base - probably the whole shift takes 1/rate seconds
     * with my extra disengage step, the whole thing should take also 1/rate seconds
     */
    shiftRate = shiftRate * MISC::GET_FRAME_TIME() * 4.0f;

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

    if (!g_settings.Debug.DisableNPCGearbox) {
        auto& gearStates = _npcVehicle.GetGearbox();

        if (npcVehicle == 0 ||
            !ENTITY::DOES_ENTITY_EXIST(npcVehicle))
            return;

        if (gearStates.Shifting)
            return;

        if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(npcVehicle))
            return;

        auto topGear = g_ext.GetTopGear(npcVehicle);
        auto currGear = g_ext.GetGearCurr(npcVehicle);

        // Shift to forward/reverse when "stuck" in the opposite gear?
        if (currGear == 0 && g_ext.GetThrottleP(npcVehicle) > 0.2f) {
            gearStates.Shifting = false;
            shiftTo(gearStates, 1, true);
            gearStates.FakeNeutral = false;
        }

        if (currGear == 1 && g_ext.GetThrottleP(npcVehicle) < -0.2f) {
            gearStates.Shifting = false;
            shiftTo(gearStates, 0, true);
            gearStates.FakeNeutral = false;
        }

        if (topGear == 1 || currGear == 0)
            return;

        float throttle = g_ext.GetThrottleP(npcVehicle);
        auto gearRatios = g_ext.GetGearRatios(npcVehicle);
        float driveMaxFlatVel = g_ext.GetDriveMaxFlatVel(npcVehicle);
        float rpm = g_ext.GetCurrentRPM(npcVehicle);

        if (throttle >= gearStates.ThrottleHang)
            gearStates.ThrottleHang = throttle;
        else if (gearStates.ThrottleHang > 0.0f)
            gearStates.ThrottleHang -= MISC::GET_FRAME_TIME() * g_settings.AutoParams.EcoRate;

        if (gearStates.ThrottleHang < 0.0f)
            gearStates.ThrottleHang = 0.0f;

        float currSpeed = ENTITY::GET_ENTITY_SPEED_VECTOR(npcVehicle, true).y;

        float nextGearMinSpeed = 0.0f; // don't care about top gear
        if (currGear < topGear) {
            nextGearMinSpeed = g_settings.AutoParams.NextGearMinRPM * driveMaxFlatVel / gearRatios[currGear + 1];
        }

        float currGearMinSpeed = g_settings.AutoParams.CurrGearMinRPM * driveMaxFlatVel / gearRatios[currGear];

        float engineLoad = gearStates.ThrottleHang - map(rpm, 0.2f, 1.0f, 0.0f, 1.0f);

        bool skidding = false;
        auto skids = g_ext.GetWheelSkidSmokeEffect(npcVehicle);
        for (uint8_t i = 0; i < g_ext.GetNumWheels(npcVehicle); ++i) {
            if (abs(skids[i]) > 3.5f && g_ext.IsWheelPowered(npcVehicle, i))
                skidding = true;
        }

        // Shift up.
        if (currGear < topGear) {
            if (engineLoad < g_settings.AutoParams.UpshiftLoad && currSpeed > nextGearMinSpeed && !skidding) {
                shiftTo(gearStates, currGear + 1, true);
                gearStates.FakeNeutral = false;
                gearStates.LastUpshiftTime = MISC::GET_GAME_TIMER();
            }
        }

        // Shift down later when ratios are far apart
        float gearRatioRatio = 1.0f;

        if (topGear > 1 && currGear > 1) {
            float thisGearRatio = gearRatios[currGear - 1] / gearRatios[currGear];
            gearRatioRatio = thisGearRatio;
        }

        float rateUp = *reinterpret_cast<float*>(g_ext.GetHandlingPtr(npcVehicle) + hOffsets.fClutchChangeRateScaleUpShift);
        float upshiftDuration = 1.0f / (rateUp * g_settings.ShiftOptions.ClutchRateMult);
        bool tpPassed = MISC::GET_GAME_TIMER() > gearStates.LastUpshiftTime + static_cast<int>(1000.0f * upshiftDuration * g_settings.AutoParams.DownshiftTimeoutMult);

        // Shift down
        if (currGear > 1) {
            if (tpPassed && engineLoad > g_settings.AutoParams.DownshiftLoad* gearRatioRatio || currSpeed < currGearMinSpeed) {
                shiftTo(gearStates, currGear - 1, true);
                gearStates.FakeNeutral = false;
            }
        }
    }

    if (!g_settings.Debug.DisableNPCBrake) {
        // Braking!
        if (MemoryPatcher::BrakePatcher.Patched()) {
            // TODO: Proper way of finding out what the fronts/rears are!
            auto numWheels = g_ext.GetNumWheels(npcVehicle);

            float handlingBrakeForce = *reinterpret_cast<float*>(g_ext.GetHandlingPtr(npcVehicle) + hOffsets.fBrakeForce);
            float bbalF = *reinterpret_cast<float*>(g_ext.GetHandlingPtr(npcVehicle) + hOffsets.fBrakeBiasFront);
            float bbalR = *reinterpret_cast<float*>(g_ext.GetHandlingPtr(npcVehicle) + hOffsets.fBrakeBiasRear);
            float inpBrakeForce = handlingBrakeForce * g_ext.GetBrakeP(npcVehicle);

            if (numWheels == 2) {
                g_ext.SetWheelBrakePressure(npcVehicle, 0, inpBrakeForce * bbalF);
                g_ext.SetWheelBrakePressure(npcVehicle, 1, inpBrakeForce * bbalR);
            }
            else if (numWheels >= 4 && numWheels % 2 == 0) {
                g_ext.SetWheelBrakePressure(npcVehicle, 0, inpBrakeForce * bbalF);
                g_ext.SetWheelBrakePressure(npcVehicle, 1, inpBrakeForce * bbalF);
                for (uint8_t i = 2; i < numWheels; ++i) {
                    g_ext.SetWheelBrakePressure(npcVehicle, i, inpBrakeForce * bbalR);
                }
            }
            else {
                for (uint8_t i = 0; i < numWheels; ++i) {
                    g_ext.SetWheelBrakePressure(npcVehicle, i, inpBrakeForce);
                }
            }
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
        auto raycastCoordA = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(g_playerPed, distMin * cos(angle), distMin * sin(angle), 0.0f);
        auto raycastCoordB = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(g_playerPed, distMax * cos(angle), distMax * sin(angle), 0.0f);
        auto ray = SHAPETEST::_START_SHAPE_TEST_RAY(
            raycastCoordA.x, raycastCoordA.y, raycastCoordA.z, raycastCoordB.x, raycastCoordB.y, raycastCoordB.z, 10, g_playerVehicle, 0);
        BOOL hit;
        Vector3 endCoords, surfaceNormal;
        Entity entity;
        SHAPETEST::GET_SHAPE_TEST_RESULT(ray, &hit, &endCoords, &surfaceNormal, &entity);
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
        if (!ENTITY::DOES_ENTITY_EXIST(vehicle.GetVehicle()))
            continue;

        if (Util::IsPedOnSeat(vehicle.GetVehicle(), g_playerPed, -1))
            continue;

        bool ignored = std::find_if(g_ignoredVehicles.begin(), g_ignoredVehicles.end(), [&](const auto & ignoredVehicle) {
            return ignoredVehicle == vehicle.GetVehicle();
        }) != g_ignoredVehicles.end();
        if (ignored)
            continue;

        updateNPCVehicle(vehicle);

        updateShifting(vehicle.GetVehicle(), vehicle.GetGearbox());
        g_ext.SetGearCurr(vehicle.GetVehicle(), vehicle.GetGearbox().LockGear);
        g_ext.SetGearNext(vehicle.GetVehicle(), vehicle.GetGearbox().LockGear);
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
        bool vehicleMissing = std::find_if(manVehicles.begin(), manVehicles.end(), [&](const auto & npcVehicle) {
            return npcVehicle.GetVehicle() == vehicle;
        }) == manVehicles.end();
        if (vehicleMissing) {
            manVehicles.emplace_back(vehicle);
        }
    }
}

void update_npc() {
    // I only patch brakes for ABS/TCS/ESP when other stuff is also patched
    bool mtActive = MemoryPatcher::NumGearboxPatched > 0;
    if (!g_settings.Debug.DisplayNPCInfo && !mtActive) 
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
            updateNPCVehicleList(raycastVehicles_, g_npcVehicles);

            if (VEHICLE::GET_PED_IN_VEHICLE_SEAT(g_playerVehicle, -1, 0) != g_playerPed) {
                g_npcVehicles.emplace_back(g_playerVehicle);
            }

        }
    }
    else {
        updateNPCVehicleList(vehicles, g_npcVehicles);
    }

    if (g_settings.Debug.DisplayNPCInfo) {
        showText(0.9, 0.5, 0.4, "NPC Vehs: " + std::to_string(count));
        showNPCsInfo(g_npcVehicles);
    }

    if (mtActive) {
        updateNPCVehicles(g_npcVehicles);
    }
}

int filterExceptionNPC(int code, PEXCEPTION_POINTERS ex) {
    logger.Write(FATAL, "[ NPC ] Caught exception 0x%X", code);
    logger.Write(FATAL, "[ NPC ]     Exception address 0x%p", ex->ExceptionRecord->ExceptionAddress);
#ifdef _DEBUG
    DumpStackTrace(ex);
#endif
    return EXCEPTION_EXECUTE_HANDLER;
}

void NPCMain() {
    __try {
        while (true) {
            update_npc();
            WAIT(0);
        }
    }
    __except (filterExceptionNPC(GetExceptionCode(), GetExceptionInformation())) {
        logger.Write(FATAL, "Exception in NPC thread?");
    }
}
