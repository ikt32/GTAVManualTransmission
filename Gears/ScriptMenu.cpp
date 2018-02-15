#define NOMINMAX
#include <string>

#include "inc/main.h"
#include "script.h"
#include "menu.h"

#include "ScriptSettings.hpp"
#include "Input/ScriptControls.hpp"
#include "Input/keyboard.h"
#include "Util/Util.hpp"
#include "inc/natives.h"
#include "menukeyboard.h"

extern ScriptSettings settings;
extern std::string settingsGeneralFile;
extern std::string settingsWheelFile;
extern std::string settingsMenuFile;

extern NativeMenu::Menu menu;

extern ScriptControls controls;
extern ScriptSettings settings;

extern int prevNotification;
extern int speedoIndex;
extern int textureWheelId;

const std::string escapeKey = "BACKSPACE";
const std::string skipKey = "RIGHT";

// FontName, fontID
std::vector<std::string> fonts{
    { "Chalet London" },
    { "Sign Painter" },
    { "Slab Serif" },
    { "Chalet Cologne" },
    { "Pricedown" },
};

std::vector<int> fontIDs{
    0,
    1,
    2,
    4,
    7
};

std::vector<std::string> buttonConfTags{
    { "TOGGLE_MOD" },
    { "CHANGE_SHIFTMODE" },
    { "CLUTCH_BUTTON" },
    { "SHIFT_UP" },
    { "SHIFT_DOWN" },
    { "ENGINE" },
    { "HANDBRAKE" },
    { "HORN" },
    { "LIGHTS" },
    { "LOOK_BACK" },
    { "LOOK_LEFT" },
    { "LOOK_RIGHT" },
    { "CHANGE_CAMERA" },
    { "RADIO_NEXT" },
    { "RADIO_PREVIOUS" },
    { "INDICATOR_LEFT" },
    { "INDICATOR_RIGHT" },
    { "INDICATOR_HAZARD" },
};

std::vector<std::string> keyboardConfTags{
    { "Toggle" },
    { "ToggleH" },
    { "ShiftUp" },
    { "ShiftDown" },
    { "Clutch" },
    { "Engine" },
    { "Throttle" },
    { "Brake" },
    { "HR" },
    { "H1" },
    { "H2" },
    { "H3" },
    { "H4" },
    { "H5" },
    { "H6" },
    { "H7" },
    { "HN" },
};

std::vector<std::string> controllerConfTags{
    { "Toggle" },
    { "ToggleShift" },
    { "ShiftUp" },
    { "ShiftDown" },
    { "Clutch" },
    { "Engine" },
    { "Throttle" },
    { "Brake" }
};

std::vector<std::string> keyboardConfTagsDetail{
    { "Toggle mod on/off" },
    { "Switch shift mode" },
    { "Shift up" },
    { "Shift down" },
    { "Hold for clutch" },
    { "Toggle engine on/off" },
    { "Key used for throttle" },
    { "Key used for brake" },
    { "H-pattern gear R press" },
    { "H-pattern gear 1 press" },
    { "H-pattern gear 2 press" },
    { "H-pattern gear 3 press" },
    { "H-pattern gear 4 press" },
    { "H-pattern gear 5 press" },
    { "H-pattern gear 6 press" },
    { "H-pattern gear 7 press" },
    { "H-pattern Neutral" },
};

std::vector<std::string> controllerConfTagDetail{
    { "Toggle mod usage: hold" },
    { "Toggle shift usage: hold" },
    { "Shift up usage: press" },
    { "Shift down usage: press" },
    { "Clutch usage: axis or button" },
    { "Engine usage: press" },
    { "Throttle: axis or button" },
    { "Brake: axis or button" }
};

std::vector<std::string> speedoTypes = {
    "off",
    "kph",
    "mph",
    "ms"
};

std::vector<int> blockableControls = {
    -1,
    ControlVehicleAim,
    ControlVehicleHandbrake,
    ControlVehicleAttack,
    ControlVehicleDuck,
    ControlVehicleSelectNextWeapon,
    ControlVehicleCinCam,
};

std::vector<std::string> blockableControlsHelp = {
    "None",
    "Aim",
    "Handbrake",
    "Attack",
    "Duck",
    "NextWeapon",
    "CinematicCamera",
};

