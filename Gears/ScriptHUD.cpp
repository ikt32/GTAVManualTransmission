#include "ScriptHUD.h"
#include <fmt/format.h>
#include <inc/natives.h>
#include <numeric>

#include <menu.h>


#include "CustomSteering.h"
#include "DrivingAssists.h"
#include "Memory/VehicleExtensions.hpp"

#include "Util/MathExt.h"
#include "Util/UIUtils.h"

#include "Input/CarControls.hpp"
#include "VehicleData.hpp"
#include "ScriptSettings.hpp"
#include "WheelInput.h"
#include "Memory/Offsets.hpp"

using VExt = VehicleExtensions;

extern NativeMenu::Menu g_menu;
extern ScriptSettings g_settings;
extern CarControls g_controls;
extern int g_textureWheelId;
extern int g_textureAbsId;
extern int g_textureTcsId;
extern int g_textureEspId;
extern int g_textureBrkId;

extern VehicleGearboxStates g_gearStates;
extern Vehicle g_playerVehicle;
extern VehicleData g_vehData;
extern VehiclePeripherals g_peripherals;

void drawDebugInfo();
void drawGForces();
void drawVehicleWheelInfo();
void drawInputWheelInfo();
void updateDashLights();
void drawDashLights();
void drawLSDInfo();
void drawMouseSteering();

namespace GForce {
    std::vector<std::pair<float, float>> CoordTrails;
    Vector3 PrevWorldVel;
}

namespace DashLights {
    int LastAbsTime = 0;
    int LastTcsTime = 0;
    int LastEspTime = 0;
    int LightDuration = 300; // milliseconds
}

void updateDashLights() {
    const int currentTime = MISC::GET_GAME_TIMER();

    bool abs = false;
    bool tcs = false;
    bool esp = false;

    for (int i = 0; i < g_vehData.mWheelCount; ++i) {
        abs |= g_vehData.mWheelsAbs[i];
        tcs |= g_vehData.mWheelsTcs[i];
        esp |= g_vehData.mWheelsEspO[i] || g_vehData.mWheelsEspU[i];
    }

    if (g_peripherals.IgnitionState == IgnitionState::Stall) {
        abs = true;
        tcs = true;
        esp = true;
    }

    if (g_vehData.mHasABS) {
        for (int i = 0; i < g_vehData.mWheelCount; ++i) {
            abs |= g_vehData.mWheelTyreSpeeds[i] == 0.0f &&
                VExt::GetBrakeP(g_playerVehicle) > 0.0f &&
                g_vehData.mVelocity.y > 3.0f;
        }
    }

    if (abs)
        DashLights::LastAbsTime = currentTime;
    if (tcs)
        DashLights::LastTcsTime = currentTime;
    if (esp)
        DashLights::LastEspTime = currentTime;
}

void drawDashLights() {
    const int currentTime = MISC::GET_GAME_TIMER();

    bool abs = DashLights::LastAbsTime + DashLights::LightDuration >= currentTime;
    bool tcs = DashLights::LastTcsTime + DashLights::LightDuration >= currentTime;
    bool esp = DashLights::LastEspTime + DashLights::LightDuration >= currentTime;
    bool brk = VExt::GetHandbrake(g_playerVehicle);

    const float size = g_settings().HUD.DashIndicators.Size;
    const float txSz = 0.025f * size;
    const float rectSzX = 0.125f * g_settings().HUD.DashIndicators.Size;
    const float rectSzY = 0.050f * g_settings().HUD.DashIndicators.Size;
    const float XPos = g_settings().HUD.DashIndicators.XPos;
    const float YPos = g_settings().HUD.DashIndicators.YPos;

    Util::ColorF absColor{};
    if (g_settings().DriveAssists.ABS.Enable || g_vehData.mHasABS) {
        if (abs) {
            absColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        }
        else {
            absColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        }
    }
    else {
        absColor.A = 0.0f;
    }

    drawTexture(g_textureAbsId, 0, -9998, 100,
        txSz, txSz,
        0.5f, 0.5f, // center of texture
        XPos - 0.045f * size, YPos,
        0.0f, GRAPHICS::_GET_ASPECT_RATIO(FALSE), absColor.R, absColor.G, absColor.B, absColor.A);

    Util::ColorF tcsColor{};
    if (g_settings().DriveAssists.TCS.Enable) {
        if (tcs) {
            tcsColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        }
        else {
            tcsColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        }
    }
    else {
        tcsColor.A = 0.0f;
    }

    drawTexture(g_textureTcsId, 0, -9998, 100,
        txSz, txSz,
        0.5f, 0.5f, // center of texture
        XPos - 0.015f * size, YPos,
        0.0f, GRAPHICS::_GET_ASPECT_RATIO(FALSE), tcsColor.R, tcsColor.G, tcsColor.B, tcsColor.A);

    Util::ColorF espColor{};
    if (g_settings().DriveAssists.ESP.Enable) {
        if (esp) {
            espColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        }
        else {
            espColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        }
    }
    else {
        espColor.A = 0.0f;
    }

    drawTexture(g_textureEspId, 0, -9998, 100,
        txSz, txSz,
        0.5f, 0.5f, // center of texture
        XPos + 0.015f * size, YPos,
        0.0f, GRAPHICS::_GET_ASPECT_RATIO(FALSE), espColor.R, espColor.G, espColor.B, espColor.A);

    Util::ColorF brkColor;
    if (brk) {
        brkColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    }
    else {
        brkColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    }

    drawTexture(g_textureBrkId, 0, -9998, 100,
        txSz, txSz,
        0.5f, 0.5f, // center of texture
        XPos + 0.045f * size, YPos,
        0.0f, GRAPHICS::_GET_ASPECT_RATIO(FALSE), brkColor.R, brkColor.G, brkColor.B, brkColor.A);

    GRAPHICS::DRAW_RECT(XPos, YPos,
        rectSzX, rectSzY,
        0, 0, 0, 127, 0);
}

