#include "script.h"

#include <inc/natives.h>

#include <menu.h>

#include "Memory/VehicleExtensions.hpp"

#include "Util/MathExt.h"
#include "Util/UIUtils.h"

#include "Input/CarControls.hpp"
#include "VehicleData.hpp"
#include "ScriptSettings.hpp"
#include "Util/StringFormat.h"

extern NativeMenu::Menu menu;
extern ScriptSettings settings;
extern std::string settingsGeneralFile;
extern std::string settingsWheelFile;
extern std::string settingsMenuFile;

extern CarControls carControls;
extern ScriptSettings settings;

extern int prevNotification;
extern int speedoIndex;
extern int textureWheelId;

extern VehicleInfo vehInfo;
extern EngineValues engVal;
extern VehicleDerivatives vehDerivs;
extern VehicleMiscStates miscStates;
extern VehicleGearboxStates gearStates;
extern WheelPatchStates wheelPatchStates;

extern VehicleExtensions ext;
extern Vehicle vehicle;

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
        settings.RPMIndicatorBackgroundR,
        settings.RPMIndicatorBackgroundG,
        settings.RPMIndicatorBackgroundB,
        settings.RPMIndicatorBackgroundA
    };

    Color foreground = {
        settings.RPMIndicatorForegroundR,
        settings.RPMIndicatorForegroundG,
        settings.RPMIndicatorForegroundB,
        settings.RPMIndicatorForegroundA
    };

    Color rpmcolor = foreground;
    if (engVal.RPM > settings.RPMIndicatorRedline) {
        Color redline = {
            settings.RPMIndicatorRedlineR,
            settings.RPMIndicatorRedlineG,
            settings.RPMIndicatorRedlineB,
            settings.RPMIndicatorRedlineA
        };
        rpmcolor = redline;
    }
    float ratio = ext.GetGearRatios(vehicle)[ext.GetGearCurr(vehicle)];
    float minUpshift = ext.GetInitialDriveMaxFlatVel(vehicle);
    float maxUpshift = ext.GetDriveMaxFlatVel(vehicle);
    if (engVal.RPM > map(minUpshift / ratio, 0.0f, maxUpshift / ratio, 0.0f, 1.0f)) {
        Color rpmlimiter = {
            settings.RPMIndicatorRevlimitR,
            settings.RPMIndicatorRevlimitG,
            settings.RPMIndicatorRevlimitB,
            settings.RPMIndicatorRevlimitA
        };
        rpmcolor = rpmlimiter;
    }
    drawRPMIndicator(
        settings.RPMIndicatorXpos,
        settings.RPMIndicatorYpos,
        settings.RPMIndicatorWidth,
        settings.RPMIndicatorHeight,
        rpmcolor,
        background,
        engVal.RPM
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

    std::string str = fmt("%03d", static_cast<int>(speed));

    if (showUnit) 
        str = fmt("%s %s", str, units);

    return str;
}

void drawSpeedoMeter() {
    float dashms = vehInfo.HasSpeedo ? ext.GetDashSpeed(vehicle) : abs(ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y);

    showText(settings.SpeedoXpos, settings.SpeedoYpos, settings.SpeedoSize,
        formatSpeedo(settings.Speedo, dashms, settings.SpeedoShowUnit, settings.HUDFont),
        settings.HUDFont);
}

void drawShiftModeIndicator() {
    std::string shiftModeText;
    auto color = solidWhite;
    switch (settings.ShiftMode) {
    case Sequential: shiftModeText = "S";
        break;
    case HPattern: shiftModeText = "H";
        break;
    case Automatic: shiftModeText = "A";
        break;
    default: shiftModeText = "";
        break;
    }
    if (!settings.EnableManual) {
        shiftModeText = "A";
        color = { 0, 126, 232, 255 };
    }
    showText(settings.ShiftModeXpos, settings.ShiftModeYpos, settings.ShiftModeSize, shiftModeText, settings.HUDFont, color, true);
}

void drawGearIndicator() {
    std::string gear = std::to_string(ext.GetGearCurr(vehicle));
    if (gearStates.FakeNeutral && settings.EnableManual) {
        gear = "N";
    }
    else if (ext.GetGearCurr(vehicle) == 0) {
        gear = "R";
    }
    Color c;
    if (ext.GetGearCurr(vehicle) == ext.GetTopGear(vehicle)) {
        c.R = settings.GearTopColorR;
        c.G = settings.GearTopColorG;
        c.B = settings.GearTopColorB;
        c.A = 255;
    }
    else {
        c = solidWhite;
    }
    showText(settings.GearXpos, settings.GearYpos, settings.GearSize, gear, settings.HUDFont, c, true);
}