int getBlockableControlIndex(int control) {
    for (int i = 0; i < blockableControls.size(); i++) {
        if (control == blockableControls[i])
            return i;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//                             Menu stuff
///////////////////////////////////////////////////////////////////////////////
void menuInit() {
    menu.ReadSettings();
}

void menuClose() {
    settings.SaveGeneral();
    settings.SaveWheel(&controls);
    settings.SaveController(&controls);
}

void update_mainmenu() {
    menu.Title("Manual Transmission", 0.90f);
    menu.Subtitle(std::string("~b~") + DISPLAY_VERSION);

    for(auto device : controls.FreeDevices) {
        if (menu.Option(device.name, NativeMenu::solidRed, 
            { "~r~<b>THIS DEVICE HASN'T BEEN SET UP YET!</b>~w~",
              "Set it up or apply this option to discard the message.", 
              "Full name: " + device.name })) {
            settings.SteeringAppendDevice(device.guid, device.name);
            settings.Read(&controls);
            controls.CheckGUIDs(settings.reggdGuids);
        }
    }

    bool tempEnableRead = settings.EnableManual;

    if (menu.BoolOption("Enable manual transmission", tempEnableRead,
                        { "Enable or disable the entire mod." })) {
        toggleManual();
    }

    int shiftModeTemp = settings.ShiftMode;
    std::vector<std::string> gearboxModes = {
        "Sequential",
        "H-pattern",
        "Automatic"
    };

    menu.StringArray("Gearbox", gearboxModes, shiftModeTemp, 
                     { "Choose your gearbox! Options are Sequential, H-pattern and Automatic." });
        
    if (shiftModeTemp != settings.ShiftMode) {
        setShiftMode(shiftModeTemp);
    }

    menu.MenuOption("Mod options", "optionsmenu", { "You can tweak and fine-tune gearbox simulation here." });
    menu.MenuOption("Keyboard/Controller", "controlsmenu", { "Configure the keyboard and controller inputs." });
    menu.MenuOption("Steering Wheel", "wheelmenu", { "Set up your steering wheel." });
    menu.MenuOption("HUD Options", "hudmenu", { "Toggle and move HUD elements. Choose between imperial or metric speeds." });
    menu.MenuOption("Debug", "debugmenu", { "Show technical details and options." });
    //menu.MenuOption("Plane!", "planemenu");

    int activeIndex = 0;
    std::string activeInputName;
    switch (controls.PrevInput) {
        case ScriptControls::Keyboard:
            activeInputName = "Keyboard";
            break;
        case ScriptControls::Controller:
            activeInputName = "Controller";
            break;
        case ScriptControls::Wheel:
            activeInputName = "Wheel";
            break;
            //case ScriptControls::Stick:
            //	activeInputName = "Stick";
            //	break;
    }
    std::vector<std::string> active = { activeInputName };
    menu.StringArray("Active input", active, activeIndex, { "Active input is automatically detected and can't be changed." });
}

void update_optionsmenu() {
    menu.Title("Mod options");
    menu.Subtitle("Gearbox simulation options");

    menu.BoolOption("Engine Damage", settings.EngDamage,
                    { "Damage the engine when over-revving and when mis-shifting." });

    menu.BoolOption("Engine Stalling (H)", settings.EngStall,
                    { "Stall the engine when the wheel speed gets too low. Applies to H-pattern shift mode." });

    menu.BoolOption("Engine Stalling (S)", settings.EngStallS,
                    { "Stall the engine when the wheel speed gets too low. Applies to sequential shift mode." });

    menu.BoolOption("Clutch Shift (H)", settings.ClutchShiftingH,
                    { "Require holding the clutch to shift in H-pattern shift mode." });

    menu.BoolOption("Clutch Shift (S)", settings.ClutchShiftingS,
                    { "Require holding the clutch to shift in sequential mode." });

    menu.BoolOption("Engine Braking", settings.EngBrake,
                    { "Help the car braking by slowing down more at high RPMs." });

    menu.BoolOption("Gear/RPM Lockup", settings.EngLock,
                    { "Simulate wheel lock-up when mis-shifting to a too low gear for the RPM range." });

    menu.BoolOption("Clutch Bite", settings.ClutchCatching,
                    { "Simulate clutch biting action and clutch interaction at near-stop speeds." });

    menu.BoolOption("Default Neutral gear", settings.DefaultNeutral,
                    { "The car will be in neutral when you get in." });

    menu.MenuOption("Finetuning", "finetuneoptionsmenu",
                    { "Fine-tune the parameters above." });

    menu.MenuOption("Misc. options", "miscoptionsmenu",
                    { "Options that don't have to do with the gearbox simulation, but mainly are helpers." });
}

void update_miscoptionsmenu() {
    menu.Title("Misc. options");
    menu.Subtitle("Small helping functions");

    menu.BoolOption("Simple Bike", settings.SimpleBike, 
                    { "Disables bike engine stalling and the clutch bite simulation." });

    menu.BoolOption("Hill gravity workaround", settings.HillBrakeWorkaround, 
                    { "Gives the car a push to overcome the games' default brakes at a stop." });

    menu.BoolOption("Auto gear 1", settings.AutoGear1, 
                    { "Automatically switch to first gear when the car reaches a standstill." });

    menu.BoolOption("Auto look back", settings.AutoLookBack, 
                    { "Automatically look back whenever in reverse gear." });

    menu.BoolOption("Clutch + throttle start", settings.ThrottleStart, 
                    { "Allow to start the engine by pressing clutch and throttle, like in DiRT Rally." });
}

void update_finetuneoptionsmenu() {
    menu.Title("Fine-tuning");
    menu.Subtitle("Gearbox fine-tuning options");

    menu.FloatOption("Clutch bite threshold", settings.ClutchThreshold, 0.0f, 1.0f, 0.05f, 
                     { "How far the clutch has to be lifted to start biting. This value should be lower than \"Stalling threshold\"."});
    menu.FloatOption("Stalling threshold", settings.StallingThreshold, 0.0f, 1.0f, 0.05f, 
                     { "How far the clutch has to be lifted to start stalling. This value should be higher than \"Clutch bite point\"."});
    menu.FloatOption("RPM Damage", settings.RPMDamage, 0.0f, 10.0f, 0.05f,
                     { "Damage from redlining too long." });
    menu.IntOption("Misshift Damage", settings.MisshiftDamage, 0, 100, 5, 
                   { "Damage from being over the rev range." });
    menu.FloatOption("Engine braking threshold", settings.EngBrakeThreshold, 0.0f, 1.0f, 0.05f, 
                     { "RPM where engine braking starts being effective." });
    menu.FloatOption("Engine braking power", settings.EngBrakePower, 0.0f, 5.0f, 0.05f, 
                     { "Decrease this value if your wheels lock up when engine braking." });
}

void update_controlsmenu() {
    menu.Title("Controls");
    menu.Subtitle("Controls options");

    menu.MenuOption("Controller", "controllermenu",
                    {"Change controller control assignments."});
        
    menu.MenuOption("Keyboard", "keyboardmenu",
                    {"Change keyboard control assignments."});
        
    menu.BoolOption("Non-Xinput controller", controls.UseLegacyController,
                    { "If you needed to set up your controller in the pause menu, you should enable this." });
        
    menu.MenuOption("Non-Xinput controller", "legacycontrollermenu",
                    { "Set up the non-Xinput controller with native controls." });

    if (menu.BoolOption("Patch steering for all inputs", settings.PatchSteeringAlways,
                        { "Also patch steering reduction and automatic countersteer for keyboard and controller inputs.",
                            "Only active if this patch is also enabled for steering wheels.",
                            "PatchSteering (Wheel) is "+ std::string(settings.PatchSteering ? "enabled." : "disabled.")})) {
        settings.SaveWheel(&controls);
        initSteeringPatches();
    }

    if (menu.FloatOption("Steering reduction (kb/controller)", settings.SteeringReductionOther, 0.0f, 1.0f, 0.01f,
                         { "Reduce steering input at higher speeds.","From InfamousSabre's Custom Steering." })) {
        settings.SaveWheel(&controls);
    }

    if (menu.FloatOption("Steering multiplier (kb/controller)", settings.GameSteerMultOther, 0.1f, 2.0f, 0.01f,
                         { "Increase/decrease steering lock.","From InfamousSabre's Custom Steering." })) {
        settings.SaveWheel(&controls);
    }

    menu.BoolOption("Block H-pattern on controller", settings.BlockHShift, 
                    { "Block H-pattern mode when using controller input." });
}

void update_legacycontrollermenu() {
    menu.Title("Non-Xinput controls");
    menu.Subtitle("Non-Xinput options");

    int oldIndexUp = getBlockableControlIndex(controls.ControlNativeBlocks[static_cast<int>(ScriptControls::LegacyControlType::ShiftUp)]);
    if (menu.StringArray("Shift Up blocks", blockableControlsHelp, oldIndexUp)) {
        controls.ControlNativeBlocks[static_cast<int>(ScriptControls::LegacyControlType::ShiftUp)] = blockableControls[oldIndexUp];
    }

    int oldIndexDown = getBlockableControlIndex(controls.ControlNativeBlocks[static_cast<int>(ScriptControls::LegacyControlType::ShiftDown)]);
    if (menu.StringArray("Shift Down blocks", blockableControlsHelp, oldIndexDown)) {
        controls.ControlNativeBlocks[static_cast<int>(ScriptControls::LegacyControlType::ShiftDown)] = blockableControls[oldIndexDown];
    }

    int oldIndexClutch = getBlockableControlIndex(controls.ControlNativeBlocks[static_cast<int>(ScriptControls::LegacyControlType::Clutch)]);
    if (menu.StringArray("Clutch blocks", blockableControlsHelp, oldIndexClutch)) {
        controls.ControlNativeBlocks[static_cast<int>(ScriptControls::LegacyControlType::Clutch)] = blockableControls[oldIndexClutch];
    }

    std::vector<std::string> controllerInfo;
    controllerInfo.push_back("Press RIGHT to clear key");
    controllerInfo.push_back("Press RETURN to configure button");
    controllerInfo.push_back("");

    auto it = 0;
    for (auto confTag : controllerConfTags) {
        controllerInfo.back() = controllerConfTagDetail.at(it);
        controllerInfo.push_back("Assigned to " + controls.NativeControl2Text(controls.ConfTagLController2Value(confTag)) + 
            " (" + std::to_string(controls.ConfTagLController2Value(confTag)) + ")");

        if (menu.OptionPlus("Assign " + confTag, controllerInfo, nullptr, std::bind(clearLControllerButton, confTag), nullptr, "Current setting")) {
            WAIT(500);
            bool result = configLControllerButton(confTag);
            //showNotification(result ? (confTag + " saved").c_str() : ("Cancelled " + confTag + " assignment").c_str(), &prevNotification);
            if (!result) showNotification(("Cancelled " + confTag + " assignment").c_str(), &prevNotification);
            WAIT(500);
        }
        it++;
        controllerInfo.pop_back();
    }
}

void update_controllermenu() {
    menu.Title("Controller controls");
    menu.Subtitle("Controller options");

    menu.BoolOption("Engine button toggles", settings.ToggleEngine, 
                    { "Checked: the engine button turns the engine on AND off.",
                        "Not checked: the button only turns the engine on when it's off." });

    menu.IntOption("Long press time (ms)", controls.CToggleTime, 100, 5000, 50,
                   { "Timeout for long press buttons to activate." });

    menu.IntOption("Max tap time (ms)", controls.MaxTapTime, 50, 1000, 10,
                   { "Buttons pressed and released within this time are regarded as a tap. Shift up, shift down are tap controls." });

    float currTriggerValue = controls.GetXboxTrigger();
    float prevTriggerValue = currTriggerValue;
    menu.FloatOption("Trigger value", currTriggerValue, 0.25, 1.0, 0.05,
                     { "Threshold for an analog input to be detected as button press." });

    if (currTriggerValue != prevTriggerValue) {
        controls.SetXboxTrigger(currTriggerValue);
    }

    menu.BoolOption("Block car controls", settings.BlockCarControls,
                    { "Blocks car action controls. Holding activates the original button again.",
                        "Experimental!" });

    menu.BoolOption("Ignore shifts in UI", settings.IgnoreShiftsUI,
    { "Ignore shift up/shift down while using the phone or when the menu is open" });

    int oldIndexUp = getBlockableControlIndex(controls.ControlXboxBlocks[static_cast<int>(ScriptControls::ControllerControlType::ShiftUp)]);
    if (menu.StringArray("Shift Up blocks", blockableControlsHelp, oldIndexUp)) {
        controls.ControlXboxBlocks[static_cast<int>(ScriptControls::ControllerControlType::ShiftUp)] = blockableControls[oldIndexUp];
    }
        
    int oldIndexDown = getBlockableControlIndex(controls.ControlXboxBlocks[static_cast<int>(ScriptControls::ControllerControlType::ShiftDown)]);
    if (menu.StringArray("Shift Down blocks", blockableControlsHelp, oldIndexDown)) {
        controls.ControlXboxBlocks[static_cast<int>(ScriptControls::ControllerControlType::ShiftDown)] = blockableControls[oldIndexDown];
    }

    int oldIndexClutch = getBlockableControlIndex(controls.ControlXboxBlocks[static_cast<int>(ScriptControls::ControllerControlType::Clutch)]);
    if (menu.StringArray("Clutch blocks", blockableControlsHelp, oldIndexClutch)) {
        controls.ControlXboxBlocks[static_cast<int>(ScriptControls::ControllerControlType::Clutch)] = blockableControls[oldIndexClutch];
    }

    std::vector<std::string> controllerInfo;
    controllerInfo.push_back("Press RIGHT to clear key");
    controllerInfo.push_back("Press RETURN to configure button");
    controllerInfo.push_back("");

    auto it = 0;
    for (auto confTag : controllerConfTags) {
        controllerInfo.back() = controllerConfTagDetail.at(it);
        controllerInfo.push_back("Assigned to " + controls.ConfTagController2Value(confTag));
        if (menu.OptionPlus("Assign " + confTag, controllerInfo, nullptr, std::bind(clearControllerButton, confTag), nullptr, "Current setting")) {
            WAIT(500);
            bool result = configControllerButton(confTag);
            //showNotification(result ? (confTag + " saved").c_str() : ("Cancelled " + confTag + " assignment").c_str(), &prevNotification);
            if (!result) showNotification(("Cancelled " + confTag + " assignment").c_str(), &prevNotification);
            WAIT(500);
        }
        it++;
        controllerInfo.pop_back();
    }
}

void update_keyboardmenu() {
    menu.Title("Keyboard controls");
    menu.Subtitle("Keyboard assignments");

    std::vector<std::string> keyboardInfo;
    keyboardInfo.push_back("Press RIGHT to clear key");
    keyboardInfo.push_back("Press RETURN to configure key");
    keyboardInfo.push_back("");

    int it = 0;
    for (auto confTag : keyboardConfTags) {
        keyboardInfo.back() = keyboardConfTagsDetail.at(it);
        keyboardInfo.push_back("Assigned to " + key2str(controls.ConfTagKB2key(confTag)));
        if (menu.OptionPlus("Assign " + confTag, keyboardInfo, nullptr, std::bind(clearKeyboardKey, confTag), nullptr, "Current setting")) {
            WAIT(500);
            bool result = configKeyboardKey(confTag);
            if (!result) showNotification(("Cancelled " + confTag + " assignment").c_str(), &prevNotification);
            WAIT(500);
        }
        it++;
        keyboardInfo.pop_back();
    }
}

void update_wheelmenu() {
    menu.Title("Steering wheel");
    menu.Subtitle("Steering wheel options");

    if (menu.BoolOption("Enable wheel", settings.EnableWheel,
                        { "Enable usage of a steering wheel." })) {
        settings.SaveWheel(&controls);
        settings.Read(&controls);
        initWheel();
    }

    if (menu.BoolOption("Enable wheel always", settings.WheelWithoutManual,
                        { "Enable your wheel even when the Manual Transmission part of this mod is off." })) {
        settings.SaveWheel(&controls);
    }
        
    if (menu.BoolOption("Patch steering", settings.PatchSteering,
                        { "Patches steering reduction and automatic countersteer for more direct control." })) {
        settings.SaveWheel(&controls);
        initSteeringPatches();
    }

    if (menu.FloatOption("Steering reduction (wheel)", settings.SteeringReductionWheel, 0.0f, 1.0f, 0.01f, 
                         { "Reduce steering input at higher speeds. Best to leave it at 0 for steering wheels." })) {
        settings.SaveWheel(&controls);
    }

    if (menu.FloatOption("Steering multiplier (wheel)", settings.GameSteerMultWheel, 0.1f, 2.0f, 0.01f,
                         { "Increase steering lock for all cars. You might want to increase it for faster steering and more steering lock." })) {
        updateSteeringMultiplier();
    }

    if (menu.BoolOption("Disable non-wheel steering", settings.PatchSteeringControl,
                        { "Disable keyboard and controller inputs for steering. This fixes the jerky animation." })) {
        settings.SaveWheel(&controls);
        initSteeringPatches();
    }

    if (menu.BoolOption("Logitech RPM LEDs", settings.LogiLEDs, 
                        { "Show the RPM LEDs on Logitech steering wheels. If the wheel doesn't have compatible RPM LEDs, this might crash." } )) {
        settings.SaveWheel(&controls);
    }

    menu.MenuOption("Steering wheel axis setup", "axesmenu", 
                    { "Configure the analog controls." });
        
    menu.MenuOption("Force feedback options", "forcefeedbackmenu",
                    { "Fine-tune your force feedback parameters." });
        
    menu.MenuOption("Steering wheel angles", "anglemenu",
                    { "Set up soft lock options here." });
        
    menu.MenuOption("Steering wheel button setup", "buttonsmenu",
                    { "Set up your buttons on your steering wheel." });

    std::vector<std::string> hpatInfo;
    hpatInfo.push_back("Press RIGHT to clear H-pattern shifter");
    hpatInfo.push_back("Active gear:");
    if (controls.ButtonIn(ScriptControls::WheelControlType::HR)) hpatInfo.push_back("Reverse");
    if (controls.ButtonIn(ScriptControls::WheelControlType::H1)) hpatInfo.push_back("Gear 1");
    if (controls.ButtonIn(ScriptControls::WheelControlType::H2)) hpatInfo.push_back("Gear 2");
    if (controls.ButtonIn(ScriptControls::WheelControlType::H3)) hpatInfo.push_back("Gear 3");
    if (controls.ButtonIn(ScriptControls::WheelControlType::H4)) hpatInfo.push_back("Gear 4");
    if (controls.ButtonIn(ScriptControls::WheelControlType::H5)) hpatInfo.push_back("Gear 5");
    if (controls.ButtonIn(ScriptControls::WheelControlType::H6)) hpatInfo.push_back("Gear 6");
    if (controls.ButtonIn(ScriptControls::WheelControlType::H7)) hpatInfo.push_back("Gear 7");

    if (menu.OptionPlus("H-pattern shifter setup", hpatInfo, nullptr, std::bind(clearHShifter), nullptr, "Input values",
                        { "Select this option to start H-pattern shifter setup. Follow the on-screen instructions." })) {
        bool result = configHPattern();
        showNotification(result ? "H-pattern shifter saved" : "Cancelled H-pattern shifter setup", &prevNotification);
    }

    menu.BoolOption("Keyboard H-pattern", settings.HPatternKeyboard, 
                    { "This allows you to use the keyboard for H-pattern shifting. Configure the controls in the keyboard section." });
}

void update_anglemenu() {
    menu.Title("Wheel angles");
    menu.Subtitle("Soft lock & angle setup");
    float minLock = 180.0f;
    if (menu.FloatOption("Physical degrees", settings.SteerAngleMax, minLock, 1080.0, 30.0,
                         { "How many degrees your wheel can physically turn." })) {
        if (settings.SteerAngleCar > settings.SteerAngleMax) { settings.SteerAngleCar = settings.SteerAngleMax; }
        if (settings.SteerAngleBike > settings.SteerAngleMax) { settings.SteerAngleBike = settings.SteerAngleMax; }
        if (settings.SteerAngleBoat > settings.SteerAngleMax) { settings.SteerAngleBoat = settings.SteerAngleMax; }
    }
    menu.FloatOption("Car soft lock", settings.SteerAngleCar, minLock, settings.SteerAngleMax, 30.0,
                     { "Soft lock for cars and trucks. (degrees)" });

    menu.FloatOption("Bike soft lock", settings.SteerAngleBike, minLock, settings.SteerAngleMax, 30.0,
                     { "Soft lock for bikes and quads. (degrees)" });

    menu.FloatOption("Boat soft lock", settings.SteerAngleBoat, minLock, settings.SteerAngleMax, 30.0,
                     { "Soft lock for boats. (degrees)" });
}

void incGamma(float max, float step) {
    if (settings.BrakeGamma + step > max) return;
    settings.BrakeGamma += step;
}

void decGamma(float min, float step) {
    if (settings.BrakeGamma - step < min) return;
    settings.BrakeGamma -= step;
}

std::vector<std::string> showGammaCurve() {

    std::string larr = "< ";
    std::string rarr = " >";
    if (settings.BrakeGamma >= 5.0f - 0.01f) rarr = "";
    if (settings.BrakeGamma <= 0.1f + 0.01f) larr = "";

    char buf[100];
    _snprintf_s(buf, sizeof(buf), "%.*f", 2, settings.BrakeGamma);
    std::string printVar = buf;

    std::vector<std::string> info {
        "Brake gamma:",
        larr + printVar + rarr
    };

    const int max_samples = 100;
    std::vector<std::pair<float, float>> points;
    for (int i = 0; i < max_samples; i++) {
        float x = (float)i / (float)max_samples;
        float y = pow(x, settings.BrakeGamma);
        points.push_back({ x, y });
    }

    float rectX = 0.5f;
    float rectY = 0.5f;
    float rectW = 0.40f / GRAPHICS::_GET_ASPECT_RATIO(FALSE);
    float rectH = 0.40f;
    float blockW = rectW / max_samples;//0.001f * (16.0f / 9.0f) / GRAPHICS::_GET_ASPECT_RATIO(FALSE);
    float blockH = blockW * GRAPHICS::_GET_ASPECT_RATIO(FALSE);

    GRAPHICS::DRAW_RECT(rectX, rectY, 
                        rectW + 3.0f*blockW, rectH + 3.0f*blockH,
                        255, 255, 255, 191);
    GRAPHICS::DRAW_RECT(rectX, rectY, 
                        rectW + blockW / 2.0f, rectH + blockH / 2.0f, 
                        0, 0, 0, 239);

    for (auto point : points) {
        float pointX = rectX - 0.5f*rectW + point.first *rectW;
        float pointY = rectY + 0.5f*rectH - point.second *rectH;
        GRAPHICS::DRAW_RECT(pointX, pointY, 
                            blockW, blockH, 
                            255, 255, 255, 255);
    }

    std::pair<float, float> currentPoint = { controls.BrakeValAvg, pow(controls.BrakeValAvg, settings.BrakeGamma) };
    float pointX = rectX - 0.5f*rectW + currentPoint.first * rectW;
    float pointY = rectY + 0.5f*rectH - currentPoint.second * rectH;
    GRAPHICS::DRAW_RECT(pointX, pointY, 
                        3.0f*blockW, 3.0f*blockH, 
                        255, 0, 0, 255);
    
    return info;
}
 
void update_axesmenu() {
    menu.Title("Configure axes");
    menu.Subtitle("Setup steering and pedals");
    controls.UpdateValues(ScriptControls::Wheel, true);
    std::vector<std::string> info = {
        "Press RIGHT to clear this axis" ,
        "Steer    : " + std::to_string(controls.SteerVal),
        "Throttle : " + std::to_string(controls.ThrottleVal),
        "Brake    : " + std::to_string(controls.BrakeVal),
        "Clutch   : " + std::to_string(controls.ClutchValRaw),
        "Handbrake: " + std::to_string(controls.HandbrakeVal),
    };

    if (menu.OptionPlus("Configure steering", info, nullptr, std::bind(clearAxis, "STEER"), nullptr, "Input values")) {
        bool result = configAxis("STEER");
        showNotification(result ? "Steering axis saved" : "Cancelled steering axis configuration", &prevNotification);
        if (result) initWheel();
    }
    if (menu.OptionPlus("Configure throttle", info, nullptr, std::bind(clearAxis, "THROTTLE"), nullptr, "Input values")) {
        bool result = configAxis("THROTTLE");
        showNotification(result ? "Throttle axis saved" : "Cancelled throttle axis configuration", &prevNotification);
        if (result) initWheel();
    }
    if (menu.OptionPlus("Configure brake", info, nullptr, std::bind(clearAxis, "BRAKES"), nullptr, "Input values")) {
        bool result = configAxis("BRAKES");
        showNotification(result ? "Brake axis saved" : "Cancelled brake axis configuration", &prevNotification);
        if (result) initWheel();
    }
    if (menu.OptionPlus("Configure clutch", info, nullptr, std::bind(clearAxis, "CLUTCH"), nullptr, "Input values")) {
        bool result = configAxis("CLUTCH");
        showNotification(result ? "Clutch axis saved" : "Cancelled clutch axis configuration", &prevNotification);
        if (result) initWheel();
    }
    if (menu.OptionPlus("Configure handbrake", info, nullptr, std::bind(clearAxis, "HANDBRAKE_ANALOG"), nullptr, "Input values")) {
        bool result = configAxis("HANDBRAKE_ANALOG");
        showNotification(result ? "Handbrake axis saved" : "Cancelled handbrake axis configuration", &prevNotification);
        if (result) initWheel();
    }

    menu.FloatOption("Steering deadzone", controls.DZSteer, 0.0f, 0.5f, 0.01f, 
                     { "Deadzone size, from the center of the wheel." });

    menu.FloatOption("Steering deadzone offset", controls.DZSteerOffset, -0.5f, 0.5f, 0.01f,
                     { "Put the deadzone with an offset from the center." });

    if (!settings.PatchSteeringControl) {
        menu.FloatOption("Steering anti-deadzone", controls.ADZSteer, 0.0f, 1.0f, 0.01f,
                         { "GTA V ignores 25% input for analog controls by default." });
    }

    menu.FloatOption("Throttle anti-deadzone",	controls.ADZThrottle, 0.0f, 1.0f, 0.01f, 
                     { "GTA V ignores 25% input for analog controls by default." });

    menu.FloatOption("Brake anti-deadzone",		controls.ADZBrake, 0.0f, 1.0f, 0.01f,
                     { "GTA V ignores 25% input for analog controls by default." });

    bool showGamma = false;
    std::vector<std::string> extras = {};
    menu.OptionPlus("Brake gamma", extras, &showGamma, std::bind(incGamma, 5.0f, 0.01f), std::bind(decGamma, 0.1f, 0.01f), "Brake gamma", 
                    { "Linearity of the brake pedal. Values over 1.0 feel more real if you have progressive springs." });
    if (showGamma) {
        extras = showGammaCurve();
        menu.OptionPlusPlus(extras, "Brake gamma");
    }

    menu.BoolOption("Invert steer", controls.InvertSteer);
    menu.BoolOption("Invert throttle", controls.InvertThrottle);
    menu.BoolOption("Invert brake", controls.InvertBrake);
    menu.BoolOption("Invert clutch", controls.InvertClutch);
}

void update_forcefeedbackmenu() {
    menu.Title("Force feedback");
    menu.Subtitle("Force feedback setup");

    menu.BoolOption("Enable", settings.EnableFFB, 
                    { "Enable or disable force feedback entirely." });

    menu.BoolOption("Scale forces", settings.ScaleFFB, 
                    { "Scale forces to degree of rotation." });

    menu.FloatOption("Self aligning torque multiplier", settings.SATAmpMult, 0.1f, 5.0f, 0.05f,
                     { "Force feedback strength for steering. Increase for weak wheels, decrease for strong/fast wheels.",
                     "Putting this too high clips force feedback. Too low and the car doesn't feel responsive."});

    menu.FloatOption("Detail effect multiplier", settings.DetailMult, 0.0f, 10.0f, 0.1f,
                     { "Force feedback effects caused by the suspension." });

    menu.FloatOption("Collision effect multiplier", settings.CollisionMult, 0.0f, 5.0f, 0.1f,
                     { "Force feedback effect caused by frontal/rear collisions." });

    menu.IntOption("Damper max (low speed)", settings.DamperMax, 0, 200, 1, 
                   { "Wheel friction at low speed." });
        
    menu.IntOption("Damper min (high speed)", settings.DamperMin, 0, 200, 1,
                   { "Wheel friction at high speed." });
        
    menu.FloatOption("Damper min speed", settings.DamperMinSpeed, 0.0f, 40.0f, 0.2f,
                     { "Speed where the damper strength should be minimal.", "In m/s." });
}

void update_buttonsmenu() {
    menu.Title("Configure buttons");
    menu.Subtitle("Button setup and review");

    std::vector<std::string> wheelToKeyInfo;
    wheelToKeyInfo.push_back("Active wheel-to-key options:");
    wheelToKeyInfo.push_back("Press RIGHT to clear a button");
    wheelToKeyInfo.push_back("Device: " + settings.GUIDToDeviceIndex(controls.WheelToKeyGUID));
    for (int i = 0; i < MAX_RGBBUTTONS; i++) {
        if (controls.WheelToKey[i] != -1) {
            wheelToKeyInfo.push_back(std::to_string(i) + " = " + key2str(controls.WheelToKey[i]));
            if (controls.WheelControl.IsButtonPressed(i, controls.WheelToKeyGUID)) {
                wheelToKeyInfo.back() += " (Pressed)";
            }
        }
    }

    if (menu.OptionPlus("Set up WheelToKey", wheelToKeyInfo, nullptr, clearWheelToKey, nullptr, "Info",
                        { "Set up wheel buttons that press a keyboard key. Only one device can be used for this." })) {
        bool result = configWheelToKey();
        showNotification(result ? "Entry added" : "Cancelled entry addition", &prevNotification);
    }

    std::vector<std::string> buttonInfo;
    buttonInfo.push_back("Press RIGHT to clear this button");
    buttonInfo.push_back("Active buttons:");
    if (controls.ButtonIn(ScriptControls::WheelControlType::HR)) buttonInfo.push_back("Gear R");
    if (controls.ButtonIn(ScriptControls::WheelControlType::H1)) buttonInfo.push_back("Gear 1");
    if (controls.ButtonIn(ScriptControls::WheelControlType::H2)) buttonInfo.push_back("Gear 2");
    if (controls.ButtonIn(ScriptControls::WheelControlType::H3)) buttonInfo.push_back("Gear 3");
    if (controls.ButtonIn(ScriptControls::WheelControlType::H4)) buttonInfo.push_back("Gear 4");
    if (controls.ButtonIn(ScriptControls::WheelControlType::H5)) buttonInfo.push_back("Gear 5");
    if (controls.ButtonIn(ScriptControls::WheelControlType::H6)) buttonInfo.push_back("Gear 6");
    if (controls.ButtonIn(ScriptControls::WheelControlType::H7)) buttonInfo.push_back("Gear 7");
    if (controls.ButtonIn(ScriptControls::WheelControlType::ShiftUp))	buttonInfo.push_back("ShiftUp");
    if (controls.ButtonIn(ScriptControls::WheelControlType::ShiftDown)) buttonInfo.push_back("ShiftDown");
    if (controls.ButtonIn(ScriptControls::WheelControlType::Clutch))	buttonInfo.push_back("ClutchButton");
    if (controls.ButtonIn(ScriptControls::WheelControlType::Engine))	buttonInfo.push_back("Engine");
    if (controls.ButtonIn(ScriptControls::WheelControlType::Handbrake)) buttonInfo.push_back("Handbrake");
    if (controls.ButtonIn(ScriptControls::WheelControlType::Horn))		buttonInfo.push_back("Horn");
    if (controls.ButtonIn(ScriptControls::WheelControlType::Lights))	buttonInfo.push_back("Lights");
    if (controls.ButtonIn(ScriptControls::WheelControlType::LookBack))	buttonInfo.push_back("LookBack");
    if (controls.ButtonIn(ScriptControls::WheelControlType::LookLeft))	buttonInfo.push_back("LookLeft");
    if (controls.ButtonIn(ScriptControls::WheelControlType::LookRight))	buttonInfo.push_back("LookRight");
    if (controls.ButtonIn(ScriptControls::WheelControlType::Camera))	buttonInfo.push_back("Camera");
    if (controls.ButtonIn(ScriptControls::WheelControlType::RadioNext)) buttonInfo.push_back("RadioNext");
    if (controls.ButtonIn(ScriptControls::WheelControlType::RadioPrev)) buttonInfo.push_back("RadioPrev");
    if (controls.ButtonIn(ScriptControls::WheelControlType::IndicatorLeft))		buttonInfo.push_back("IndicatorLeft");
    if (controls.ButtonIn(ScriptControls::WheelControlType::IndicatorRight))	buttonInfo.push_back("IndicatorRight");
    if (controls.ButtonIn(ScriptControls::WheelControlType::IndicatorHazard))	buttonInfo.push_back("IndicatorHazard");
    if (controls.ButtonIn(ScriptControls::WheelControlType::Toggle))	buttonInfo.push_back("ToggleMod");
    if (controls.ButtonIn(ScriptControls::WheelControlType::ToggleH))	buttonInfo.push_back("ChangeShiftMode");
    if (buttonInfo.size() == 2) buttonInfo.push_back("None");

    for (auto confTag : buttonConfTags) {
        buttonInfo.push_back("Assigned to " + std::to_string(controls.ConfTagWheel2Value(confTag)));
        if (menu.OptionPlus("Assign " + confTag, buttonInfo, nullptr, std::bind(clearButton, confTag), nullptr, "Current inputs")) {
            bool result = configButton(confTag);
            showNotification(result ? (confTag + " saved").c_str() : ("Cancelled " + confTag + " assignment").c_str(), &prevNotification);
        }
        buttonInfo.pop_back();
    }
}

void update_hudmenu() {
    menu.Title("HUD Options");
    menu.Subtitle("");

    menu.BoolOption("Enable", settings.HUD,
                    { "Display HUD elements." });

    menu.BoolOption("Always enable", settings.AlwaysHUD,
                    { "Display HUD even if manual transmission is off." });

    int fontIndex = static_cast<int>(std::find(fontIDs.begin(), fontIDs.end(), settings.HUDFont) - fontIDs.begin());
    if (menu.StringArray("Font: ", fonts, fontIndex, { "Select the font for speed, gearbox mode, current gear." })) {
        settings.HUDFont = fontIDs.at(fontIndex);
    }

    menu.MenuOption("Gear and shift mode", "geardisplaymenu");
    menu.MenuOption("Speedometer", "speedodisplaymenu");
    menu.MenuOption("RPM Gauge", "rpmdisplaymenu");

    std::vector<std::string> wheelTexturePresent = {};

    if (textureWheelId == -1) {
        wheelTexturePresent.push_back("Wheel texture not found!");
    }

    menu.MenuOption("Wheel & Pedal Info", "wheelinfomenu");
}

void update_geardisplaymenu() {
    menu.Title("Gear options");
    menu.Subtitle("");

    menu.BoolOption("Gear", settings.GearIndicator);
    menu.BoolOption("Shift Mode", settings.ShiftModeIndicator);

    menu.FloatOption("Gear X", settings.GearXpos, 0.0f, 1.0f, 0.005f);
    menu.FloatOption("Gear Y", settings.GearYpos, 0.0f, 1.0f, 0.005f);
    menu.FloatOption("Gear Size", settings.GearSize, 0.0f, 3.0f, 0.05f);
    menu.IntOption("Gear Top Color Red", settings.GearTopColorR, 0, 255);
    menu.IntOption("Gear Top Color Green", settings.GearTopColorG, 0, 255);
    menu.IntOption("Gear Top Color Blue", settings.GearTopColorB, 0, 255);

    menu.FloatOption("Shift Mode X", settings.ShiftModeXpos, 0.0f, 1.0f, 0.005f);
    menu.FloatOption("Shift Mode Y", settings.ShiftModeYpos, 0.0f, 1.0f, 0.005f);
    menu.FloatOption("Shift Mode Size", settings.ShiftModeSize, 0.0f, 3.0f, 0.05f);
}

void update_speedodisplaymenu() {
    menu.Title("Speedometer options");
    menu.Subtitle("");

    ptrdiff_t oldPos = std::find(speedoTypes.begin(), speedoTypes.end(), settings.Speedo) - speedoTypes.begin();
    menu.StringArray("Speedometer", speedoTypes, speedoIndex);
    if (speedoIndex != oldPos) {
        settings.Speedo = speedoTypes.at(speedoIndex);
    }
    menu.BoolOption("Show units", settings.SpeedoShowUnit);

    menu.FloatOption("Speedometer X", settings.SpeedoXpos, 0.0f, 1.0f, 0.005f);
    menu.FloatOption("Speedometer Y", settings.SpeedoYpos, 0.0f, 1.0f, 0.005f);
    menu.FloatOption("Speedometer Size", settings.SpeedoSize, 0.0f, 3.0f, 0.05f);
}

void update_rpmdisplaymenu() {
    menu.Title("RPM Gauge options");
    menu.Subtitle("");

    menu.BoolOption("RPM Gauge", settings.RPMIndicator);
    menu.FloatOption("RPM Redline", settings.RPMIndicatorRedline, 0.0f, 1.0f, 0.01f);

    menu.FloatOption("RPM X", settings.RPMIndicatorXpos, 0.0f, 1.0f, 0.0025f);
    menu.FloatOption("RPM Y", settings.RPMIndicatorYpos, 0.0f, 1.0f, 0.0025f);
    menu.FloatOption("RPM Width", settings.RPMIndicatorWidth, 0.0f, 1.0f, 0.0025f);
    menu.FloatOption("RPM Height", settings.RPMIndicatorHeight, 0.0f, 1.0f, 0.0025f);

    menu.IntOption("RPM Background Red	", settings.RPMIndicatorBackgroundR, 0, 255);
    menu.IntOption("RPM Background Green", settings.RPMIndicatorBackgroundG, 0, 255);
    menu.IntOption("RPM Background Blue	", settings.RPMIndicatorBackgroundB, 0, 255);
    menu.IntOption("RPM Background Alpha", settings.RPMIndicatorBackgroundA, 0, 255);

    menu.IntOption("RPM Foreground Red	", settings.RPMIndicatorForegroundR, 0, 255);
    menu.IntOption("RPM Foreground Green", settings.RPMIndicatorForegroundG, 0, 255);
    menu.IntOption("RPM Foreground Blue	", settings.RPMIndicatorForegroundB, 0, 255);
    menu.IntOption("RPM Foreground Alpha", settings.RPMIndicatorForegroundA, 0, 255);

    menu.IntOption("RPM Redline Red		", settings.RPMIndicatorRedlineR, 0, 255);
    menu.IntOption("RPM Redline Green	", settings.RPMIndicatorRedlineG, 0, 255);
    menu.IntOption("RPM Redline Blue	", settings.RPMIndicatorRedlineB, 0, 255);
    menu.IntOption("RPM Redline Alpha	", settings.RPMIndicatorRedlineA, 0, 255);

    menu.IntOption("RPM Revlimit Red	", settings.RPMIndicatorRevlimitR, 0, 255);
    menu.IntOption("RPM Revlimit Green	", settings.RPMIndicatorRevlimitG, 0, 255);
    menu.IntOption("RPM Revlimit Blue	", settings.RPMIndicatorRevlimitB, 0, 255);
    menu.IntOption("RPM Revlimit Alpha	", settings.RPMIndicatorRevlimitA, 0, 255);
}

void update_wheelinfomenu() {
    menu.Title("Wheel & Pedal Info");
    menu.Subtitle("");

    menu.BoolOption("Display steering wheel info", settings.SteeringWheelInfo, { "Show input info graphically." });
    menu.BoolOption("Always display info", settings.AlwaysSteeringWheelInfo, { "Display the info even without a steering wheel." });
    menu.FloatOption("Wheel image X", settings.SteeringWheelTextureX, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Wheel image Y", settings.SteeringWheelTextureY, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Wheel image size", settings.SteeringWheelTextureSz, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Pedals X", settings.PedalInfoX, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Pedals Y", settings.PedalInfoY, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Pedals Height", settings.PedalInfoH	, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Pedals Width", settings.PedalInfoW	, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Pedals Pad X", settings.PedalInfoPadX, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Pedals Pad Y", settings.PedalInfoPadY, 0.0f, 1.0f, 0.01f);
}

void update_debugmenu() {
    menu.Title("Debug settings");
    menu.Subtitle("Extra mod info");

    menu.BoolOption("Display info", settings.DisplayInfo, 
                    { "Show all detailed technical info of the gearbox and inputs calculations." });
    menu.BoolOption("Display car wheel info", settings.DisplayWheelInfo, 
                    { "Show per-wheel debug info with off-ground detection, lockup detection and suspension info." });
    menu.BoolOption("Display gearing info", settings.DisplayGearingInfo,
                    { "Show gear ratios and shift points from auto mode." });
    menu.BoolOption("Display force feedback lines", settings.DisplayFFBInfo,
                    { "Show lines detailing force feedback direction and force.",
                        "Green: Vehicle velocity","Red: Vehicle rotation","Purple: Steering direction"});
    menu.BoolOption("Show NPC info", settings.ShowNPCInfo,
                    { "Show vehicle info of NPC vehicles near you." });
}


// These tabs look fine in Visual Studio with Tab = 4 spaces
void update_menu() {
    menu.CheckKeys();

    /* mainmenu */
    if (menu.CurrentMenu("mainmenu")) { 			update_mainmenu(); 				}

    /* mainmenu -> optionsmenu */
    if (menu.CurrentMenu("optionsmenu")) { 			update_optionsmenu(); 			}

    /* mainmenu -> optionsmenu -> miscoptionsmenu */
    if (menu.CurrentMenu("miscoptionsmenu")) { 		update_miscoptionsmenu(); 		}

    /* mainmenu -> optionsmenu -> finetuneoptionsmenu */
    if (menu.CurrentMenu("finetuneoptionsmenu")) { 	update_finetuneoptionsmenu(); 	}

    /* mainmenu -> controlsmenu */
    if (menu.CurrentMenu("controlsmenu")) { 		update_controlsmenu(); 			}

    /* mainmenu -> controlsmenu -> legacycontrollermenu */
    if (menu.CurrentMenu("legacycontrollermenu")) {	update_legacycontrollermenu(); 	}

    /* mainmenu -> controlsmenu -> controllermenu */
    if (menu.CurrentMenu("controllermenu")) { 		update_controllermenu(); 		}

    /* mainmenu -> controlsmenu -> keyboardmenu */
    if (menu.CurrentMenu("keyboardmenu")) { 		update_keyboardmenu();			}

    /* mainmenu -> wheelmenu */
    if (menu.CurrentMenu("wheelmenu")) { 			update_wheelmenu(); 			}

    /* mainmenu -> wheelmenu -> anglemenu */
    if (menu.CurrentMenu("anglemenu")) { 			update_anglemenu(); 			}

    /* mainmenu -> wheelmenu -> axesmenu */
    if (menu.CurrentMenu("axesmenu")) { 			update_axesmenu(); 				}

    /* mainmenu -> wheelmenu -> forcefeedbackmenu */
    if (menu.CurrentMenu("forcefeedbackmenu")) { 	update_forcefeedbackmenu(); 	}

    /* mainmenu -> wheelmenu -> buttonsmenu */
    if (menu.CurrentMenu("buttonsmenu")) { 			update_buttonsmenu(); 			}

    /* mainmenu -> hudmenu */
    if (menu.CurrentMenu("hudmenu")) { 				update_hudmenu(); 				}

    /* mainmenu -> hudmenu -> geardisplaymenu*/
    if (menu.CurrentMenu("geardisplaymenu")) { 		update_geardisplaymenu(); 		}

    /* mainmenu -> hudmenu -> speedodisplaymenu*/
    if (menu.CurrentMenu("speedodisplaymenu")) { 	update_speedodisplaymenu(); 	}

    /* mainmenu -> hudmenu -> rpmdisplaymenu*/
    if (menu.CurrentMenu("rpmdisplaymenu")) { 		update_rpmdisplaymenu(); 		}

    /* mainmenu -> hudmenu -> wheelinfomenu*/
    if (menu.CurrentMenu("wheelinfomenu")) { 		update_wheelinfomenu(); 		}

    /* mainmenu -> debugmenu */
    if (menu.CurrentMenu("debugmenu")) { 			update_debugmenu(); 			}

    menu.EndMenu();
}


///////////////////////////////////////////////////////////////////////////////
//                              Config helpers/util
///////////////////////////////////////////////////////////////////////////////

// Wheel section
// Look at that argument list! :D
bool getConfigAxisWithValues(std::vector<std::tuple<GUID, std::string, int>> startStates, GUID &selectedGUID, std::string &selectedAxis, int hyst, bool &positive, int &startValue_) {
    for (auto guid : controls.WheelControl.GetGuids()) {
        for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
            for (auto startState : startStates) {
                std::string axisName = controls.WheelControl.DIAxisHelper[i];
                int axisValue = controls.WheelControl.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), guid);
                int startValue = std::get<2>(startState);
                if (std::get<0>(startState) == guid &&
                    std::get<1>(startState) == axisName) {
                    startValue_ = startValue;
                    if (axisValue > startValue + hyst) { // 0 (min) - 65535 (max)
                        selectedGUID = guid;
                        selectedAxis = axisName;
                        positive = true;
                        return true;
                    }
                    if (axisValue < startValue - hyst) { // 65535 (min) - 0 (max)
                        selectedGUID = guid;
                        selectedAxis = axisName;
                        positive = false;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void saveAxis(const std::string &confTag, GUID devGUID, std::string axis, int min, int max) {
    std::wstring wDevName = controls.WheelControl.FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
    std::string devName = std::string(wDevName.begin(), wDevName.end());
    auto index = settings.SteeringAppendDevice(devGUID, devName);
    settings.SteeringSaveAxis(confTag, index, axis, min, max);
    if (confTag == "STEER") {
        settings.SteeringSaveFFBAxis(confTag, index, axis);
    }
    settings.Read(&controls);
}

void saveButton(const std::string &confTag, GUID devGUID, int button) {
    std::wstring wDevName = controls.WheelControl.FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
    std::string devName = std::string(wDevName.begin(), wDevName.end()).c_str();
    auto index = settings.SteeringAppendDevice(devGUID, devName.c_str());
    settings.SteeringSaveButton(confTag, index, button);
    settings.Read(&controls);
}

void addWheelToKey(const std::string &confTag, GUID devGUID, int button, std::string keyName) {
    std::wstring wDevName = controls.WheelControl.FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
    std::string devName = std::string(wDevName.begin(), wDevName.end()).c_str();
    auto index = settings.SteeringAppendDevice(devGUID, devName.c_str());
    settings.SteeringAddWheelToKey(confTag, index, button, keyName);
    settings.Read(&controls);
}

void saveHShifter(const std::string &confTag, GUID devGUID, std::array<int, NUMGEARS> buttonArray) {
    std::wstring wDevName = controls.WheelControl.FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
    std::string devName = std::string(wDevName.begin(), wDevName.end()).c_str();
    auto index = settings.SteeringAppendDevice(devGUID, devName);
    settings.SteeringSaveHShifter(confTag, index, buttonArray.data());
    settings.Read(&controls);
}

void clearAxis(std::string confTag) {
    settings.SteeringSaveAxis(confTag, -1, "", 0, 0);
    settings.Read(&controls);
    showNotification(("Cleared axis " + confTag).c_str(), &prevNotification);
    initWheel();
}

void clearWheelToKey() {
    GAMEPLAY::DISPLAY_ONSCREEN_KEYBOARD(1, "VEUI_ENTER_TEXT", "", "", "", "", "", 30);
    while (GAMEPLAY::UPDATE_ONSCREEN_KEYBOARD() == 0) {
        CONTROLS::DISABLE_ALL_CONTROL_ACTIONS(0);
        WAIT(0);
    }
    if (!GAMEPLAY::GET_ONSCREEN_KEYBOARD_RESULT()) return;
    std::string result = GAMEPLAY::GET_ONSCREEN_KEYBOARD_RESULT();

    int button;
    if (str2int(button, result.c_str(), 10) != STR2INT_SUCCESS) {
        showNotification("Invalid input: " + result + " is not a valid number!", &prevNotification);
        return;
    }
    bool found = settings.SteeringClearWheelToKey(button);
    if (found) {
        showNotification("Removed button " + result, &prevNotification);
        settings.Read(&controls);
    }
    else {
        showNotification("Button " + result + " not found.", &prevNotification);
    }
}

void clearButton(std::string confTag) {
    settings.SteeringSaveButton(confTag, -1, -1);
    settings.Read(&controls);
    showNotification(("Cleared button " + confTag).c_str(), &prevNotification);
}

void clearHShifter() {
    int empty[NUMGEARS] = {};
    for (int i = 0; i < NUMGEARS; i++) {
        empty[i] = -1;
    }
    settings.SteeringSaveHShifter("SHIFTER", -1, empty);
    settings.Read(&controls);
    showNotification("Cleared H-pattern shifter", &prevNotification);
}

// stick
//bool getConfigAxisStickWithValues(std::vector<std::tuple<GUID, std::string, int>> startStates, GUID &selectedGUID, std::string &selectedAxis, int hyst, bool &positive, int &startValue_) {
//	for (auto guid : controls.StickControl.GetGUIDs()) {
//		for (int i = 0; i < GenDInput::SIZEOF_DIAxis - 1; i++) {
//			for (auto startState : startStates) {
//				std::string axisName = controls.StickControl.DIAxisHelper[i];
//				int axisValue = controls.StickControl.GetAxisValue(guid, static_cast<GenDInput::DIAxis>(i));
//				int startValue = std::get<2>(startState);
//				if (std::get<0>(startState) == guid &&
//					std::get<1>(startState) == axisName) {
//					startValue_ = startValue;
//					if (axisValue > startValue + hyst) { // 0 (min) - 65535 (max)
//						selectedGUID = guid;
//						selectedAxis = axisName;
//						positive = true;
//						return true;
//					}
//					if (axisValue < startValue - hyst) { // 65535 (min) - 0 (max)
//						selectedGUID = guid;
//						selectedAxis = axisName;
//						positive = false;
//						return true;
//					}
//				}
//			}
//		}
//	}
//	return false;
//}
//
//void saveStickAxis(const std::string &confTag, GUID devGUID, std::string axis, int min, int max) {
//	std::wstring wDevName = controls.StickControl.FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
//	std::string devName = std::string(wDevName.begin(), wDevName.end());
//	auto index = settings.SteeringAppendDevice(devGUID, devName);
//	settings.StickSaveAxis(confTag, index, axis, min, max);
//	settings.Read(&controls);
//}

// Controller and keyboard
void saveKeyboardKey(std::string confTag, std::string key) {
    settings.KeyboardSaveKey(confTag, key);
    settings.Read(&controls);
    showNotification(("Saved key " + confTag + ": " + key).c_str(), &prevNotification);
}

void saveControllerButton(std::string confTag, std::string button) {
    settings.ControllerSaveButton(confTag, button);
    settings.Read(&controls);
    showNotification(("Saved button " + confTag + ": " + button).c_str(), &prevNotification);
}

void saveLControllerButton(std::string confTag, int button) {
    settings.LControllerSaveButton(confTag, button);
    settings.Read(&controls);
    showNotification(("Saved button " + confTag + ": " + std::to_string(button)).c_str(), &prevNotification);
}

void clearKeyboardKey(std::string confTag) {
    settings.KeyboardSaveKey(confTag, "UNKNOWN");
    settings.Read(&controls);
    showNotification(("Cleared key " + confTag).c_str(), &prevNotification);
}

void clearControllerButton(std::string confTag) {
    settings.ControllerSaveButton(confTag, "UNKNOWN");
    settings.Read(&controls);
    showNotification(("Cleared button " + confTag).c_str(), &prevNotification);
}

void clearLControllerButton(std::string confTag) {
    settings.LControllerSaveButton(confTag, -1);
    settings.Read(&controls);
    showNotification(("Cleared button " + confTag).c_str(), &prevNotification);
}

///////////////////////////////////////////////////////////////////////////////
//                              Config inputs
///////////////////////////////////////////////////////////////////////////////
// Wheel
bool configAxis(std::string str) {

    std::string confTag = str;
    std::string additionalInfo = "Press " + escapeKey + " to exit.";

    if (str == "STEER") {
        additionalInfo += " Steer right to register axis.";
    }
    else if (str == "HANDBRAKE_ANALOG") {
        additionalInfo += " Fully pull and set back handbrake to register axis.";
    }
    else {
        additionalInfo += " Fully press and release the " + confTag + " pedal to register axis.";
    }

    controls.UpdateValues(ScriptControls::InputDevices::Wheel, true);
    // Save current state
    std::vector<std::tuple<GUID, std::string, int>> startStates;
    for (auto guid : controls.WheelControl.GetGuids()) {
        for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
            std::string axisName = controls.WheelControl.DIAxisHelper[i];
            int axisValue = controls.WheelControl.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), guid);
            startStates.push_back(std::tuple<GUID, std::string, int>(guid, axisName, axisValue));
        }
    }

    // Ignore inputs before selection if they moved less than hyst
    int hyst = 65536 / 8;

    // To track direction of physical <-> digital value.
    bool positive = true;

    GUID selectedGUID;
    std::string selectedAxis;
    int startValue;
    int endValue;

    // going down!
    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        controls.UpdateValues(ScriptControls::InputDevices::Wheel, true);
        if (getConfigAxisWithValues(startStates, selectedGUID, selectedAxis, hyst, positive, startValue)) {
            break;
        }
        showSubtitle(additionalInfo);
        WAIT(0);
    }

    // I'll just assume no manufacturer uses inane non-full range steering wheel values
    if (confTag == "STEER") {
        int min = (positive ? 0 : 65535);
        int max = (positive ? 65535 : 0);
        saveAxis(confTag, selectedGUID, selectedAxis, min, max);
        return true;
    }

    int prevAxisValue = controls.WheelControl.GetAxisValue(controls.WheelControl.StringToAxis(selectedAxis), selectedGUID);

    // and up again!
    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        controls.UpdateValues(ScriptControls::InputDevices::Wheel, true);

        int axisValue = controls.WheelControl.GetAxisValue(controls.WheelControl.StringToAxis(selectedAxis), selectedGUID);

        if (positive && axisValue < prevAxisValue) {
            endValue = prevAxisValue;
            break;
        }

        if (!positive && axisValue > prevAxisValue) {
            endValue = prevAxisValue;
            break;
        }

        prevAxisValue = axisValue;

        showSubtitle(additionalInfo);
        WAIT(0);
    }

    int min = startValue;
    int max = endValue;
    saveAxis(confTag, selectedGUID, selectedAxis, min, max);
    return true;
}