Vector3 GetAccelVector() {
    Vector3 worldVel = ENTITY::GET_ENTITY_VELOCITY(g_playerVehicle);
    Vector3 worldVelDelta = (worldVel - GForce::PrevWorldVel);

    Vector3 fwdVec = ENTITY::GET_ENTITY_FORWARD_VECTOR(g_playerVehicle);
    Vector3 upVec = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(g_playerVehicle, 0.0f, 0.0f, 1.0f) - ENTITY::GET_ENTITY_COORDS(g_playerVehicle, true);
    Vector3 rightVec = Cross(fwdVec, upVec);

    Vector3 relVelDelta{
        -Dot(worldVelDelta, rightVec), 0,
        Dot(worldVelDelta, fwdVec), 0,
        Dot(worldVelDelta, upVec), 0,
    };

    GForce::PrevWorldVel = worldVel;

    return relVelDelta * (1.0f / MISC::GET_FRAME_TIME());
}

void drawGForces() {
    using namespace GForce;
    float locX = g_settings.Debug.Metrics.GForce.PosX;
    float locY = g_settings.Debug.Metrics.GForce.PosY;
    float szX = g_settings.Debug.Metrics.GForce.Size / GRAPHICS::_GET_ASPECT_RATIO(FALSE);
    float szY = g_settings.Debug.Metrics.GForce.Size;

    // menu width = 0.225f
    bool screenLocationConflict = 
        locX > g_menu.menuX - 0.225f / 2.0f && 
        locX < g_menu.menuX + 0.225f / 2.0f;

    if (g_menu.IsThisOpen() && screenLocationConflict)
        return;

    Vector3 accel = GetAccelVector();

    float GForceX = accel.x / 9.8f;
    float GForceY = accel.y / 9.8f;
    float GForceZ = accel.z / 9.8f;

    UI::ShowText(locX + 0.100f, locY - 0.075f, 0.5f, fmt::format("LAT: {:.2f} g", GForceX));
    UI::ShowText(locX + 0.100f, locY - 0.025f, 0.5f, fmt::format("LON: {:.2f} g", GForceY));
    UI::ShowText(locX + 0.100f, locY + 0.025f, 0.5f, fmt::format("VERT: {:.2f} g", GForceZ));
    
    // 1 div = 1G, entire thing = 2g
    float offX = (szX * 0.5f) * GForceX * 0.5f;
    float offY = (szY * 0.5f) * GForceY * 0.5f;

    CoordTrails.emplace_back(offX, offY);
    while (CoordTrails.size() > 15) {
        CoordTrails.erase(CoordTrails.begin());
    }

    GRAPHICS::DRAW_RECT(locX, locY, szX, szY, 0, 0, 0, 127, 0);
    GRAPHICS::DRAW_RECT(locX, locY, 0.001f, szY, 255, 255, 255, 127, 0);
    GRAPHICS::DRAW_RECT(locX, locY, szX, 0.001f, 255, 255, 255, 127, 0);

    GRAPHICS::DRAW_RECT(locX + 0.25f * szX, locY, 0.001f, szY, 127, 127, 127, 127, 0);
    GRAPHICS::DRAW_RECT(locX, locY + 0.25f * szY, szX, 0.001f, 127, 127, 127, 127, 0);

    GRAPHICS::DRAW_RECT(locX - 0.25f * szX, locY, 0.001f, szY, 127, 127, 127, 127, 0);
    GRAPHICS::DRAW_RECT(locX, locY - 0.25f * szY, szX, 0.001f, 127, 127, 127, 127, 0);

    std::vector<float> allX;
    std::vector<float> allY;

    int alpha = 0;
    for (auto it = CoordTrails.begin(); it != CoordTrails.end(); ++it) {
        auto c = *it;
        if (std::next(it) == CoordTrails.end()) {
            GRAPHICS::DRAW_RECT(locX + c.first, locY + c.second, szX * 0.025f, szY * 0.025f, 255, 255, 255, 255, 0);
            allX.push_back(locX + c.first);
            allY.push_back(locY + c.second);
        }
        else {
            GRAPHICS::DRAW_RECT(locX + c.first, locY + c.second, szX * 0.025f, szY * 0.025f, 127, 127, 127, alpha, 0);
            allX.push_back(locX + c.first);
            allY.push_back(locY + c.second);
        }
        alpha += 255 / static_cast<int>(CoordTrails.size());
    }

    GRAPHICS::DRAW_RECT(avg(allX), avg(allY), szX * 0.020f, szY * 0.020f, 255, 0, 0, 255, 0);
}