void drawHUD() {
    if (settings.GearIndicator) {
        drawGearIndicator();
    }
    if (settings.ShiftModeIndicator) {
        drawShiftModeIndicator();
    }
    if (settings.Speedo == "kph" ||
        settings.Speedo == "mph" ||
        settings.Speedo == "ms") {
        drawSpeedoMeter();
    }
    if (settings.RPMIndicator) {
        drawRPMIndicator();
    }
}

void drawDebugInfo() {
    if (!menu.IsThisOpen()) {
        showText(0.01, 0.275, 0.3, fmt("Mod Enabled:\t\t%s" , settings.EnableManual ? "Yes" : "No"));
        showText(0.01, 0.300, 0.3, fmt("RPM:\t\t\t%.3f", engVal.RPM));
        showText(0.01, 0.325, 0.3, fmt("Current Gear:\t\t%d", ext.GetGearCurr(vehicle)));
        showText(0.01, 0.350, 0.3, fmt("Next Gear:\t\t%d", ext.GetGearNext(vehicle)));
        showText(0.01, 0.375, 0.3, fmt("Clutch:\t\t\t%.3f", ext.GetClutch(vehicle)));
        showText(0.01, 0.400, 0.3, fmt("Throttle:\t\t\t%.3f", ext.GetThrottle(vehicle)));
        showText(0.01, 0.425, 0.3, fmt("Turbo:\t\t\t%.3f", ext.GetTurbo(vehicle)));
        showText(0.01, 0.450, 0.3, fmt("VehAddress:\t\t0x%X", reinterpret_cast<uint64_t>(ext.GetAddress(vehicle))));
        showText(0.01, 0.475, 0.3, fmt("Speedo Present:\t%s", vehInfo.HasSpeedo ? "Yes" : "No"));
        showText(0.01, 0.500, 0.3, fmt("Drive Bias:\t\t%.3f", ext.GetDriveBiasFront(vehicle)));
    }

    showText(0.85, 0.050, 0.4, fmt("Throttle:\t%.3f", carControls.ThrottleVal) , 4);
    showText(0.85, 0.075, 0.4, fmt("Brake:\t\t%.3f" , carControls.BrakeVal)    , 4);
    showText(0.85, 0.100, 0.4, fmt("Clutch:\t\t%.3f", carControls.ClutchVal)   , 4);
    showText(0.85, 0.125, 0.4, fmt("Handb:\t\t%.3f" , carControls.HandbrakeVal), 4);

    if (settings.EnableWheel)
        showText(0.85, 0.150, 0.4, fmt("Wheel %s present", carControls.WheelAvailable() ? "" : " not"), 4);
    

    if (settings.DisplayGearingInfo) {
        if (ext.GetGearCurr(vehicle) < ext.GetGearNext(vehicle)) {
            gearStates.UpshiftSpeedsGame[ext.GetGearCurr(vehicle)] = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y;
        }

        auto ratios = ext.GetGearRatios(vehicle);
        float DriveMaxFlatVel = ext.GetDriveMaxFlatVel(vehicle);
        float InitialDriveMaxFlatVel = ext.GetInitialDriveMaxFlatVel(vehicle);

        int i = 0;
        showText(0.10f, 0.05f, 0.35f, "Ratios");
        for (auto ratio : ratios) {
            showText(0.10f, 0.10f + 0.025f * i, 0.35f, "G" + std::to_string(i) + ": " + std::to_string(ratio));
            i++;
        }

        i = 0;
        showText(0.25f, 0.05f, 0.35f, "InitialDriveMaxFlatVel");
        for (auto ratio : ratios) {
            float maxSpeed = InitialDriveMaxFlatVel / ratio;
            showText(0.25f, 0.10f + 0.025f * i, 0.35f, "G" + std::to_string(i) + ": " + std::to_string(maxSpeed));
            i++;
        }

        i = 0;
        showText(0.40f, 0.05f, 0.35f, "DriveMaxFlatVel");
        for (auto ratio : ratios) {
            float maxSpeed = DriveMaxFlatVel / ratio;
            showText(0.40f, 0.10f + 0.025f * i, 0.35f, "G" + std::to_string(i) + ": " + std::to_string(maxSpeed));
            i++;
        }

        i = 0;
        showText(0.55f, 0.05f, 0.35f, "Actual (Game)");
        for (const auto& speed : gearStates.UpshiftSpeedsGame) {
            showText(0.55f, 0.10f + 0.025f * i, 0.35f, "G" + std::to_string(i) + ": " + std::to_string(speed));
            i++;
        }

        i = 0;
        showText(0.70f, 0.05f, 0.35f, "Actual (Mod)");
        for (const auto& speed : gearStates.UpshiftSpeedsMod) {
            showText(0.70f, 0.10f + 0.025f * i, 0.35f, "G" + std::to_string(i) + ": " + std::to_string(speed));
            i++;
        }
    }
}

