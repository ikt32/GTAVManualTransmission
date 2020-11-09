#include "script.h"

#ifdef _DEBUG
#include "Dump.h"
#endif

#include "VehicleData.hpp"
#include "ScriptSettings.hpp"

#include "Memory/VehicleExtensions.hpp"
#include "Memory/MemoryPatcher.hpp"
#include "Memory/Offsets.hpp"

#include "Util/MathExt.h"
#include "Util/UIUtils.h"
#include "Util/ScriptUtils.h"

#include <inc/natives.h>
#include <fmt/format.h>
#include <set>

using VExt = VehicleExtensions;

class NPCVehicle;

extern ScriptSettings g_settings;
extern Ped g_playerPed;
extern Vehicle g_playerVehicle;

std::vector<Vehicle> g_ignoredVehicles;
std::vector<NPCVehicle> g_npcVehicles;

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
            float throttle = VExt::GetThrottleP(npcVehicle);

            if (throttle >= 0.0f) {
                thColor.R = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
                thColor.B = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
            }
            else {
                thColor.B = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
                thColor.G = static_cast<int>(map(throttle, 0.0f, 1.0f, 255.0f, 0.0f));
            }

            Util::ColorI brColor = fgColor;
            float brake = VExt::GetBrakeP(npcVehicle);

            brColor.B = static_cast<int>(map(brake, 0.0f, 1.0f, 255.0f, 0.0f));
            brColor.G = static_cast<int>(map(brake, 0.0f, 1.0f, 255.0f, 0.0f));

            Util::ColorI rpmColor = fgColor;
            float rpm = VExt::GetCurrentRPM(npcVehicle);
            rpmColor.G = static_cast<int>(map(rpm, 0.0f, 1.0f, 255.0f, 165.0f));
            rpmColor.B = static_cast<int>(map(rpm, 0.0f, 1.0f, 255.0f, 0.0f));

            auto gearStates = _npcVehicle.GetGearbox();
            auto load = gearStates.ThrottleHang - map(VExt::GetCurrentRPM(npcVehicle), 0.2f, 1.0f, 0.0f, 1.0f);

            std::vector<std::pair<std::string, Util::ColorI>> dbgLines = {
                    { fmt::format("Throttle: {:.2f}", throttle), thColor },
                    { fmt::format("Brake: {:.2f}", brake), brColor },
                    { fmt::format("Steer: {:.2f}", VExt::GetSteeringAngle(npcVehicle)), fgColor },
                    { fmt::format("RPM: {:.2f}", rpm), rpmColor },
                    { fmt::format("Gear: {}/{}", VExt::GetGearCurr(npcVehicle), VExt::GetTopGear(npcVehicle)), fgColor },
                    { fmt::format("Load: {:.2f}", load), fgColor },
                    { fmt::format("{}Shifting", gearStates.Shifting ? "~g~" : ""), fgColor },
                    { fmt::format("TH: {}", gearStates.ThrottleHang), fgColor },
            };

            if (VEHICLE::IS_TOGGLE_MOD_ON(npcVehicle, VehicleToggleModTurbo)) {
                dbgLines.emplace_back(fmt::format("Turbo: {:.2f}", VExt::GetTurbo(npcVehicle)), fgColor);
            }

            UI::ShowText3DColors(targetPos, dbgLines, bgColor);
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

    auto handlingPtr = VExt::GetHandlingPtr(npcVehicle);
    // This is x Clutch per second? e.g. changerate 2.5 -> clutch fully (dis)engages in 1/2.5 seconds? or whole thing?
    float rateUp = *reinterpret_cast<float*>(handlingPtr + hOffsets.fClutchChangeRateScaleUpShift);
    float rateDown = *reinterpret_cast<float*>(handlingPtr + hOffsets.fClutchChangeRateScaleDownShift);

    float shiftRate = gearStates.ShiftDirection == ShiftDirection::Up ? rateUp : rateDown;
    shiftRate *= g_settings.BaseConfig()->ShiftOptions.ClutchRateMult;

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

    if (gearStates.ClutchVal == 0.0f) {
        if (gearStates.ShiftDirection == ShiftDirection::Up) {
            VExt::SetThrottle(npcVehicle, 0.0f);
            VExt::SetThrottleP(npcVehicle, 0.0f);
        }
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

        auto topGear = VExt::GetTopGear(npcVehicle);
        auto currGear = VExt::GetGearCurr(npcVehicle);

        // Shift to forward/reverse when "stuck" in the opposite gear?
        if (currGear == 0 && VExt::GetThrottleP(npcVehicle) > 0.2f) {
            gearStates.Shifting = false;
            shiftTo(gearStates, 1, true);
            gearStates.FakeNeutral = false;
        }

        if (currGear == 1 && VExt::GetThrottleP(npcVehicle) < -0.2f) {
            gearStates.Shifting = false;
            shiftTo(gearStates, 0, true);
            gearStates.FakeNeutral = false;
        }

        if (topGear == 1 || currGear == 0)
            return;

        float throttle = VExt::GetThrottleP(npcVehicle);
        auto gearRatios = VExt::GetGearRatios(npcVehicle);
        float driveMaxFlatVel = VExt::GetDriveMaxFlatVel(npcVehicle);
        float rpm = VExt::GetCurrentRPM(npcVehicle);

        if (throttle >= gearStates.ThrottleHang)
            gearStates.ThrottleHang = throttle;
        else if (gearStates.ThrottleHang > 0.0f)
            gearStates.ThrottleHang -= MISC::GET_FRAME_TIME() * g_settings.BaseConfig()->AutoParams.EcoRate;

        if (gearStates.ThrottleHang < 0.0f)
            gearStates.ThrottleHang = 0.0f;

        float currSpeed = ENTITY::GET_ENTITY_SPEED_VECTOR(npcVehicle, true).y;

        float nextGearMinSpeed = 0.0f; // don't care about top gear
        if (currGear < topGear) {
            nextGearMinSpeed = g_settings.BaseConfig()->AutoParams.NextGearMinRPM * driveMaxFlatVel / gearRatios[currGear + 1];
        }

        float currGearMinSpeed = g_settings.BaseConfig()->AutoParams.CurrGearMinRPM * driveMaxFlatVel / gearRatios[currGear];

        float engineLoad = gearStates.ThrottleHang - map(rpm, 0.2f, 1.0f, 0.0f, 1.0f);

        bool skidding = false;
        auto skids = VExt::GetWheelTractionVectorLength(npcVehicle);
        for (uint8_t i = 0; i < VExt::GetNumWheels(npcVehicle); ++i) {
            if (abs(skids[i]) > 3.5f && VExt::IsWheelPowered(npcVehicle, i))
                skidding = true;
        }

        float theoryTopSpeed = 1.0f * driveMaxFlatVel / gearRatios[currGear];
        // Also (allow) shifting up if the wheelspeed is greater than possible
        if (skidding && currSpeed > theoryTopSpeed * 1.1f) {
            skidding = false;
        }

        // Shift up.
        if (currGear < topGear) {
            if (engineLoad < g_settings.BaseConfig()->AutoParams.UpshiftLoad && currSpeed > nextGearMinSpeed && !skidding) {
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

        float rateUp = *reinterpret_cast<float*>(VExt::GetHandlingPtr(npcVehicle) + hOffsets.fClutchChangeRateScaleUpShift);
        float upshiftDuration = 1.0f / (rateUp * g_settings.BaseConfig()->ShiftOptions.ClutchRateMult);
        bool tpPassed = MISC::GET_GAME_TIMER() > gearStates.LastUpshiftTime + static_cast<int>(1000.0f * upshiftDuration * g_settings.BaseConfig()->AutoParams.DownshiftTimeoutMult);

        // Shift down
        if (currGear > 1) {
            if (tpPassed && engineLoad > g_settings.BaseConfig()->AutoParams.DownshiftLoad* gearRatioRatio || currSpeed < currGearMinSpeed) {
                shiftTo(gearStates, currGear - 1, true);
                gearStates.FakeNeutral = false;
            }
        }
    }

    if (!g_settings.Debug.DisableNPCBrake) {
        // Braking!
        if (MemoryPatcher::BrakePatcher.Patched()) {
            // TODO: Proper way of finding out what the fronts/rears are!
            auto numWheels = VExt::GetNumWheels(npcVehicle);

            float handlingBrakeForce = *reinterpret_cast<float*>(VExt::GetHandlingPtr(npcVehicle) + hOffsets.fBrakeForce);
            float bbalF = *reinterpret_cast<float*>(VExt::GetHandlingPtr(npcVehicle) + hOffsets.fBrakeBiasFront);
            float bbalR = *reinterpret_cast<float*>(VExt::GetHandlingPtr(npcVehicle) + hOffsets.fBrakeBiasRear);
            float inpBrakeForce = handlingBrakeForce * VExt::GetBrakeP(npcVehicle);

            if (numWheels == 2) {
                VExt::SetWheelBrakePressure(npcVehicle, 0, inpBrakeForce * bbalF);
                VExt::SetWheelBrakePressure(npcVehicle, 1, inpBrakeForce * bbalR);
            }
            else if (numWheels >= 4 && numWheels % 2 == 0) {
                VExt::SetWheelBrakePressure(npcVehicle, 0, inpBrakeForce * bbalF);
                VExt::SetWheelBrakePressure(npcVehicle, 1, inpBrakeForce * bbalF);
                for (uint8_t i = 2; i < numWheels; ++i) {
                    VExt::SetWheelBrakePressure(npcVehicle, i, inpBrakeForce * bbalR);
                }
            }
            else {
                for (uint8_t i = 0; i < numWheels; ++i) {
                    VExt::SetWheelBrakePressure(npcVehicle, i, inpBrakeForce);
                }
            }
        }
    }
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
        VExt::SetGearCurr(vehicle.GetVehicle(), vehicle.GetGearbox().LockGear);
        VExt::SetGearNext(vehicle.GetVehicle(), vehicle.GetGearbox().LockGear);
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

    updateNPCVehicleList(vehicles, g_npcVehicles);

    if (g_settings.Debug.DisplayNPCInfo) {
        UI::ShowText(0.9, 0.5, 0.4, "NPC Vehs: " + std::to_string(count));
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