void drawRPMIndicator(float x, float y, float width, float height, Util::ColorI fg, Util::ColorI bg, float rpm) {
    float bgpaddingx = 0.00f;
    float bgpaddingy = 0.01f;
    // background
    GRAPHICS::DRAW_RECT(x, y, width + bgpaddingx, height + bgpaddingy, bg.R, bg.G, bg.B, bg.A, 0);

    // rpm bar
    GRAPHICS::DRAW_RECT(x - width*0.5f + rpm*width*0.5f, y, width*rpm, height, fg.R, fg.G, fg.B, fg.A, 0);
}

void drawRPMIndicator() {
    Util::ColorI background = {
        g_settings().HUD.RPMBar.BgR,
        g_settings().HUD.RPMBar.BgG,
        g_settings().HUD.RPMBar.BgB,
        g_settings().HUD.RPMBar.BgA
    };

    Util::ColorI foreground = {
        g_settings().HUD.RPMBar.FgR,
        g_settings().HUD.RPMBar.FgG,
        g_settings().HUD.RPMBar.FgB,
        g_settings().HUD.RPMBar.FgA
    };

    Util::ColorI rpmcolor = foreground;
    if (g_vehData.mRPM > g_settings().HUD.RPMBar.Redline) {
        Util::ColorI redline = {
            g_settings().HUD.RPMBar.RedlineR,
            g_settings().HUD.RPMBar.RedlineG,
            g_settings().HUD.RPMBar.RedlineB,
            g_settings().HUD.RPMBar.RedlineA
        };
        rpmcolor = redline;
    }
    float ratio = VExt::GetGearRatios(g_playerVehicle)[VExt::GetGearCurr(g_playerVehicle)];
    float minUpshift = VExt::GetInitialDriveMaxFlatVel(g_playerVehicle);
    float maxUpshift = VExt::GetDriveMaxFlatVel(g_playerVehicle);
    if (g_vehData.mRPM > map(minUpshift / ratio, 0.0f, maxUpshift / ratio, 0.0f, 1.0f)) {
        Util::ColorI rpmlimiter = {
            g_settings().HUD.RPMBar.RevLimitR,
            g_settings().HUD.RPMBar.RevLimitG,
            g_settings().HUD.RPMBar.RevLimitB,
            g_settings().HUD.RPMBar.RevLimitA
        };
        rpmcolor = rpmlimiter;
    }

    float rpm = g_vehData.mRPM;

    // Stall indicator
    if (Math::Near(g_vehData.mRPM, 0.2f, 0.01f) && g_gearStates.StallProgress > 0.0f) {
        rpm = map(g_gearStates.StallProgress, 0.0f, 1.0f, 0.2f, 0.0f);

        auto hsvColor = Util::RGB2HSV(Util::ColorF {
            static_cast<float>(rpmcolor.R) / 255.0f,
            static_cast<float>(rpmcolor.G) / 255.0f,
            static_cast<float>(rpmcolor.B) / 255.0f,
            1.0f, // alpha is ignored anyway
        });

        // hue: transition orange to red
        hsvColor.R = map(rpm, 0.0f, 0.2f, 0.0f, 30.0f);        

        // sat: transition whatever to full saturation
        hsvColor.G = std::clamp(map(rpm, 0.1f, 0.2f, 1.0f, hsvColor.G), 0.0f, 1.0f);

        // val: transition whatever to max saturated brightness
        hsvColor.B = std::clamp(map(rpm, 0.1f, 0.2f, 1.0f, hsvColor.B), 0.0f, 1.0f);

        auto rgbF = Util::HSV2RGB(hsvColor);

        rpmcolor = Util::ColorI {
            static_cast<int>(rgbF.R * 255.0f),
            static_cast<int>(rgbF.G * 255.0f),
            static_cast<int>(rgbF.B * 255.0f),
            rpmcolor.A,
        };
    }

    drawRPMIndicator(
        g_settings().HUD.RPMBar.XPos,
        g_settings().HUD.RPMBar.YPos,
        g_settings().HUD.RPMBar.XSz,
        g_settings().HUD.RPMBar.YSz,
        rpmcolor,
        background,
        rpm
    );
}