bool configWheelToKey() {
    std::string additionalInfo = "Press a button to configure. Press " + escapeKey + " to exit.";

    controls.UpdateValues(ScriptControls::InputDevices::Wheel, true);

    /*
     * 0: Wait for button select
     * 1: Wait for keyboard key select
     * 2: Done!
     */
    int progress = 0;
    GUID selectedGuid{};
    int button = -1;
    std::string keyName;

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        controls.UpdateValues(ScriptControls::InputDevices::Wheel, true);

        if (progress == 0) {
            for (auto guid : controls.WheelControl.GetGuids()) {
                for (int i = 0; i < MAX_RGBBUTTONS; i++) {
                    if (controls.WheelControl.IsButtonJustReleased(i, guid)) {
                        selectedGuid = guid;
                        button = i;
                        progress++;
                    }
                }
                //POV hat
                for (auto d : controls.WheelControl.POVDirections) {
                    if (controls.WheelControl.IsButtonJustReleased(d, guid)) {
                        selectedGuid = guid;
                        button = d;
                        progress++;
                    }
                }
            }
            if (progress == 1) {
                additionalInfo = "Press a keyboard key to configure. Press " + escapeKey + " to exit.";
            }
        }
        if (progress == 1) {
            for (auto key : NativeMenu::KeyMap) {
                if (key.first != "ESC" && IsKeyJustUp(key.second)) {
                    keyName = key.first;
                    progress++;
                }
            }
            for (char letter = 0x30; letter <= 0x5A; letter++) {
                if (letter > 0x39 && letter < 0x41)
                    continue;
                std::string letter_ = std::string(1, letter);
                if (IsKeyJustUp(str2key(letter_))) {
                    keyName = letter_;
                    progress++;
                }
            }
        }
        if (progress >=2 ) {
            addWheelToKey("TO_KEYBOARD", selectedGuid, button, keyName);
            return true;
        }
        showSubtitle(additionalInfo);
        WAIT(0);
    }
}