void drawInputWheelInfo() {
    // Steering Wheel
    float rotation = settings.SteerAngleMax * (carControls.SteerVal - 0.5f);
    if (carControls.PrevInput != CarControls::Wheel) rotation = 90.0f * -ext.GetSteeringInputAngle(vehicle);

    drawTexture(textureWheelId, 0, -9998, 100,
        settings.SteeringWheelTextureSz, settings.SteeringWheelTextureSz,
        0.5f, 0.5f, // center of texture
        settings.SteeringWheelTextureX, settings.SteeringWheelTextureY,
        rotation / 360.0f, GRAPHICS::_GET_ASPECT_RATIO(FALSE), 1.0f, 1.0f, 1.0f, 1.0f);

    // Pedals
    float barWidth = settings.PedalInfoW / 3.0f;

    float barYBase = (settings.PedalInfoY + settings.PedalInfoH * 0.5f);

    GRAPHICS::DRAW_RECT(settings.PedalInfoX, settings.PedalInfoY, 3.0f * barWidth + settings.PedalInfoPadX, settings.PedalInfoH + settings.PedalInfoPadY, 0, 0, 0, 92);
    GRAPHICS::DRAW_RECT(settings.PedalInfoX - 1.0f*barWidth, barYBase - carControls.ThrottleVal*settings.PedalInfoH*0.5f, barWidth, carControls.ThrottleVal*settings.PedalInfoH, 0, 255, 0, 255);
    GRAPHICS::DRAW_RECT(settings.PedalInfoX + 0.0f*barWidth, barYBase - carControls.BrakeVal*settings.PedalInfoH*0.5f, barWidth, carControls.BrakeVal*settings.PedalInfoH, 255, 0, 0, 255);
    GRAPHICS::DRAW_RECT(settings.PedalInfoX + 1.0f*barWidth, barYBase - carControls.ClutchVal*settings.PedalInfoH*0.5f, barWidth, carControls.ClutchVal*settings.PedalInfoH, 0, 0, 255, 255);
}

std::vector<bool> getWheelLockups(Vehicle handle) {
    std::vector<bool> lockups;
    float velocity = ENTITY::GET_ENTITY_VELOCITY(vehicle).y;
    auto wheelsSpeed = ext.GetWheelRotationSpeeds(vehicle);
    for (auto wheelSpeed : wheelsSpeed) {
        if (abs(velocity) > 0.01f && wheelSpeed == 0.0f)
            lockups.push_back(true);
        else
            lockups.push_back(false);
    }
    return lockups;
}

void drawVehicleWheelInfo() {
    auto numWheels = ext.GetNumWheels(vehicle);
    auto wheelsSpeed = ext.GetTyreSpeeds(vehicle);
    auto wheelsCompr = ext.GetWheelCompressions(vehicle);
    auto wheelsHealt = ext.GetWheelHealths(vehicle);
    auto wheelsContactCoords = ext.GetWheelLastContactCoords(vehicle);
    auto wheelsOnGround = ext.GetWheelsOnGround(vehicle);
    auto wheelCoords = ext.GetWheelCoords(vehicle, ENTITY::GET_ENTITY_COORDS(vehicle, true), ENTITY::GET_ENTITY_ROTATION(vehicle, 0), ENTITY::GET_ENTITY_FORWARD_VECTOR(vehicle));
    auto wheelLockups = getWheelLockups(vehicle);
    auto wheelsPower = ext.GetWheelPower(vehicle);
    auto wheelsBrake = ext.GetWheelBrakePressure(vehicle);
    for (int i = 0; i < numWheels; i++) {
        float wheelCompr = wheelsCompr[i];
        Color c = wheelLockups[i] ? solidOrange : transparentGray;
        c = wheelsOnGround[i] ? c : solidRed;
        showDebugInfo3D(wheelCoords[i], {
            fmt("Index: \t%d", i),
            fmt("Power: \t%s", ext.IsWheelPowered(vehicle, i) ? "Yes" : "No"),
            fmt("Speed: \t%.3f", wheelsSpeed[i]),
            fmt("Compr: \t%.3f", wheelCompr),
            fmt("Health: \t%.3f", wheelsHealt[i]),
            fmt("Power: \t%.3f", wheelsPower[i]),
            fmt("Brake: \t%.3f",wheelsBrake[i])}, c);
        GRAPHICS::DRAW_LINE(wheelCoords[i].x, wheelCoords[i].y, wheelCoords[i].z,
            wheelCoords[i].x, wheelCoords[i].y, wheelCoords[i].z + 1.0f + 2.5f * wheelCompr, 255, 0, 0, 255);
    }
}