std::string formatSpeedo(std::string units, float speed, bool showUnit, int hudFont) {
    if (units == "kph") 
        speed = speed * 3.6f;

    if (units == "mph") 
        speed = speed / 0.44704f;

    if (hudFont != 2 && units == "kph") 
        units = "km/h";

    if (hudFont != 2 && units == "ms") 
        units = "m/s";

    std::string str = fmt::format("{:03.0f}", speed);

    if (showUnit) 
        str = fmt::format("{} {}", str, units);

    return str;
}

void drawSpeedoMeter() {
    float dashms = g_vehData.mHasSpeedo ? VExt::GetDashSpeed(g_playerVehicle) : abs(ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true).y);
    const Util::ColorI color {
        g_settings().HUD.Speedo.ColorR,
        g_settings().HUD.Speedo.ColorG,
        g_settings().HUD.Speedo.ColorB,
        255
    };
    UI::ShowText(g_settings().HUD.Speedo.XPos, g_settings().HUD.Speedo.YPos, g_settings().HUD.Speedo.Size,
        formatSpeedo(g_settings().HUD.Speedo.Speedo, dashms, g_settings().HUD.Speedo.ShowUnit, g_settings().HUD.Font),
        g_settings().HUD.Font, color, g_settings().HUD.Outline);
}

void drawShiftModeIndicator() {
    std::string shiftModeText;
    Util::ColorI color {
        g_settings().HUD.ShiftMode.ColorR,
        g_settings().HUD.ShiftMode.ColorG,
        g_settings().HUD.ShiftMode.ColorB,
        255
    };
    switch (g_settings().MTOptions.ShiftMode) {
        case EShiftMode::Sequential:    shiftModeText = "S"; break;
        case EShiftMode::HPattern:      shiftModeText = "H"; break;
        case EShiftMode::Automatic:     shiftModeText = "A"; break;
        default: shiftModeText = ""; break;
    }
    if (!g_settings.MTOptions.Enable) {
        shiftModeText = "A";
        color = { 0, 126, 232, 255 };
    }
    UI::ShowText(g_settings().HUD.ShiftMode.XPos, g_settings().HUD.ShiftMode.YPos, g_settings().HUD.ShiftMode.Size, 
        shiftModeText, g_settings().HUD.Font, color, g_settings().HUD.Outline);
}

void drawGearIndicator() {
    std::string gear = std::to_string(VExt::GetGearCurr(g_playerVehicle));
    if (VExt::GetHandbrake(g_playerVehicle)) {
        gear = "P";
    }
    else if (g_gearStates.FakeNeutral && g_settings.MTOptions.Enable) {
        gear = "N";
    }
    else if (VExt::GetGearCurr(g_playerVehicle) == 0) {
        gear = "R";
    }
    Util::ColorI color {
        g_settings().HUD.Gear.ColorR,
        g_settings().HUD.Gear.ColorG,
        g_settings().HUD.Gear.ColorB,
        255
    };
    if (VExt::GetGearCurr(g_playerVehicle) == VExt::GetTopGear(g_playerVehicle)) {
        color.R = g_settings().HUD.Gear.TopColorR;
        color.G = g_settings().HUD.Gear.TopColorG;
        color.B = g_settings().HUD.Gear.TopColorB;
        color.A = 255;
    }
    UI::ShowText(g_settings().HUD.Gear.XPos, g_settings().HUD.Gear.YPos, g_settings().HUD.Gear.Size, 
        gear, g_settings().HUD.Font, color, g_settings().HUD.Outline);
}