bool configButton(std::string str) {
    std::string confTag = str;
    std::string additionalInfo = "Press " + escapeKey + " to exit.";
    additionalInfo += " Press a button to set " + confTag + ".";

    controls.UpdateValues(ScriptControls::InputDevices::Wheel, true);

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        controls.UpdateValues(ScriptControls::InputDevices::Wheel, true);

        for (auto guid : controls.WheelControl.GetGuids()) {
            for (int i = 0; i < MAX_RGBBUTTONS; i++) {
                if (controls.WheelControl.IsButtonJustReleased(i, guid)) {
                    saveButton(confTag, guid, i);
                    return true;
                }
            }

            for (auto d : controls.WheelControl.POVDirections) {
                if (controls.WheelControl.IsButtonJustReleased(d, guid)) {
                    saveButton(confTag, guid, d);
                    return true;
                }
            }
        }
        showSubtitle(additionalInfo);
        WAIT(0);
    }
}

bool configHPattern() {
    std::string confTag = "SHIFTER";
    std::string additionalInfo = "Press " + escapeKey + " to exit. Press " + skipKey + " to skip gear.";

    controls.UpdateValues(ScriptControls::InputDevices::Wheel, true);

    GUID devGUID = {};
    std::array<int, NUMGEARS> buttonArray; // There are gears 1-7 + R
    std::fill(buttonArray.begin(), buttonArray.end(), -1);

    int progress = 0;

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        if (IsKeyJustUp(str2key(skipKey))) {
            progress++;
        }

        controls.UpdateValues(ScriptControls::InputDevices::Wheel, true);

        for (auto guid : controls.WheelControl.GetGuids()) {
            for (int i = 0; i < MAX_RGBBUTTONS; i++) {
                // only find unregistered buttons
                if (controls.WheelControl.IsButtonJustPressed(i, guid) &&
                    find(begin(buttonArray), end(buttonArray), i) == end(buttonArray)) {
                    if (progress == 0) { // also save device info when just started
                        devGUID = guid;
                        buttonArray[progress] = i;
                        progress++;
                    }
                    else if (guid == devGUID) { // only accept same device onwards
                        buttonArray[progress] = i;
                        progress++;
                    }
                }
            }
        }


        if (progress > 7) {
            break;
        }
        std::string gearDisplay;
        switch (progress) {
            case 0: gearDisplay = "Reverse"; break;
            case 1: gearDisplay = "1st gear"; break;
            case 2: gearDisplay = "2nd gear"; break;
            case 3: gearDisplay = "3rd gear"; break;
            case 4: case 5: case 6:
            case 7: gearDisplay = std::to_string(progress) + "th gear"; break;
            default: gearDisplay = "?"; break;
        }
        showSubtitle("Shift into " + gearDisplay + ". " + additionalInfo);
        WAIT(0);
    }
    saveHShifter(confTag, devGUID, buttonArray);
    return true;
}

