#include "script.h"
#include <fmt/format.h>
#include <inc/natives.h>

#include <menu.h>

#include "Memory/VehicleExtensions.hpp"

#include "Util/MathExt.h"
#include "Util/UIUtils.h"

#include "Input/CarControls.hpp"
#include "VehicleData.hpp"
#include "ScriptSettings.hpp"
#include "Memory/Offsets.hpp"

extern NativeMenu::Menu menu;
extern ScriptSettings settings;
extern CarControls carControls;
extern int textureWheelId;

extern VehicleGearboxStates gearStates;
extern VehicleExtensions ext;
extern Vehicle playerVehicle;
extern VehicleData vehData;

///////////////////////////////////////////////////////////////////////////////
//                           Display elements
///////////////////////////////////////////////////////////////////////////////
void drawRPMIndicator(float x, float y, float width, float height, Color fg, Color bg, float rpm) {
    float bgpaddingx = 0.00f;
    float bgpaddingy = 0.01f;
    // background
    GRAPHICS::DRAW_RECT(x, y, width + bgpaddingx, height + bgpaddingy, bg.R, bg.G, bg.B, bg.A);

    // rpm bar
    GRAPHICS::DRAW_RECT(x - width*0.5f + rpm*width*0.5f, y, width*rpm, height, fg.R, fg.G, fg.B, fg.A);
}