void MTHUD::UpdateHUD() {
    updateDashLights();

    // main HUD elements
    if (g_settings().HUD.Enable && g_vehData.mDomain == VehicleDomain::Road &&
        (g_settings.MTOptions.Enable || g_settings().HUD.Always)) {
        if (g_settings().HUD.Gear.Enable) {
            drawGearIndicator();
        }
        if (g_settings().HUD.ShiftMode.Enable) {
            drawShiftModeIndicator();
        }
        if (g_settings().HUD.Speedo.Speedo == "kph" ||
            g_settings().HUD.Speedo.Speedo == "mph" ||
            g_settings().HUD.Speedo.Speedo == "ms") {
            drawSpeedoMeter();
        }
        if (g_settings().HUD.RPMBar.Enable) {
            drawRPMIndicator();
        }

        if (g_settings().HUD.DashIndicators.Enable) {
            drawDashLights();
        }

        if (g_settings().HUD.MouseSteering.Enable &&
            g_settings().CustomSteering.Mouse.Enable &&
            g_settings().CustomSteering.Mode > 0) {
            drawMouseSteering();
        }
    }

    // wheel stuff
    if (g_settings().HUD.Enable &&
        (g_vehData.mDomain == VehicleDomain::Road || g_vehData.mDomain == VehicleDomain::Water) &&
        (g_controls.PrevInput == CarControls::Wheel || g_settings().HUD.Wheel.Always) &&
        g_settings().HUD.Wheel.Enable && g_textureWheelId != -1) {
        drawInputWheelInfo();
    }

    if (g_settings.Debug.DisplayFFBInfo) {
        WheelInput::DrawDebugLines();
    }

    // debug stuff
    if (g_settings.Debug.DisplayInfo) {
        drawDebugInfo();
        drawLSDInfo();
    }
    if (g_settings.Debug.DisplayWheelInfo) {
        drawVehicleWheelInfo();
    }
    if (g_settings.Debug.Metrics.GForce.Enable) {
        drawGForces();
    }

    if (g_settings.Debug.DisplayInfo &&
        g_settings.CustomSteering.Mode > 0 &&
        g_controls.PrevInput != CarControls::Wheel &&
        g_vehData.mDomain == VehicleDomain::Road) {
        CustomSteering::DrawDebug();
    }
}

void drawDebugInfo() {
    if (!g_menu.IsThisOpen()) {
        UI::ShowText(0.01, 0.250, 0.3, fmt::format("Address: 0x{:X}", reinterpret_cast<uintptr_t>(VExt::GetAddress(g_playerVehicle))));
        UI::ShowText(0.01, 0.275, 0.3, fmt::format("Mod Enabled:\t\t{}" , g_settings.MTOptions.Enable));
        UI::ShowText(0.01, 0.300, 0.3, fmt::format("RPM:\t\t\t{:.3f}", g_vehData.mRPM));
        UI::ShowText(0.01, 0.325, 0.3, fmt::format("Current Gear:\t\t{}", VExt::GetGearCurr(g_playerVehicle)));
        UI::ShowText(0.01, 0.350, 0.3, fmt::format("Next Gear:\t\t{}", VExt::GetGearNext(g_playerVehicle)));
        UI::ShowText(0.01, 0.375, 0.3, fmt::format("Clutch:\t\t\t{:.2f}", VExt::GetClutch(g_playerVehicle)));
        UI::ShowText(0.01, 0.400, 0.3, fmt::format("Throttle:\t\t\t{:.2f}", VExt::GetThrottle(g_playerVehicle)));
        UI::ShowText(0.01, 0.425, 0.3, fmt::format("Turbo:\t\t\t{:.2f}", VExt::GetTurbo(g_playerVehicle)));
        UI::ShowText(0.01, 0.450, 0.3, fmt::format("{}Speedo", g_vehData.mHasSpeedo ? "~g~" : "~r~"));
        UI::ShowText(0.01, 0.475, 0.3, fmt::format("{}E {}CVT -> {}Clutch",
            g_vehData.mIsElectric ? "~g~" : "~r~", g_vehData.mIsCVT ? "~g~" : "~r~",
            g_vehData.mHasClutch ? "~g~" : "~r~"));
        UI::ShowText(0.01, 0.500, 0.3, fmt::format("{}ABS",
            g_vehData.mHasABS ? "~g~" : "~r~"));

        UI::ShowText(0.01, 0.550, 0.3, fmt::format("{}Shifting", g_gearStates.Shifting ? "~g~" : "~r~"));
        UI::ShowText(0.01, 0.575, 0.3, fmt::format("Clutch: {}" ,g_gearStates.ClutchVal));
        UI::ShowText(0.01, 0.600, 0.3, fmt::format("Lock: {}" ,g_gearStates.LockGear));
        UI::ShowText(0.01, 0.625, 0.3, fmt::format("Next: {}" ,g_gearStates.NextGear));

        // Old automatic gearbox
        if (!g_settings().AutoParams.UsingATCU) {
            UI::ShowText(0.01, 0.650, 0.3, fmt::format("{}Load/upReq: {:.3f}\t/{:.3f}",
                g_gearStates.Shifting ? "~c~" : "", g_gearStates.EngineLoad, g_gearStates.UpshiftLoad));
            UI::ShowText(0.01, 0.675, 0.3, fmt::format("{}Load/dnReq: {:.3f}\t/{:.3f}",
                g_gearStates.Shifting ? "~c~" : "", g_gearStates.EngineLoad, g_gearStates.DownshiftLoad));
        }
        // Nyconing's ATCU
        else {
            UI::ShowText(0.01, 0.650, 0.3, fmt::format("Next optimal up-shifting: {:.2f}%", g_gearStates.Atcu.upshiftingIndex * 100.0f));
            UI::ShowText(0.01, 0.675, 0.3, fmt::format("Next optimal down-shifting: {:.2f}%", g_gearStates.Atcu.downshiftingIndex * 100.0f));
        }
    }

    UI::ShowText(0.85, 0.050, 0.4, fmt::format("Throttle:\t{:.3f}", g_controls.ThrottleVal) , 4);
    UI::ShowText(0.85, 0.075, 0.4, fmt::format("Brake:\t\t{:.3f}" , g_controls.BrakeVal)    , 4);
    UI::ShowText(0.85, 0.100, 0.4, fmt::format("Clutch:\t\t{:.3f}", g_controls.ClutchVal)   , 4);
    UI::ShowText(0.85, 0.125, 0.4, fmt::format("Handb:\t\t{:.3f}" , g_controls.HandbrakeVal), 4);

    if (g_settings.Wheel.Options.Enable)
        UI::ShowText(0.85, 0.150, 0.4, fmt::format("Wheel {} present", g_controls.WheelAvailable() ? "" : " not"), 4);
    

    if (g_settings.Debug.DisplayGearingInfo) {
        auto ratios = VExt::GetGearRatios(g_playerVehicle);
        float DriveMaxFlatVel = VExt::GetDriveMaxFlatVel(g_playerVehicle);

        int i = 0;
        UI::ShowText(0.30f, 0.05f, 0.35f, "Ratios");
        for (auto ratio : ratios) {
            UI::ShowText(0.30f, 0.10f + 0.025f * i, 0.35f, fmt::format("G{}: {:.3f}", i, ratio));
            i++;
        }

        i = 0;
        UI::ShowText(0.45f, 0.05f, 0.35f, "DriveMaxFlatVel");
        for (auto ratio : ratios) {
            float maxSpeed = DriveMaxFlatVel / ratio;
            UI::ShowText(0.45f, 0.10f + 0.025f * i, 0.35f, fmt::format("G{}: {:.3f}", i, maxSpeed));
            i++;
        }

        float rateUp = *reinterpret_cast<float*>(VExt::GetHandlingPtr(g_playerVehicle) + hOffsets.fClutchChangeRateScaleUpShift);
        float rateDown = *reinterpret_cast<float*>(VExt::GetHandlingPtr(g_playerVehicle) + hOffsets.fClutchChangeRateScaleDownShift);
        float upshiftDuration = 1.0f / (rateUp * g_settings().ShiftOptions.ClutchRateMult);
        float downshiftDuration = 1.0f / (rateDown * g_settings().ShiftOptions.ClutchRateMult);

        UI::ShowText(0.60f, 0.050f, 0.35f, fmt::format("ClutchRate Up: {:.3f}", rateUp));
        UI::ShowText(0.60f, 0.075f, 0.35f, fmt::format("ClutchRate Dn: {:.3f}", rateDown));
        UI::ShowText(0.60f, 0.100f, 0.35f, fmt::format("Duration Up: {:.3f}", upshiftDuration));
        UI::ShowText(0.60f, 0.125f, 0.35f, fmt::format("Duration Dn: {:.3f}", downshiftDuration));
        UI::ShowText(0.60f, 0.150f, 0.35f, fmt::format("Shift timeout (dn): {:.3f}", downshiftDuration * g_settings().AutoParams.DownshiftTimeoutMult));
    }
}