// Keyboard
bool isMenuControl(int control) {
    if (control == menu.GetControls().ControlKeys[NativeMenu::MenuControls::ControlType::MenuKey] ||
        control == menu.GetControls().ControlKeys[NativeMenu::MenuControls::ControlType::MenuSelect] ||
        control == menu.GetControls().ControlKeys[NativeMenu::MenuControls::ControlType::MenuCancel]) {
        return true;
    }
    return false;
}

bool configKeyboardKey(const std::string &confTag) {
    std::string additionalInfo = "Press " + escapeKey + " to exit.";
    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        for (auto k : NativeMenu::KeyMap) {
            if (isMenuControl(k.second)) {
                showNotification("Can't use menu controls!", &prevNotification);
                continue;
            }
            if (IsKeyJustUp(k.second)) {
                saveKeyboardKey(confTag, k.first);
                return true;
            }
        }
        for (char letter = 0x30; letter <= 0x5A; letter++) {
            if (letter > 0x39 && letter < 0x41)
                continue;
            std::string letter_ = std::string(1, letter);
            if (isMenuControl(str2key(letter_))) {
                showNotification("Can't use menu controls!", &prevNotification);
                continue;
            }
            if (IsKeyJustUp(str2key(letter_))) {
                saveKeyboardKey(confTag, letter_);
                return true;
            }
        }

        showSubtitle("Press " + confTag + ". Menu controls can't be chosen." + additionalInfo);
        WAIT(0);
    }
}