void drawRPMIndicator() {
    Color background = {
        settings.HUD.RPMBar.BgR,
        settings.HUD.RPMBar.BgG,
        settings.HUD.RPMBar.BgB,
        settings.HUD.RPMBar.BgA
    };

    Color foreground = {
        settings.HUD.RPMBar.FgR,
        settings.HUD.RPMBar.FgG,
        settings.HUD.RPMBar.FgB,
        settings.HUD.RPMBar.FgA
    };

    Color rpmcolor = foreground;
    if (vehData.mRPM > settings.HUD.RPMBar.Redline) {
        Color redline = {
            settings.HUD.RPMBar.RedlineR,
            settings.HUD.RPMBar.RedlineG,
            settings.HUD.RPMBar.RedlineB,
            settings.HUD.RPMBar.RedlineA
        };
        rpmcolor = redline;
    }
    float ratio = ext.GetGearRatios(playerVehicle)[ext.GetGearCurr(playerVehicle)];
    float minUpshift = ext.GetInitialDriveMaxFlatVel(playerVehicle);
    float maxUpshift = ext.GetDriveMaxFlatVel(playerVehicle);
    if (vehData.mRPM > map(minUpshift / ratio, 0.0f, maxUpshift / ratio, 0.0f, 1.0f)) {
        Color rpmlimiter = {
            settings.HUD.RPMBar.RevLimitR,
            settings.HUD.RPMBar.RevLimitG,
            settings.HUD.RPMBar.RevLimitB,
            settings.HUD.RPMBar.RevLimitA
        };
        rpmcolor = rpmlimiter;
    }
    drawRPMIndicator(
        settings.HUD.RPMBar.XPos,
        settings.HUD.RPMBar.YPos,
        settings.HUD.RPMBar.XSz,
        settings.HUD.RPMBar.YSz,
        rpmcolor,
        background,
        vehData.mRPM
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
    float dashms = vehData.mHasSpeedo ? ext.GetDashSpeed(playerVehicle) : abs(ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y);

    showText(settings.HUD.Speedo.XPos, settings.HUD.Speedo.YPos, settings.HUD.Speedo.Size,
        formatSpeedo(settings.HUD.Speedo.Speedo, dashms, settings.HUD.Speedo.ShowUnit, settings.HUD.HUDFont),
        settings.HUD.HUDFont);
}

void drawShiftModeIndicator() {
    std::string shiftModeText;
    auto color = solidWhite;
    switch (settings.MTOptions.ShiftMode) {
        case EShiftMode::Sequential:    shiftModeText = "S"; break;
        case EShiftMode::HPattern:      shiftModeText = "H"; break;
        case EShiftMode::Automatic:     shiftModeText = "A"; break;
        default: shiftModeText = ""; break;
    }
    if (!settings.MTOptions.Enable) {
        shiftModeText = "A";
        color = { 0, 126, 232, 255 };
    }
    showText(settings.HUD.ShiftMode.XPos, settings.HUD.ShiftMode.YPos, settings.HUD.ShiftMode.Size, shiftModeText, settings.HUD.HUDFont, color, true);
}

void drawGearIndicator() {
    std::string gear = std::to_string(ext.GetGearCurr(playerVehicle));
    if (ext.GetHandbrake(playerVehicle)) {
        gear = "P";
    }
    else if (gearStates.FakeNeutral && settings.MTOptions.Enable) {
        gear = "N";
    }
    else if (ext.GetGearCurr(playerVehicle) == 0) {
        gear = "R";
    }
    Color c;
    if (ext.GetGearCurr(playerVehicle) == ext.GetTopGear(playerVehicle)) {
        c.R = settings.HUD.Gear.TopColorR;
        c.G = settings.HUD.Gear.TopColorG;
        c.B = settings.HUD.Gear.TopColorB;
        c.A = 255;
    }
    else {
        c = solidWhite;
    }
    showText(settings.HUD.Gear.XPos, settings.HUD.Gear.YPos, settings.HUD.Gear.Size, gear, settings.HUD.HUDFont, c, true);
}

void drawHUD() {
    if (settings.HUD.Gear.Enable) {
        drawGearIndicator();
    }
    if (settings.HUD.ShiftMode.Enable) {
        drawShiftModeIndicator();
    }
    if (settings.HUD.Speedo.Speedo == "kph" ||
        settings.HUD.Speedo.Speedo == "mph" ||
        settings.HUD.Speedo.Speedo == "ms") {
        drawSpeedoMeter();
    }
    if (settings.HUD.RPMBar.Enable) {
        drawRPMIndicator();
    }
}

void drawDebugInfo() {
    if (!menu.IsThisOpen()) {
        showText(0.01, 0.250, 0.3, fmt::format("Address: 0x{:X}", reinterpret_cast<uintptr_t>(ext.GetAddress(playerVehicle))));
        showText(0.01, 0.275, 0.3, fmt::format("Mod Enabled:\t\t{}" , settings.MTOptions.Enable));
        showText(0.01, 0.300, 0.3, fmt::format("RPM:\t\t\t{:.3f}", vehData.mRPM));
        showText(0.01, 0.325, 0.3, fmt::format("Current Gear:\t\t{}", ext.GetGearCurr(playerVehicle)));
        showText(0.01, 0.350, 0.3, fmt::format("Next Gear:\t\t{}", ext.GetGearNext(playerVehicle)));
        showText(0.01, 0.375, 0.3, fmt::format("Clutch:\t\t\t{:.2f}", ext.GetClutch(playerVehicle)));
        showText(0.01, 0.400, 0.3, fmt::format("Throttle:\t\t\t{:.2f}", ext.GetThrottle(playerVehicle)));
        showText(0.01, 0.425, 0.3, fmt::format("Turbo:\t\t\t{:.2f}", ext.GetTurbo(playerVehicle)));
        showText(0.01, 0.450, 0.3, fmt::format("{}Speedo", vehData.mHasSpeedo ? "~g~" : "~r~"));
        showText(0.01, 0.475, 0.3, fmt::format("{}E {}CVT -> {}Clutch",
            vehData.mIsElectric ? "~g~" : "~r~", vehData.mIsCVT ? "~g~" : "~r~",
            vehData.mHasClutch ? "~g~" : "~r~"));
        showText(0.01, 0.500, 0.3, fmt::format("{}ABS",
            vehData.mHasABS ? "~g~" : "~r~"));

        showText(0.01, 0.550, 0.3, fmt::format("{}Shifting", gearStates.Shifting ? "~g~" : "~r~"));
        showText(0.01, 0.575, 0.3, fmt::format("Clutch: {}" ,gearStates.ClutchVal));
        showText(0.01, 0.600, 0.3, fmt::format("Lock: {}" ,gearStates.LockGear));
        showText(0.01, 0.625, 0.3, fmt::format("Next: {}" ,gearStates.NextGear));
        showText(0.01, 0.650, 0.3, fmt::format("{}Load/upReq: {:.3f}\t/{:.3f}",
            gearStates.Shifting ? "~c~" : "", gearStates.EngineLoad, gearStates.UpshiftLoad));
        showText(0.01, 0.675, 0.3, fmt::format("{}Load/dnReq: {:.3f}\t/{:.3f}",
            gearStates.Shifting ? "~c~" : "", gearStates.EngineLoad, gearStates.DownshiftLoad));
    }

    showText(0.85, 0.050, 0.4, fmt::format("Throttle:\t{:.3f}", carControls.ThrottleVal) , 4);
    showText(0.85, 0.075, 0.4, fmt::format("Brake:\t\t{:.3f}" , carControls.BrakeVal)    , 4);
    showText(0.85, 0.100, 0.4, fmt::format("Clutch:\t\t{:.3f}", carControls.ClutchVal)   , 4);
    showText(0.85, 0.125, 0.4, fmt::format("Handb:\t\t{:.3f}" , carControls.HandbrakeVal), 4);

    if (settings.Wheel.Options.Enable)
        showText(0.85, 0.150, 0.4, fmt::format("Wheel {} present", carControls.WheelAvailable() ? "" : " not"), 4);
    

    if (settings.Debug.DisplayGearingInfo) {
        auto ratios = ext.GetGearRatios(playerVehicle);
        float DriveMaxFlatVel = ext.GetDriveMaxFlatVel(playerVehicle);

        int i = 0;
        showText(0.30f, 0.05f, 0.35f, "Ratios");
        for (auto ratio : ratios) {
            showText(0.30f, 0.10f + 0.025f * i, 0.35f, fmt::format("G{}: {:.3f}", i, ratio));
            i++;
        }

        i = 0;
        showText(0.45f, 0.05f, 0.35f, "DriveMaxFlatVel");
        for (auto ratio : ratios) {
            float maxSpeed = DriveMaxFlatVel / ratio;
            showText(0.45f, 0.10f + 0.025f * i, 0.35f, fmt::format("G{}: {:.3f}", i, maxSpeed));
            i++;
        }

        float rateUp = *reinterpret_cast<float*>(ext.GetHandlingPtr(playerVehicle) + hOffsets.fClutchChangeRateScaleUpShift);
        float rateDown = *reinterpret_cast<float*>(ext.GetHandlingPtr(playerVehicle) + hOffsets.fClutchChangeRateScaleDownShift);
        float upshiftDuration = 1.0f / (rateUp * settings.ShiftOptions.ClutchRateMult);
        float downshiftDuration = 1.0f / (rateDown * settings.ShiftOptions.ClutchRateMult);

        showText(0.60f, 0.050f, 0.35f, fmt::format("ClutchRate Up: {:.3f}", rateUp));
        showText(0.60f, 0.075f, 0.35f, fmt::format("ClutchRate Dn: {:.3f}", rateDown));
        showText(0.60f, 0.100f, 0.35f, fmt::format("Duration Up: {:.3f}", upshiftDuration));
        showText(0.60f, 0.125f, 0.35f, fmt::format("Duration Dn: {:.3f}", downshiftDuration));
        showText(0.60f, 0.150f, 0.35f, fmt::format("Shift timeout (dn): {:.3f}", downshiftDuration * settings.AutoParams.DownshiftTimeoutMult));
    }
}

void drawInputWheelInfo() {
    // Steering Wheel
    float rotation = settings.Wheel.Steering.AngleMax * (carControls.SteerVal - 0.5f);
    if (carControls.PrevInput != CarControls::Wheel) rotation = 90.0f * -ext.GetSteeringInputAngle(playerVehicle);

    drawTexture(textureWheelId, 0, -9998, 100,
        settings.HUD.Wheel.ImgSize, settings.HUD.Wheel.ImgSize,
        0.5f, 0.5f, // center of texture
        settings.HUD.Wheel.ImgXPos, settings.HUD.Wheel.ImgYPos,
        rotation / 360.0f, GRAPHICS::_GET_ASPECT_RATIO(FALSE), 1.0f, 1.0f, 1.0f, 1.0f);

    // Pedals
    float barWidth = settings.HUD.Wheel.PedalXSz / 3.0f;

    float barYBase = (settings.HUD.Wheel.PedalYPos + settings.HUD.Wheel.PedalYSz * 0.5f);

    GRAPHICS::DRAW_RECT(settings.HUD.Wheel.PedalXPos, settings.HUD.Wheel.PedalYPos, 3.0f * barWidth + settings.HUD.Wheel.PedalXPad, settings.HUD.Wheel.PedalYSz + settings.HUD.Wheel.PedalYPad, 
        0, 0, 0, settings.HUD.Wheel.PedalBgA);
    GRAPHICS::DRAW_RECT(settings.HUD.Wheel.PedalXPos + 1.0f * barWidth, barYBase - carControls.ThrottleVal * settings.HUD.Wheel.PedalYSz * 0.5f,
        barWidth, carControls.ThrottleVal * settings.HUD.Wheel.PedalYSz, 
        settings.HUD.Wheel.PedalThrottleR, settings.HUD.Wheel.PedalThrottleG, settings.HUD.Wheel.PedalThrottleB, settings.HUD.Wheel.PedalThrottleA);
    GRAPHICS::DRAW_RECT(settings.HUD.Wheel.PedalXPos + 0.0f * barWidth, barYBase - carControls.BrakeVal * settings.HUD.Wheel.PedalYSz * 0.5f,
        barWidth, carControls.BrakeVal * settings.HUD.Wheel.PedalYSz,
        settings.HUD.Wheel.PedalBrakeR, settings.HUD.Wheel.PedalBrakeG, settings.HUD.Wheel.PedalBrakeB, settings.HUD.Wheel.PedalBrakeA);
    GRAPHICS::DRAW_RECT(settings.HUD.Wheel.PedalXPos - 1.0f * barWidth, barYBase - carControls.ClutchVal * settings.HUD.Wheel.PedalYSz * 0.5f,
        barWidth, carControls.ClutchVal * settings.HUD.Wheel.PedalYSz,
        settings.HUD.Wheel.PedalClutchR, settings.HUD.Wheel.PedalClutchG, settings.HUD.Wheel.PedalClutchB, settings.HUD.Wheel.PedalClutchA);
}

void drawVehicleWheelInfo() {
    auto numWheels = ext.GetNumWheels(playerVehicle);
    auto wheelsSpeed = ext.GetTyreSpeeds(playerVehicle);
    auto wheelsCompr = ext.GetWheelCompressions(playerVehicle);
    auto wheelsHealt = ext.GetWheelHealths(playerVehicle);
    auto wheelsContactCoords = ext.GetWheelLastContactCoords(playerVehicle);
    auto wheelsOnGround = ext.GetWheelsOnGround(playerVehicle);
    auto wheelCoords = ext.GetWheelCoords(playerVehicle, ENTITY::GET_ENTITY_COORDS(playerVehicle, true), ENTITY::GET_ENTITY_ROTATION(playerVehicle, 0), ENTITY::GET_ENTITY_FORWARD_VECTOR(playerVehicle));
    auto wheelsPower = ext.GetWheelPower(playerVehicle);
    auto wheelsBrake = ext.GetWheelBrakePressure(playerVehicle);
    for (int i = 0; i < numWheels; i++) {
        Color color = transparentGray;
        if (vehData.mWheelsTcs[i]) {
            color = Color{ 255, 255, 0, 127 };
        }
        if (vehData.mWheelsAbs[i]) {
            color = Color{ 255, 0, 0, 127 };
        }
        if (vehData.mWheelsLockedUp[i]) {
            color = Color{ 127, 0, 255, 127 };
        }
        if (!wheelsOnGround[i]) {
            color = Color{ 0, 0, 0, 0 };
        }
        showDebugInfo3D(wheelCoords[i], {
            fmt::format("Index: \t{}", i),
            fmt::format("{}Powered", ext.IsWheelPowered(playerVehicle, i) ? "~g~" : "~r~"),
            fmt::format("Speed: \t{:.3f}", wheelsSpeed[i]),
            fmt::format("Compr: \t{:.3f}", wheelsCompr[i]),
            fmt::format("Health: \t{:.3f}", wheelsHealt[i]),
            fmt::format("Power: \t{:.3f}", wheelsPower[i]),
            fmt::format("Brake: \t{:.3f}", wheelsBrake[i])}, color);
        GRAPHICS::DRAW_LINE(wheelCoords[i].x, wheelCoords[i].y, wheelCoords[i].z,
            wheelCoords[i].x, wheelCoords[i].y, wheelCoords[i].z + 1.0f + 2.5f * wheelsCompr[i], 255, 0, 0, 255);
    }
}