void drawInputWheelInfo() {
    // Steering Wheel
    float rotation = 0.0f;
    if (g_controls.PrevInput == CarControls::Wheel) 
        rotation = g_settings().Wheel.Steering.AngleMax * (g_controls.SteerVal - 0.5f);
    else if (g_settings().CustomSteering.Mode > 0 &&
        g_settings().CustomSteering.CustomRotation)
        rotation = g_settings().CustomSteering.CustomRotationDegrees * 0.5f * -VExt::GetSteeringInputAngle(g_playerVehicle);
    else
        rotation = 90.0f * -VExt::GetSteeringInputAngle(g_playerVehicle);

    drawTexture(g_textureWheelId, 0, -9998, 100,
        g_settings().HUD.Wheel.ImgSize, g_settings().HUD.Wheel.ImgSize,
        0.5f, 0.5f, // center of texture
        g_settings().HUD.Wheel.ImgXPos, g_settings().HUD.Wheel.ImgYPos,
        rotation / 360.0f, GRAPHICS::_GET_ASPECT_RATIO(FALSE), 1.0f, 1.0f, 1.0f, 1.0f);

    // Pedals
    float barWidth = g_settings().HUD.Wheel.PedalXSz / 3.0f;

    float barYBase = (g_settings().HUD.Wheel.PedalYPos + g_settings().HUD.Wheel.PedalYSz * 0.5f);

    GRAPHICS::DRAW_RECT(g_settings().HUD.Wheel.PedalXPos, g_settings().HUD.Wheel.PedalYPos, 3.0f * barWidth + g_settings().HUD.Wheel.PedalXPad, g_settings().HUD.Wheel.PedalYSz + g_settings().HUD.Wheel.PedalYPad, 
        0, 0, 0, g_settings().HUD.Wheel.PedalBgA, 0);
    GRAPHICS::DRAW_RECT(g_settings().HUD.Wheel.PedalXPos + 1.0f * barWidth, barYBase - g_controls.ThrottleVal * g_settings().HUD.Wheel.PedalYSz * 0.5f,
        barWidth, g_controls.ThrottleVal * g_settings().HUD.Wheel.PedalYSz, 
        g_settings().HUD.Wheel.PedalThrottleR, g_settings().HUD.Wheel.PedalThrottleG, g_settings().HUD.Wheel.PedalThrottleB, g_settings().HUD.Wheel.PedalThrottleA, 0);
    GRAPHICS::DRAW_RECT(g_settings().HUD.Wheel.PedalXPos + 0.0f * barWidth, barYBase - g_controls.BrakeVal * g_settings().HUD.Wheel.PedalYSz * 0.5f,
        barWidth, g_controls.BrakeVal * g_settings().HUD.Wheel.PedalYSz,
        g_settings().HUD.Wheel.PedalBrakeR, g_settings().HUD.Wheel.PedalBrakeG, g_settings().HUD.Wheel.PedalBrakeB, g_settings().HUD.Wheel.PedalBrakeA, 0);
    GRAPHICS::DRAW_RECT(g_settings().HUD.Wheel.PedalXPos - 1.0f * barWidth, barYBase - g_controls.ClutchVal * g_settings().HUD.Wheel.PedalYSz * 0.5f,
        barWidth, g_controls.ClutchVal * g_settings().HUD.Wheel.PedalYSz,
        g_settings().HUD.Wheel.PedalClutchR, g_settings().HUD.Wheel.PedalClutchG, g_settings().HUD.Wheel.PedalClutchB, g_settings().HUD.Wheel.PedalClutchA, 0);
}