// Controller
bool configControllerButton(const std::string &confTag) {
    std::string additionalInfo = "Press " + escapeKey + " to exit.";
    XboxController* rawController = controls.GetRawController();
    if (rawController == nullptr)
        return false;

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        controls.UpdateValues(ScriptControls::InputDevices::Controller, true);
        for (std::string buttonHelper : rawController->XboxButtonsHelper) {
            auto button = rawController->StringToButton(buttonHelper);
            if (rawController->IsButtonJustPressed(button, controls.GetButtonState())) {
                saveControllerButton(confTag, buttonHelper);
                return true;
            }
        }
        showSubtitle("Press " + confTag + ". " + additionalInfo);
        WAIT(0);
    }
}

bool configLControllerButton(const std::string &confTag) {
    std::string additionalInfo = "Press " + escapeKey + " to exit.";

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        controls.UpdateValues(ScriptControls::InputDevices::Controller, true);
        
        for (auto mapItem : controls.LegacyControlsMap) {
            if (CONTROLS::IS_DISABLED_CONTROL_JUST_PRESSED(0, mapItem.second)) {
                saveLControllerButton(confTag, mapItem.second);
                return true;
            }
        }

        showSubtitle("Press " + confTag + ". " + additionalInfo);
        WAIT(0);
    }
}

