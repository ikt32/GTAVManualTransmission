#include "Dashboard.h"
#include "AWD.h"
#include "ScriptHUD.h"
#include "ScriptSettings.hpp"
#include "VehicleData.hpp"
#include "Memory/VehicleBone.h"

#include "Input/CarControls.hpp"
#include "Util/MathExt.h"
#include "Util/ScriptUtils.h"

#include <GTAVDashHook/DashHook/DashHook.h>

#include <inc/natives.h>

extern ScriptSettings g_settings;
extern CarControls g_controls;
extern VehiclePeripherals g_peripherals;

extern Ped g_playerPed;
extern Vehicle g_playerVehicle;
extern VehicleData g_vehData;

namespace {
    float lastSpeedoRotation = 0.0f;
}

void Dashboard::Update() {
    if (!g_settings.Misc.DashExtensions)
        return;

    if (!Util::VehicleAvailable(g_playerVehicle, g_playerPed))
        return;

    VehicleDashboardData data{};
    DashHook_GetData(&data);

    data.ABSLight |= DashLights::AbsBulbState && DashLights::AbsNotify;

    if (g_peripherals.IgnitionState == IgnitionState::Stall) {
        // data.indicator_left = true;
        // data.indicator_right = true;
        // data.handbrakeLight = true;
        data.engineLight = true;
        data.ABSLight = true;
        data.petrolLight = true;
        data.oilLight = true;
        // data.headlights = true;
        // data.fullBeam = true;
        data.batteryLight = true;
    }

    if (g_settings().DriveAssists.AWD.Enable) {
        if (g_settings().DriveAssists.AWD.SpecialFlags & AWD::AWD_REMAP_DIAL_Y97Y_R32) {
            data.oilPressure = lerp(
                data.oilPressure,
                AWD::GetTransferValue(),
                1.0f - pow(0.0001f, MISC::GET_FRAME_TIME()));
            // https://www.gta5-mods.com/vehicles/nissan-skyline-gt-r-bnr32
            // oil pressure gauge uses data.temp
            // battery voltage uses data.temp
        }
        else if (g_settings().DriveAssists.AWD.SpecialFlags & AWD::AWD_REMAP_DIAL_WANTED188_R32) {
            auto boneIdx = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(g_playerVehicle, "needle_torque");
            if (boneIdx != -1) {
                Vector3 rotAxis{};
                rotAxis.y = 1.0f;

                AWD::GetDisplayValue() = lerp(
                    AWD::GetDisplayValue(), AWD::GetTransferValue(),
                    1.0f - pow(0.0001f, MISC::GET_FRAME_TIME()));

                VehicleBones::RotateAxisAbsolute(g_playerVehicle, boneIdx, rotAxis, AWD::GetDisplayValue());
            }

            boneIdx = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(g_playerVehicle, "needle_speedo");
            if (boneIdx != -1) {
                Vector3 rotAxis{};
                rotAxis.y = 1.0f;

                float rotation = map(abs(g_vehData.mDiffSpeed), 0.0f, 51.0f, 0.0f, deg2rad(240.0f));

                float maxDelta = MISC::GET_FRAME_TIME() * 1.0f;

                if (abs(rotation - lastSpeedoRotation) > maxDelta)
                    rotation = lastSpeedoRotation + maxDelta * sgn(rotation - lastSpeedoRotation);

                lastSpeedoRotation = rotation;

                rotation = std::clamp(lastSpeedoRotation, 0.0f, deg2rad(240.0f));

                VehicleBones::RotateAxisAbsolute(g_playerVehicle, boneIdx, rotAxis, rotation);
            }
        }
    }

    DashHook_SetData(data);
}