std::vector<Vector3> GetWheelCoords(Vehicle handle) {
    std::vector<Vector3> worldCoords;
    std::vector<Vector3> positions = VExt::GetWheelOffsets(handle);
    Vector3 position = ENTITY::GET_ENTITY_COORDS(g_playerVehicle, true);
    Vector3 rotation = ENTITY::GET_ENTITY_ROTATION(g_playerVehicle, 0);
    rotation.x = deg2rad(rotation.x);
    rotation.y = deg2rad(rotation.y);
    rotation.z = deg2rad(rotation.z);
    Vector3 direction = ENTITY::GET_ENTITY_FORWARD_VECTOR(g_playerVehicle);

    worldCoords.reserve(positions.size());
    for (Vector3 wheelPos : positions) {
        worldCoords.emplace_back(GetOffsetInWorldCoords(position, rotation, direction, wheelPos));
    }
    return worldCoords;
}

void drawVehicleWheelInfo() {
    auto numWheels = VExt::GetNumWheels(g_playerVehicle);
    auto wheelsSpeed = VExt::GetTyreSpeeds(g_playerVehicle);
    auto wheelsCompr = VExt::GetWheelCompressions(g_playerVehicle);
    auto wheelsHealt = VExt::GetWheelHealths(g_playerVehicle);
    auto wheelsContactCoords = VExt::GetWheelLastContactCoords(g_playerVehicle);
    auto wheelsOnGround = VExt::GetWheelsOnGround(g_playerVehicle);

    auto wheelCoords = GetWheelCoords(g_playerVehicle);
    auto wheelsPower = VExt::GetWheelPower(g_playerVehicle);
    auto wheelsBrake = VExt::GetWheelBrakePressure(g_playerVehicle);
    auto wheelDims = VExt::GetWheelDimensions(g_playerVehicle);

    for (int i = 0; i < numWheels; i++) {
        Util::ColorI color = Util::ColorsI::TransparentGray;
        // TCS: Yellow
        if (g_vehData.mWheelsTcs[i]) {
            color = Util::ColorI{ 255, 255, 0, 127 };
        }
        // ESP: Blue
        if (g_vehData.mWheelsEspO[i] || g_vehData.mWheelsEspU[i]) {
            color = Util::ColorI{ 0, 0, 255, 127 };
        }
        // ABS: Red
        if (g_vehData.mWheelsAbs[i]) {
            color = Util::ColorI{ 255, 0, 0, 127 };
        }
        // Locked up: Purple
        if (g_vehData.mWheelsLockedUp[i]) {
            color = Util::ColorI{ 127, 0, 255, 127 };
        }
        // Off ground: Transparent
        if (!wheelsOnGround[i]) {
            color = Util::ColorI{ 0, 0, 0, 0 };
        }

        // The heck were tyre guys thinking mixing metric and imperial units?
        float tyreAr = 100.0f * ((wheelDims[i].TyreRadius - wheelDims[i].RimRadius) / wheelDims[i].TyreWidth);
        float rimSize = 2.0f * wheelDims[i].RimRadius * 39.3701f; // inches

        UI::ShowText3D(wheelCoords[i], {
                fmt::format("[{}] {}Powered", i, VExt::IsWheelPowered(g_playerVehicle, i) ? "~g~" : "~r~"),
                fmt::format("Speed: \t{:.3f}", wheelsSpeed[i]),
                //fmt::format("Compr: \t{:.3f}", wheelsCompr[i]),
                //fmt::format("Health: \t{:.3f}", wheelsHealt[i]),
                fmt::format("Power: \t{:.3f}", wheelsPower[i]),
                fmt::format("Brake: \t{:.3f}", wheelsBrake[i]),
                fmt::format("Tyre: {:.0f}/{:.0f}R{:.0f}", wheelDims[i].TyreWidth * 1000.0f, tyreAr, rimSize),
                fmt::format("{}ABS~w~ | {}TCS~w~ | {}ESC{}",
                    g_vehData.mWheelsAbs[i] ? "~r~" : "",
                    g_vehData.mWheelsTcs[i] ? "~r~" : "",
                    g_vehData.mWheelsEspO[i] || g_vehData.mWheelsEspU[i] ? "~r~" : "",
                    g_vehData.mWheelsEspO[i] && g_vehData.mWheelsEspU[i] ? "_O+U"
                        : g_vehData.mWheelsEspO[i] ? "_O"
                        : g_vehData.mWheelsEspU[i] ? "_U" : ""
                    )
            },
            color);
        GRAPHICS::DRAW_LINE(wheelCoords[i].x, wheelCoords[i].y, wheelCoords[i].z,
            wheelCoords[i].x, wheelCoords[i].y, wheelCoords[i].z + 1.0f + 2.5f * wheelsCompr[i], 255, 0, 0, 255);
    }
}