/*
bool configStickAxis(std::string str) {

    std::string confTag = str;
    std::string additionalInfo = "Press " + escapeKey + " to exit.";

    additionalInfo += " Fully press and release the " + confTag + " to register axis.";

    controls.UpdateValues(ScriptControls::InputDevices::Stick, false, true);
    // Save current state
    std::vector<std::tuple<GUID, std::string, int>> startStates;
    for (auto guid : controls.StickControl.GetGUIDs()) {
        for (int i = 0; i < GenDInput::SIZEOF_DIAxis - 1; i++) {
            std::string axisName = controls.StickControl.DIAxisHelper[i];
            int axisValue = controls.StickControl.GetAxisValue(guid, static_cast<GenDInput::DIAxis>(i));
            startStates.push_back(std::tuple<GUID, std::string, int>(guid, axisName, axisValue));
        }
    }

    // Ignore inputs before selection if they moved less than hyst
    int hyst = 65536 / 8;

    // To track direction of physical <-> digital value.
    bool positive = true;

    GUID selectedGUID;
    std::string selectedAxis;
    int startValue;
    int endValue;

    // going down!
    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        controls.UpdateValues(ScriptControls::InputDevices::Stick, false, true);
        if (getConfigAxisStickWithValues(startStates, selectedGUID, selectedAxis, hyst, positive, startValue)) {
            break;
        }
        showSubtitle(additionalInfo);
        WAIT(0);
    }

    int prevAxisValue = controls.StickControl.GetAxisValue(selectedGUID, controls.StickControl.StringToAxis(selectedAxis));

    // and up again!
    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        controls.UpdateValues(ScriptControls::InputDevices::Stick, false, true);

        int axisValue = controls.StickControl.GetAxisValue(selectedGUID, controls.StickControl.StringToAxis(selectedAxis));

        if (positive && axisValue < prevAxisValue) {
            endValue = prevAxisValue;
            break;
        }

        if (!positive && axisValue > prevAxisValue) {
            endValue = prevAxisValue;
            break;
        }

        prevAxisValue = axisValue;

        showSubtitle(additionalInfo);
        WAIT(0);
    }

    int min = startValue;
    int max = endValue;
    saveStickAxis(confTag, selectedGUID, selectedAxis, min, max);
    return true;
}
*/