void drawLSDInfo() {
    auto lsdData = DrivingAssists::GetLSD();
    std::string fddcol;
    if (lsdData.FDD > 0.1f) { fddcol = "~r~"; }
    if (lsdData.FDD < -0.1f) { fddcol = "~b~"; }

    std::string rddcol;
    if (lsdData.RDD > 0.1f) { rddcol = "~r~"; }
    if (lsdData.RDD < -0.1f) { rddcol = "~b~"; }
    UI::ShowText(0.60f, 0.000f, 0.25f, fmt::format("LF LSD: {:.2f}", lsdData.BrakeLF));
    UI::ShowText(0.65f, 0.000f, 0.25f, fmt::format("RF LSD: {:.2f}", lsdData.BrakeRF));
    UI::ShowText(0.70f, 0.000f, 0.25f, fmt::format("{}L-R: {:.2f}", fddcol, lsdData.FDD));
    UI::ShowText(0.60f, 0.025f, 0.25f, fmt::format("LR LSD: {:.2f}", lsdData.BrakeLR));
    UI::ShowText(0.65f, 0.025f, 0.25f, fmt::format("RR LSD: {:.2f}", lsdData.BrakeRR));
    UI::ShowText(0.70f, 0.025f, 0.25f, fmt::format("{}L-R: {:.2f}", rddcol, lsdData.RDD));
    UI::ShowText(0.60f, 0.050f, 0.25f, fmt::format(
        "{}LSD: {}", lsdData.Use ? "~g~" : "~r~", lsdData.Use ? "Active" : "Idle/Off"));
}

void drawMouseSteering() {
    GRAPHICS::DRAW_RECT(
        g_settings().HUD.MouseSteering.XPos,
        g_settings().HUD.MouseSteering.YPos,
        g_settings().HUD.MouseSteering.XSz,
        g_settings().HUD.MouseSteering.YSz,
        g_settings().HUD.MouseSteering.BgR,
        g_settings().HUD.MouseSteering.BgG,
        g_settings().HUD.MouseSteering.BgB,
        g_settings().HUD.MouseSteering.BgA, 0);
    GRAPHICS::DRAW_RECT(
        g_settings().HUD.MouseSteering.XPos + CustomSteering::GetMouseX() * g_settings().HUD.MouseSteering.XSz * 0.5f,
        g_settings().HUD.MouseSteering.YPos,
        g_settings().HUD.MouseSteering.MarkerXSz,
        g_settings().HUD.MouseSteering.YSz,
        g_settings().HUD.MouseSteering.FgR,
        g_settings().HUD.MouseSteering.FgG,
        g_settings().HUD.MouseSteering.FgB,
        g_settings().HUD.MouseSteering.FgA, 0);
}