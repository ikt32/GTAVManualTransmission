#include "script.h"

#include "GitInfo.h"
#include "InputConfiguration.h"
#include "ScriptMenuUtils.h"
#include "ScriptSettings.hpp"

#include "Constants.h"
#include "UpdateChecker.h"
#include "Compatibility.h"

#include "Input/CarControls.hpp"
#include "Input/NativeController.h"
#include "Input/keyboard.h"

#include "Util/UIUtils.h"
#include "Util/Strings.hpp"
#include "Util/GUID.h"
#include "Util/MathExt.h"
#include "Util/Logger.hpp"
#include "Util/ScriptUtils.h"
#include "Util/AddonSpawnerCache.h"
#include "Util/Paths.h"

#include "Memory/MemoryPatcher.hpp"
#include "Memory/VehicleExtensions.hpp"

#include "VehicleConfig.h"
#include "SteeringAnim.h"
#include "BlockableControls.h"

#include "AWD.h"
#include "DrivingAssists.h"
#include "CruiseControl.h"
#include "WheelInput.h"

#include <menu.h>
#include <inc/main.h>
#include <inc/natives.h>

#include <fmt/format.h>

#include <Windows.h>
#include <shellapi.h>
#include <string>
#include <mutex>
#include <filesystem>

using VExt = VehicleExtensions;

extern ReleaseInfo g_releaseInfo;
extern std::mutex g_releaseInfoMutex;

extern bool g_notifyUpdate;
extern std::mutex g_notifyUpdateMutex;

extern bool g_checkUpdateDone;
extern std::mutex g_checkUpdateDoneMutex;

extern NativeMenu::Menu g_menu;
extern CarControls g_controls;
extern ScriptSettings g_settings;

extern std::vector<VehicleConfig> g_vehConfigs;

struct SFont {
    int ID;
    std::string Name;
};

struct STagInfo {
    std::string Tag;
    std::string Info;
};

namespace {
    const std::string escapeKey = "BACKSPACE";
    const std::string skipKey = "RIGHT";
    const std::string modUrl = "https://www.gta5-mods.com/scripts/manual-transmission-ikt";

    const std::vector<std::string> gearboxModes = {
        "Sequential",
        "H-pattern",
        "Automatic"
    };

    const std::vector<SFont> fonts {
        { 0, "Chalet London" },
        { 1, "Sign Painter" },
        { 2, "Slab Serif" },
        { 4, "Chalet Cologne" },
        { 7, "Pricedown" },
    };

    const std::vector<std::string> speedoTypes {
        "off",
        "kph",
        "mph",
        "ms"
    };

    const std::vector<std::string> tcsStrings{
        "Brakes", "Throttle"
    };

    const std::vector<std::string> notifyLevelStrings{
        "Debug",
        "Info",
        "UI",
        "None"
    };

    const std::vector<std::string> camAttachPoints {
        "Player head",
        "Vehicle (1)",
        "Vehicle (2)"
    };

    const std::vector<std::string> ffbCurveTypes{
        "Boosted",
        "Gamma/Linear",
    };

    const std::vector<std::string> PitchModeNames {
        "Horizon",
        "Vehicle",
        "Dynamic"
    };

    std::vector<std::string> diDevicesInfo{ "Press Enter to refresh." };

    std::string MenuSubtitleConfig() {
        std::string cfgName = "Base";
        if (g_settings.ConfigActive())
            cfgName = g_settings().Name;
        return fmt::format("CFG: [{}]", cfgName);
    }

    std::string FormatDeviceGuidName(GUID guid) {
        std::string deviceNameCfg = g_settings.GUIDToDeviceName(guid);
        DirectInputDeviceInfo* deviceEntry = g_controls.GetWheel().GetDeviceInfo(guid);
        int index = g_settings.GUIDToDeviceIndex(guid);
        if (deviceEntry) {
            return fmt::format("[{}] {}", index, deviceNameCfg);
        }
        else {
            return fmt::format("[{}] {} (Disconnected)", index, deviceNameCfg);
        }
    }

    void incVal(float& value, float max, float step) {
        if (value + step > max) {
            value = max;
            return;
        }
        value += step;
    }

    void decVal(float& value, float min, float step) {
        if (value - step < min) {
            value = min;
            return;
        }
        value -= step;
    }
}

///////////////////////////////////////////////////////////////////////////////
//                             Menu stuff
///////////////////////////////////////////////////////////////////////////////
void onMenuInit() {
    g_menu.ReadSettings();
}

void saveAllSettings() {
    g_settings.SaveGeneral();
    g_settings.SaveController(&g_controls);
    g_settings.SaveWheel();
}

void onMenuClose() {
    saveAllSettings();
    loadConfigs();
}

void update_mainmenu() {
    g_menu.Title("Manual Transmission");
    g_menu.Subtitle(fmt::format("~b~{}{}", Constants::DisplayVersion, GIT_DIFF));

    if (Paths::GetModPathChanged()) {
        g_menu.Option("Warning: Mod path moved!", NativeMenu::solidRed,
            { "Script failed writing log file in the original location.",
              "Entire ManualTransmission folder moved to AppData folder.",
              fmt::format("Old path: {}", Paths::GetInitialModPath()),
              fmt::format("New path: {}", Paths::GetModPath()) });
    }

    if (g_settings.Error()) {
        g_menu.Option("Settings load/save error", NativeMenu::solidRed,
            { "Script failed to read or write the settings file(s).",
              "This may be caused by the ManualTransmission folder and the files within being marked as read-only.",
              "Deselect read-only on the folder properties to try and fix it." });
    }

    if (logger.Error()) {
        g_menu.Option("Logging error", NativeMenu::solidRed,
            { "Script failed to write to the log file.",
              "This may be caused by the ManualTransmission folder and the files within being marked as read-only.",
              "Deselect read-only on the folder properties to try and fix it." });
    }

    if (MemoryPatcher::Error) {
        g_menu.Option("Patch test error", NativeMenu::solidRed, 
            { "One or more components failed to patch. Mod behavior is uncertain.",
              "Usually caused by a game update, or an incompatible version.", 
              "Check Gears.log for more details." });
    }

    {
        std::unique_lock releaseInfoLock(g_releaseInfoMutex, std::try_to_lock);
        std::unique_lock notifyUpdateLock(g_notifyUpdateMutex, std::try_to_lock);
        if (notifyUpdateLock.owns_lock() && releaseInfoLock.owns_lock()) {
            if (g_notifyUpdate) {
                std::vector<std::string> bodyLines =
                    NativeMenu::split(g_releaseInfo.Body, '\n');

                std::vector<std::string> extra = {
                    fmt::format("New version: {}", g_releaseInfo.Version.c_str()),
                    fmt::format("Release date: {}", g_releaseInfo.TimestampPublished.c_str()),
                    "Changelog:"
                };

                for (const auto& line : bodyLines) {
                    extra.push_back(line);
                }

                if (g_menu.OptionPlus("New update available!", extra, nullptr, [] {
                    g_settings.Update.IgnoredVersion = g_releaseInfo.Version;
                    g_notifyUpdate = false;
                    saveAllSettings();
                    }, nullptr, "Update info",
                    { "Press Select/Enter to check GTA5-Mods.com.",
                        "Press right to ignore the current update." })) {
                    WAIT(20);
                    PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, ControlFrontendPause, 1.0f);
                    ShellExecuteA(0, 0, modUrl.c_str(), 0, 0, SW_SHOW);
                }
            }
        }
    }

    bool tempEnableRead = g_settings.MTOptions.Enable;
    if (g_menu.BoolOption("Enable Manual Transmission", tempEnableRead,
        { "Enable or disable the manual transmission. Steering wheel stays active." })) {
        toggleManual(!g_settings.MTOptions.Enable);
    }

    int tempShiftMode = static_cast<int>(g_settings().MTOptions.ShiftMode);

    std::vector<std::string> detailsTemp {
        "Choose your gearbox! Options are Sequential, H-pattern and Automatic."
    };

    if (g_settings.ConfigActive()) {
        detailsTemp.push_back(fmt::format("CFG: [{}]", g_settings().Name));
    }

    g_menu.StringArray("Gearbox", gearboxModes, tempShiftMode,
        detailsTemp);

    if (tempShiftMode != static_cast<int>(g_settings().MTOptions.ShiftMode)) {
        setShiftMode(static_cast<EShiftMode>(tempShiftMode));
    }

    g_menu.MenuOption("Manual Transmission settings", "settingsmenu", 
        { "Choose what parts of the Manual Transmission script are active." });

    g_menu.MenuOption("Controls", "controlsmenu", 
        { "Configure controls for the script." });

    g_menu.MenuOption("Driving assists", "driveassistmenu",
        { "ABS, TCS and ESC assist options for the player." });

    g_menu.MenuOption("Gameplay assists", "gameassistmenu",
        { "Assist to make playing a bit easier." });

    g_menu.MenuOption("Misc options", "miscoptionsmenu",
        { "Various features that don't fit in the other categories." });

    g_menu.MenuOption("HUD options", "hudmenu",
        { "Change and move HUD elements." });

    g_menu.MenuOption("Developer options", "devoptionsmenu", 
        { "Various settings for debugging, compatibility, etc." });

    if (g_settings.Debug.DisableInputDetect) {
        int activeIndex = g_controls.PrevInput;
        std::vector<std::string> inputNames {
            "Keyboard", "Controller", "Wheel"
        };
        if (g_menu.StringArray("Active input", inputNames, activeIndex, { "Active input is set manually." })) {
            g_controls.PrevInput = static_cast<CarControls::InputDevices>(activeIndex);
        }
    }
    else {
        int activeIndex = 0;
        std::string activeInputName;
        switch (g_controls.PrevInput) {
        case CarControls::Keyboard:
            activeInputName = "Keyboard";
            break;
        case CarControls::Controller:
            activeInputName = "Controller";
            break;
        case CarControls::Wheel:
            activeInputName = "Wheel";
            break;
        }
        g_menu.StringArray("Active input", { activeInputName }, activeIndex, 
            { "Active input is automatically detected when throttle, brake or clutch is pressed on a configured device." });
    }
}

void update_settingsmenu() {
    g_menu.Title("Manual Transmission");
    g_menu.Subtitle("Settings");

    g_menu.MenuOption("Features", "featuresmenu",
        { "Turn on or off parts of Manual Transmission." });

    g_menu.MenuOption("Finetuning", "finetuneoptionsmenu",
        { "Fine-tune the parameters above." });

    g_menu.MenuOption("Shifting options", "shiftingoptionsmenu",
        { "Change the shifting behavior for sequential and automatic modes." } );

    g_menu.MenuOption("Automatic finetuning", "finetuneautooptionsmenu",
        { "Fine-tune script-provided automatic transmission parameters." });

    g_menu.MenuOption("Vehicle configurations", "vehconfigmenu",
        { "Vehicle configurations override the base mod settings, for specific vehicles. This allows selectively "
          "enabling assists or per-vehicle tweaked finetuning options.",
          "\"CFG: [...]\" in a menu subtitle or setting "
          "description indicates that the settings you change are vehicle-specific."});
}

void update_featuresmenu() {
    g_menu.Title("Features");

    if (g_settings.ConfigActive())
        g_menu.Subtitle(fmt::format("CFG: [{} (Partial)]", g_settings().Name));
    else
        g_menu.Subtitle(MenuSubtitleConfig());

    g_menu.BoolOption("Engine Damage", g_settings.MTOptions.EngDamage,
        { "Damage the engine when over-revving and when mis-shifting." });

    g_menu.BoolOption("Engine stalling (H)", g_settings.MTOptions.EngStallH,
        { "Stall the engine when the wheel speed gets too low. Applies to H-pattern shift mode." });

    g_menu.BoolOption("Engine stalling (S)", g_settings.MTOptions.EngStallS,
        { "Stall the engine when the wheel speed gets too low. Applies to sequential shift mode." });

    g_menu.BoolOption("Engine braking", g_settings.MTOptions.EngBrake,
        { "Help the car braking by slowing down more at high RPMs." });

    g_menu.BoolOption("Gear/RPM lockup", g_settings.MTOptions.EngLock,
        { "Simulate wheel lock-up when mis-shifting to a too low gear for the RPM range." });

    g_menu.BoolOption("Final gear RPM limit", g_settings.MTOptions.FinalGearRPMLimit,
        { "Limit accelerating beyond engine redline in top gear, or reverse." });

    g_menu.BoolOption("Enable speed limiter", g_settings().MTOptions.SpeedLimiter.Enable,
        { "Electronically limit speed." });

    g_menu.MenuOption("Speed limiter settings", "speedlimitersettingsmenu");

    g_menu.BoolOption("Clutch shift (H)", g_settings().MTOptions.ClutchShiftH,
        { "Require holding the clutch to shift in H-pattern shift mode. (Vehicle specific)" });

    g_menu.BoolOption("Clutch shift (S)", g_settings().MTOptions.ClutchShiftS,
        { "Require holding the clutch to shift in sequential mode. (Vehicle specific)" });

    g_menu.BoolOption("Clutch creep", g_settings().MTOptions.ClutchCreep,
        { "Simulate clutch creep when stopped with clutch engaged. (Vehicle specific)" });
}

void update_finetuneoptionsmenu() {
    g_menu.Title("Finetuning");
    g_menu.Subtitle(MenuSubtitleConfig());

    g_menu.FloatOption("Clutch bite threshold", g_settings().MTParams.ClutchThreshold, 0.0f, 1.0f, 0.05f,
        { "How far the clutch has to be lifted to start biting." });
    g_menu.FloatOption("Stalling RPM", g_settings().MTParams.StallingRPM, 0.0f, 0.2f, 0.01f,
        { "Consider stalling when the expected RPM drops below this value.",
          "The range is 0.0 to 0.2. The engine idles at 0.2 in GTA.",
          "Expected RPM is based on car speed and current gear ratio."});
    g_menu.FloatOption("Stalling rate", g_settings().MTParams.StallingRate, 0.0f, 10.0f, 0.05f,
        { "How quick the engine should stall. Higher values make it stall faster." });
    g_menu.FloatOption("Stalling slip", g_settings().MTParams.StallingSlip, 0.0f, 1.0f, 0.05f,
        { "How much the clutch may slip before the stalling rate picks up." });

    g_menu.FloatOption("RPM damage", g_settings().MTParams.RPMDamage, 0.0f, 10.0f, 0.05f,
        { "Damage from redlining too long." });
    g_menu.FloatOption("Misshift damage", g_settings().MTParams.MisshiftDamage, 0, 100, 5,
        { "Damage from being over the rev range." });
    g_menu.FloatOption("Engine braking threshold", g_settings().MTParams.EngBrakeThreshold, 0.0f, 1.0f, 0.05f,
        { "RPM where engine braking starts being effective." });
    g_menu.FloatOption("Engine braking power", g_settings().MTParams.EngBrakePower, 0.0f, 5.0f, 0.05f,
        { "Decrease this value if your wheels lock up when engine braking." });

    // Clutch creep params
    g_menu.FloatOption("Clutch creep idle RPM", g_settings().MTParams.CreepIdleRPM, 0.0f, 0.5f, 0.01f,
        { "RPM at which the engine should be idling and the car wants to move by itself.",
          "Engines in GTA idle at 0.2, but this is rather high."});
    g_menu.FloatOption("Clutch creep throttle", g_settings().MTParams.CreepIdleThrottle, 0.0f, 1.0f, 0.01f,
        { "How much throttle is given when the car speed drops below the idle RPM." });
}

void update_shiftingoptionsmenu() {
    g_menu.Title("Shifting options");
    g_menu.Subtitle(MenuSubtitleConfig());

    g_menu.BoolOption("Cut throttle on upshift", g_settings().ShiftOptions.UpshiftCut,
        { "Helps rev matching.",
            "Only applies to sequential mode."});
    g_menu.BoolOption("Blip throttle on downshift", g_settings().ShiftOptions.DownshiftBlip,
        { "Helps rev matching.",
            "Only applies to sequential mode." });
    g_menu.BoolOption("Downshift protection", g_settings().ShiftOptions.DownshiftProtect,
        { "Prevents downshifting causing overrevving. Only applies to sequential mode.",
            "Customize protection warning in the HUD options." });
    g_menu.FloatOption("Clutch rate multiplier", g_settings().ShiftOptions.ClutchRateMult, 0.05f, 20.0f, 0.05f,
        { "Change how fast clutching is. Below 1 is slower, higher than 1 is faster.",
            "Applies to sequential and automatic mode." });

    g_menu.FloatOption("Shifting RPM tolerance", g_settings().ShiftOptions.RPMTolerance, 0.0f, 1.0f, 0.05f,
        { "RPM mismatch tolerance on shifts",
            "Only applies to H-pattern with \"Clutch Shift\" enabled.",
            fmt::format("Clutch Shift (H) is {}abled.", g_settings().MTOptions.ClutchShiftH ? "~g~en" : "~r~dis")
        });
}

void update_finetuneautooptionsmenu() {
    g_menu.Title("Automatic finetuning");
    g_menu.Subtitle(MenuSubtitleConfig());

    g_menu.FloatOption("Upshift engine load", g_settings().AutoParams.UpshiftLoad, 0.01f, 0.20f, 0.01f,
        { "Upshift when the engine load drops below this value. "
          "Raise this value if the car can't upshift."});
    g_menu.FloatOption("Upshift timeout multiplier", g_settings().AutoParams.UpshiftTimeoutMult, 0.00f, 10.00f, 0.05f,
        { "Don't upshift too quickly. "
          "Timeout based on clutch change rate.",
          "Raise for longer timeout, lower to allow upshifts following up eachother quicker. 0 to disable." });
    g_menu.FloatOption("Downshift engine load", g_settings().AutoParams.DownshiftLoad, 0.30f, 1.00f, 0.01f,
        { "Downshift when the engine load rises over this value. "
          "Raise this value if the car downshifts right after upshifting." });
    g_menu.FloatOption("Downshift timeout multiplier", g_settings().AutoParams.DownshiftTimeoutMult, 0.05f, 10.00f, 0.05f,
        { "Don't downshift while car has just shifted up. "
          "Timeout based on clutch change rate.",
          "Raise for longer timeout, lower to allow earlier downshifting after an upshift." });
    g_menu.FloatOption("Next gear min RPM",    g_settings().AutoParams.NextGearMinRPM, 0.20f, 0.50f, 0.01f, 
        { "Don't upshift until next gears' RPM is over this value." });
    g_menu.FloatOption("Current gear min RPM", g_settings().AutoParams.CurrGearMinRPM, 0.20f, 0.50f, 0.01f, 
        { "Downshift when RPM drops below this value." });
    g_menu.FloatOption("Economy rate", g_settings().AutoParams.EcoRate, 0.01f, 0.50f, 0.01f,
        { "On releasing throttle, high values cause earlier upshifts.",
          "Set this low to stay in gear longer when releasing throttle." });
    g_menu.BoolOption("Use ATCU (experimental)", g_settings().AutoParams.UsingATCU,
        { "Using experimental new Automatic Transmission Control Unit by Nyconing.",
          "ATCU is configurationless, the above settings are ignored." });
}

void update_vehconfigmenu() {
    g_menu.Title("Custom vehicle settings");
    g_menu.Subtitle(MenuSubtitleConfig());

    if (g_menu.Option("Create configuration...", 
        { "Create a new, empty current configuration file.",
          "Changes made within a configuration are saved to that configuration only.",
          "The submenu subtitles indicate which configuration is being edited." })) {
        SaveVehicleConfig();
    }

    if (g_menu.Option("Reload configurations", 
        { "Reload to update manual changes in the Vehicles folder." })) {
        loadConfigs();
    }

    if (g_settings.ConfigActive()) {
        bool sel = false;
        g_menu.OptionPlus(fmt::format("Active configuration: [{}]", g_settings().Name), {}, &sel, nullptr, nullptr, "",
            { "Currently using this configuration. Any changes made are saved into the current configuration.",
              "When multiple configurations work with the same model, the configuration that comes first "
              "alphabetically is used." });
        if (sel) {
            auto extras = FormatVehicleConfig(g_settings(), gearboxModes);
            g_menu.OptionPlusPlus(extras, "Configuration overview");
        }
    }
    else {
        g_menu.Option("No active configuration");
    }

    g_menu.BoolOption("Save entire configuration", g_settings.Misc.SaveFullConfig,
        { "Enabled: Writes the full configuration for all configurations.",
            "Disabled: Only write difference between specific and base configuration." });

    for (const auto& vehConfig : g_vehConfigs) {
        bool sel = false;
        std::vector<std::string> descr = {
            "This config will activate if you get into a compatible vehicle.",
            "First matches by model & plate, then just checks model."
        };
        g_menu.OptionPlus(vehConfig.Name, {}, &sel, nullptr, nullptr, "", descr);
        if (sel) {
            auto extras = FormatVehicleConfig(vehConfig, gearboxModes);
            g_menu.OptionPlusPlus(extras, "Configuration overview");
        }
    }

    if (g_vehConfigs.empty()) {
        g_menu.OptionPlus("No configurations found", {}, nullptr, nullptr, nullptr, "Instructions", {
            "Overrides allow you to specify custom rules for specific vehicles, that override the global settings.",
            "For example, you can set different default shift modes, set which assists are active, or use different force feedback profiles for each car.",
            R"(Check the "Vehicles" folder inside the "ManualTransmission folder for more information!")"
            });
    }
}


void update_controlsmenu() {
    g_menu.Title("Controls");
    g_menu.Subtitle("");

    g_menu.MenuOption("Controller", "controllermenu",
        { "Modify bindings and adjustments for controller input." });

    g_menu.MenuOption("Keyboard", "keyboardmenu",
        { "Modify bindings for keyboard input." });

    g_menu.MenuOption("Wheel & pedals", "wheelmenu",
        { "Create and modify bindings and adjustments for steering wheels and pedals." });

    g_menu.MenuOption("Steering assists", "steeringassistmenu",
        { "Customize steering input for keyboards and controllers." });

    g_menu.MenuOption("Vehicle specific", "controlsvehconfmenu",
        { "Override and adjust input behavior for specific vehicle configurations." });
}

void update_controlsvehconfmenu() {
    g_menu.Title("Vehicle specific");
    g_menu.Subtitle(MenuSubtitleConfig());

    g_menu.BoolOption("ES: Override wheel rotation", g_settings().Steering.CustomSteering.UseCustomLock,
        { "Enhanced Steering",
          "Override the default 180 degree steering wheel rotation." });

    g_menu.FloatOptionCb("ES: Wheel rotation", g_settings().Steering.CustomSteering.SoftLock, 180.0f, 2880.0f, 30.0f, GetKbEntryFloat,
        { "Enhanced Steering",
          "Degrees of rotation for the vehicle steering wheel. Does not change max steering angle." });

    g_menu.FloatOptionCb("ES: Steering multiplier", g_settings().Steering.CustomSteering.SteeringMult, 0.01f, 2.0f, 0.01f, GetKbEntryFloat,
        { "Enhanced Steering",
          "Increase or decrease actual max steering angle." });

    g_menu.FloatOptionCb("ES: Steering reduction", g_settings().Steering.CustomSteering.SteeringReduction, 0.0f, 2.0f, 0.01f, GetKbEntryFloat,
        { "Enhanced Steering",
          "From InfamousSabre's Custom Steering.",
          "Reduce steering input at higher speeds.",
          "Increase for vehicles with low optimum slip angle.",
          "Decrease for vehicles with high optimum slip angle."});

    g_menu.FloatOptionCb("Wheel: FFB SAT mult", g_settings().Steering.Wheel.SATMult, 0.05f, 10.0f, 0.05f, GetKbEntryFloat,
        { "Wheel",
          "Vehicle-specific FFB SAT adjustment. Same comments apply as in the FFB section.",
          "Increase to make steering heavier, decrease to make it lighter." });

    bool showDynamicFfbCurveBox = false;
    if (g_menu.OptionPlus(fmt::format("Wheel: FFB response curve mult: < {:.2f} >", g_settings().Steering.Wheel.CurveMult), {}, &showDynamicFfbCurveBox,
        [=] { return incVal(g_settings().Steering.Wheel.CurveMult, 5.00f, 0.01f); },
        [=] { return decVal(g_settings().Steering.Wheel.CurveMult, 0.01f, 0.01f); },
        "Response curve", {
            "Wheel",
            "Vehicle-specific FFB curve adjustment. Same comments apply as in the FFB section.",
            "Increase to make response more linear, decrease to make it quicker." })) {
        float val = g_settings.Wheel.FFB.ResponseCurve;
        if (GetKbEntryFloat(val)) {
            g_settings.Wheel.FFB.ResponseCurve = val;
        }
    }

    if (showDynamicFfbCurveBox) {
        auto extras = ShowDynamicFfbCurve(abs(WheelInput::GetFFBConstantForce()),
            g_settings.Wheel.FFB.ResponseCurve * g_settings().Steering.Wheel.CurveMult,
            g_settings.Wheel.FFB.FFBProfile);
        g_menu.OptionPlusPlus(extras, "Response Curve");
    }

    g_menu.FloatOptionCb("Wheel: Soft lock", g_settings().Steering.Wheel.SoftLock, 180.0f, 2880.0f, 30.0f, GetKbEntryFloat,
        { "Steering range for your real wheel, in degrees. Reduce for quicker steering." });

    g_menu.FloatOptionCb("Wheel: Steering multiplier", g_settings().Steering.Wheel.SteeringMult, 0.1f, 2.0f, 0.01f, GetKbEntryFloat,
        { "Increase or decrease actual steering lock. Increase for faster steering and more angle." });
}

void update_controllermenu() {
    g_menu.Title("Controller");
    g_menu.Subtitle("");

    g_menu.BoolOption("Native input", g_settings.Controller.Native.Enable,
        { "Using a controller not supported by XInput? Enable this option and re-bind your controls." });

    if (g_settings.Controller.Native.Enable) {
        g_menu.MenuOption("Controller bindings (Native)", "controllerbindingsnativemenu",
            { "Set up controller bindings." });
    }
    else {
        g_menu.MenuOption("Controller bindings (XInput)", "controllerbindingsxinputmenu",
            { "Set up controller bindings." });
    }

    g_menu.BoolOption("Engine button toggles", g_settings.Controller.ToggleEngine,
        { "Checked: the engine button turns the engine on AND off.",
            "Not checked: the button only turns the engine on when it's off." });

    g_menu.IntOption("Long press time (ms)", g_settings.Controller.HoldTimeMs, 100, 5000, 50,
        { "Timeout for long press buttons to activate." });

    g_menu.IntOption("Max tap time (ms)", g_settings.Controller.MaxTapTimeMs, 50, 1000, 10,
        { "Buttons pressed and released within this time are regarded as a tap. Shift up, shift down are tap controls." });

    g_menu.FloatOption("Trigger value", g_settings.Controller.TriggerValue, 0.25, 1.0, 0.05,
        { "Threshold for an analog input to be detected as button press." });

    g_menu.BoolOption("Block car controls", g_settings.Controller.BlockCarControls,
        { "Blocks car action controls. Holding activates the original button again.",
            "Experimental!" });

    g_menu.BoolOption("Block H-pattern on controller", g_settings.Controller.BlockHShift,
        { "Block H-pattern mode when using controller input." });

    g_menu.BoolOption("Ignore shifts in UI", g_settings.Controller.IgnoreShiftsUI,
        { "Ignore shift up/shift down while using the phone or when the menu is open" });

    if (!g_settings.Controller.Native.Enable) {
        g_menu.BoolOption("Custom XInput deadzones", g_settings.Controller.CustomDeadzone,
            { "Use custom deadzones for XInput controller joysticks. Affects enhanced steering (when using XInput)." });

        if (g_settings.Controller.CustomDeadzone) {
            float ljVal = static_cast<float>(g_settings.Controller.DeadzoneLeftThumb);
            if (g_menu.FloatOptionCb("Deadzone left thumb", ljVal, 0.0f, 32767.0f, 1.0f, GetKbEntryFloat,
                { "Deadzone for left thumb joystick. Works for XInput mode only!",
                  fmt::format("Default value: {}", XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) })) {
                g_settings.Controller.DeadzoneLeftThumb = static_cast<int>(ljVal);
            }

            float rjVal = static_cast<float>(g_settings.Controller.DeadzoneRightThumb);
            if (g_menu.FloatOptionCb("Deadzone right thumb", rjVal, 0.0f, 32767.0f, 1.0f, GetKbEntryFloat,
                { "Deadzone for right thumb joystick. Works for XInput mode only!",
                  fmt::format("Default value: {}", XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) })) {
                g_settings.Controller.DeadzoneRightThumb = static_cast<int>(rjVal);
            }
        }
    }
}

void update_controllerbindingsnativemenu() {
    g_menu.Title("Controller bindings");
    g_menu.Subtitle("Native mode");

    std::vector<std::string> blockableControlsHelp;
    const auto& blockableControls = BlockableControls::GetList();
    blockableControlsHelp.reserve(blockableControls.size());
    for (const auto& control : blockableControls) {
        blockableControlsHelp.emplace_back(control.Text);
    }

    int oldIndexUp = BlockableControls::GetIndex(g_controls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)]);
    if (g_menu.StringArray("Shift Up blocks", blockableControlsHelp, oldIndexUp)) {
        g_controls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)] = blockableControls[oldIndexUp].Control;
    }

    int oldIndexDown = BlockableControls::GetIndex(g_controls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)]);
    if (g_menu.StringArray("Shift Down blocks", blockableControlsHelp, oldIndexDown)) {
        g_controls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)] = blockableControls[oldIndexDown].Control;
    }

    int oldIndexClutch = BlockableControls::GetIndex(g_controls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)]);
    if (g_menu.StringArray("Clutch blocks", blockableControlsHelp, oldIndexClutch)) {
        g_controls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)] = blockableControls[oldIndexClutch].Control;
    }

    std::vector<std::string> controllerInfo = {
        "Press RIGHT to clear key",
        "Press RETURN to configure button",
        "",
    };

    for (const auto& input : g_controls.LegacyControls) {
        controllerInfo.back() = input.Description;
        controllerInfo.push_back(fmt::format("Assigned to {} ({})", NativeController::GetControlName(input.Control), input.Control));

        if (g_menu.OptionPlus(fmt::format("Assign {}", input.Name), controllerInfo, nullptr, std::bind(clearLControllerButton, input.ConfigTag), nullptr, "Current setting")) {
            WAIT(500);
            bool result = configLControllerButton(input.ConfigTag);
            if (!result)
                UI::Notify(WARN, fmt::format("Cancelled {} assignment", input.Name));
            WAIT(500);
        }
        controllerInfo.pop_back();
    }
}

void update_controllerbindingsxinputmenu() {
    g_menu.Title("Controller bindings");
    g_menu.Subtitle("XInput mode");

    std::vector<std::string> blockableControlsHelp;
    const auto& blockableControls = BlockableControls::GetList();
    blockableControlsHelp.reserve(blockableControls.size());
    for (const auto& control : blockableControls) {
        blockableControlsHelp.emplace_back(control.Text);
    }

    int oldIndexUp = BlockableControls::GetIndex(g_controls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftUp)]);
    if (g_menu.StringArray("Shift Up blocks", blockableControlsHelp, oldIndexUp)) {
        g_controls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftUp)] = blockableControls[oldIndexUp].Control;
    }

    int oldIndexDown = BlockableControls::GetIndex(g_controls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftDown)]);
    if (g_menu.StringArray("Shift Down blocks", blockableControlsHelp, oldIndexDown)) {
        g_controls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftDown)] = blockableControls[oldIndexDown].Control;
    }

    int oldIndexClutch = BlockableControls::GetIndex(g_controls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::Clutch)]);
    if (g_menu.StringArray("Clutch blocks", blockableControlsHelp, oldIndexClutch)) {
        g_controls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::Clutch)] = blockableControls[oldIndexClutch].Control;
    }

    std::vector<std::string> controllerInfo = {
        "Press RIGHT to clear key",
        "Press RETURN to configure button",
        "",
    };

    for (const auto& input : g_controls.ControlXbox) {
        controllerInfo.back() = input.Description;
        controllerInfo.push_back(fmt::format("Assigned to {}", input.Control));
        if (g_menu.OptionPlus(fmt::format("Assign {}", input.Name), controllerInfo, nullptr, std::bind(clearControllerButton, input.ConfigTag), nullptr, "Current setting")) {
            WAIT(500);
            bool result = configControllerButton(input.ConfigTag);
            if (!result)
                UI::Notify(WARN, fmt::format("Cancelled {} assignment", input.Name));
            WAIT(500);
        }
        controllerInfo.pop_back();
    }
}

void update_keyboardmenu() {
    g_menu.Title("Keyboard bindings");
    g_menu.Subtitle("");

    std::vector<std::string> keyboardInfo = {
        "Press RIGHT to clear key",
        "Active inputs:",
    };

    for (uint32_t i = 0; i < static_cast<uint32_t>(CarControls::KeyboardControlType::SIZEOF_KeyboardControlType); ++i) {
        const auto& input = g_controls.KBControl[i];

        if (g_controls.IsKeyPressed(input.Control))
            keyboardInfo.emplace_back(input.Name);
    }
    if (keyboardInfo.size() == 2)
        keyboardInfo.emplace_back("None");

    for (const auto& input : g_controls.KBControl) {
        keyboardInfo.push_back(fmt::format("Assigned to {}", GetNameFromKey(input.Control)));
        if (g_menu.OptionPlus(fmt::format("Assign {}", input.Name), keyboardInfo, nullptr, std::bind(clearKeyboardKey, input.ConfigTag), nullptr, "Current setting")) {
            WAIT(500);
            bool result = configKeyboardKey(input.ConfigTag);
            UI::Notify(WARN, result ?
                fmt::format("[{}] saved", input.Name) :
                fmt::format("[{}] cancelled", input.Name));
            WAIT(500);
        }
        keyboardInfo.pop_back();
    }
}

void update_wheelmenu() {
    g_menu.Title("Wheel & pedals");
    auto wheelGuid = g_controls.WheelAxes[static_cast<int>(CarControls::WheelAxisType::Steer)].Guid;
    g_menu.Subtitle(FormatDeviceGuidName(wheelGuid));

    if (!g_controls.FreeDevices.empty()) {
        for (const auto& device : g_controls.FreeDevices) {
            if (g_menu.Option(device.name, NativeMenu::solidGreen,
                { "~r~<b>This device can be used as input device!</b>~w~",
                  "Set it up with the options below, or press Select to discard this message." })) {
                saveAllSettings();
                g_settings.SteeringAppendDevice(device.guid, device.name);
                g_settings.Read(&g_controls);
                g_controls.CheckGUIDs(g_settings.Wheel.InputDevices.RegisteredGUIDs);
            }
        }
    }

    if (g_menu.BoolOption("Enable wheel", g_settings.Wheel.Options.Enable,
        { "Enable usage of a steering wheel and pedals." })) {
        saveAllSettings();
        g_settings.Read(&g_controls);
        initWheel();
    }

    if (g_menu.BoolOption("Logitech RPM LEDs", g_settings.Wheel.Options.LogiLEDs,
        { "Show the RPM LEDs on Logitech steering wheels.",
            "If the wheel doesn't have compatible RPM LEDs, this might crash." })) {
        g_settings.SaveWheel();
    }

    g_menu.MenuOption("Analog input setup", "axesmenu",
        { "Configure the analog inputs." });

    g_menu.MenuOption("Force feedback options", "forcefeedbackmenu",
        { "Fine-tune your force feedback parameters." });

    g_menu.MenuOption("Soft lock options", "anglemenu",
        { "Set up soft lock options here." });

    g_menu.MenuOption("Button input setup", "buttonsmenu",
        { "Set up your buttons on your steering wheel." });

    std::vector<std::string> hpatInfo = {
        "Press RIGHT to clear H-pattern shifter",
        "Active gear:"
    };
    if (g_controls.ButtonIn(CarControls::WheelControlType::HR)) hpatInfo.emplace_back("Reverse");
    for (uint8_t gear = 1; gear < VExt::GearsAvailable(); ++gear) {
        // H1 == 1
        if (g_controls.ButtonIn(static_cast<CarControls::WheelControlType>(gear))) 
            hpatInfo.emplace_back(fmt::format("Gear {}", gear));
    }

    if (g_menu.OptionPlus("H-pattern shifter setup", hpatInfo, nullptr, std::bind(clearHShifter), nullptr, "Input values",
        { "Select this option to start H-pattern shifter setup. Follow the on-screen instructions." })) {
        bool result = configHPattern();
        UI::Notify(WARN, result ? "H-pattern shifter saved" : "Cancelled H-pattern shifter setup");
    }

    g_menu.BoolOption("Use shifter for automatic", g_settings.Wheel.Options.UseShifterForAuto,
        { "Use the H-pattern shifter to select the Drive/Reverse gears." });

    std::vector<std::string> hAutoInfo = {
        "Press RIGHT to clear H-pattern auto",
        "Active gear:"
    };
    if (g_controls.ButtonIn(CarControls::WheelControlType::APark)) hAutoInfo.emplace_back("Auto Park");
    if (g_controls.ButtonIn(CarControls::WheelControlType::AReverse)) hAutoInfo.emplace_back("Auto Reverse");
    if (g_controls.ButtonIn(CarControls::WheelControlType::ANeutral)) hAutoInfo.emplace_back("Auto Neutral");
    if (g_controls.ButtonIn(CarControls::WheelControlType::ADrive)) hAutoInfo.emplace_back("Auto Drive");

    if (g_menu.OptionPlus("Shifter setup for automatic", hAutoInfo, nullptr, [] { clearASelect(); }, nullptr, "Input values",
        { "Set up H-pattern shifter for automatic gearbox. Follow the on-screen instructions." })) {
        bool result = configASelect();
        UI::Notify(WARN, result ? "H-pattern shifter (auto) saved" : "Cancelled H-pattern shifter (auto) setup");
    }

    g_menu.BoolOption("Keyboard H-pattern", g_settings.Wheel.Options.HPatternKeyboard,
        { "This allows you to use the keyboard for H-pattern shifting. Configure the controls in the keyboard section." });
}

void update_anglemenu() {
    g_menu.Title("Soft lock");
    g_menu.Subtitle("");
    float minLock = 180.0f;
    if (g_menu.FloatOption("Physical degrees", g_settings.Wheel.Steering.AngleMax, minLock, 2880.0f, 30.0f,
        { "How many degrees your physical steering steering wheel turns.",
            "Set this to whatever matches the setting in the wheel driver." })) {
        if (g_settings.Wheel.Steering.AngleCar > g_settings.Wheel.Steering.AngleMax) { g_settings.Wheel.Steering.AngleCar = g_settings.Wheel.Steering.AngleMax; }
        if (g_settings.Wheel.Steering.AngleBike > g_settings.Wheel.Steering.AngleMax) { g_settings.Wheel.Steering.AngleBike = g_settings.Wheel.Steering.AngleMax; }
        if (g_settings.Wheel.Steering.AngleBoat > g_settings.Wheel.Steering.AngleMax) { g_settings.Wheel.Steering.AngleBoat = g_settings.Wheel.Steering.AngleMax; }
    }

    g_menu.FloatOption("Car soft lock", g_settings.Wheel.Steering.AngleCar, minLock, g_settings.Wheel.Steering.AngleMax, 30.0,
        { "Soft lock for generic cars and trucks. (degrees)",
            "Overridden by vehicle-specific soft lock." });

    g_menu.FloatOption("Bike soft lock", g_settings.Wheel.Steering.AngleBike, minLock, g_settings.Wheel.Steering.AngleMax, 30.0,
        { "Soft lock for generic bikes and quads. (degrees)",
            "Overridden by vehicle-specific soft lock." });

    g_menu.FloatOption("Boat soft lock", g_settings.Wheel.Steering.AngleBoat, minLock, g_settings.Wheel.Steering.AngleMax, 30.0,
        { "Soft lock for generic boats. (degrees)",
            "Overridden by vehicle-specific soft lock." });
}

void update_axesmenu() {
    g_menu.Title("Analog inputs");
    g_menu.Subtitle("");
    g_controls.UpdateValues(CarControls::Wheel, true);
    std::vector<std::string> info = {
        "Press RIGHT to clear this axis" ,
        fmt::format("Steer:\t\t{:.3f}", g_controls.SteerVal),
        fmt::format("Throttle:\t\t{:.3f}", g_controls.ThrottleVal),
        fmt::format("Brake:\t\t{:.3f}", g_controls.BrakeVal),
        fmt::format("Clutch:\t\t{:.3f}", g_controls.ClutchVal),
        fmt::format("Handbrake:\t{:.3f}", g_controls.HandbrakeVal),
    };

    for (const auto& input : g_controls.WheelAxes) {
        if (input.ConfigTag.empty())
            continue;

        // FFB handled in the wheel case of configAxis
        if (input.ConfigTag == "FFB")
            continue;

        bool selected = false;

        if (g_menu.OptionPlus(
            fmt::format("Configure {}", input.Name),
            {},
            &selected,
            [&input] { return clearAxis(input.ConfigTag); }, 
            nullptr, 
            "Input values")) {
            bool result = configAxis(input.ConfigTag);
            UI::Notify(WARN, result ? 
                fmt::format("[{}] saved", input.Name) : 
                fmt::format("[{}] cancelled", input.Name));
            if (result)
                initWheel();
        }

        if (selected) {
            if (!input.Control.empty() && input.Guid != GUID_NULL) {
                info.push_back(fmt::format("{} assigned to", input.Name));
                info.push_back(fmt::format("Device: {}", FormatDeviceGuidName(input.Guid)));
                info.push_back(fmt::format("Axis: {}", input.Control));
            }
            else {
                info.push_back(fmt::format("{} unassigned", input.Name));
            }

            g_menu.OptionPlusPlus(info, "Input values");
        }
    }

    g_menu.FloatOption("Steering deadzone", g_settings.Wheel.Steering.DeadZone, 0.0f, 0.5f, 0.01f,
        { "Deadzone size, from the center of the wheel." });

    g_menu.FloatOption("Steering deadzone offset", g_settings.Wheel.Steering.DeadZoneOffset, -0.5f, 0.5f, 0.01f,
        { "Put the deadzone with an offset from the center." });

    g_menu.FloatOption("Throttle anti-deadzone", g_settings.Wheel.Throttle.AntiDeadZone, 0.0f, 1.0f, 0.01f,
        { "GTA V ignores 25% input for analog controls by default." });

    g_menu.FloatOption("Brake anti-deadzone", g_settings.Wheel.Brake.AntiDeadZone, 0.0f, 1.0f, 0.01f,
        { "GTA V ignores 25% input for analog controls by default." });

    bool showBrakeGammaBox = false;
    std::vector<std::string> extras = {};
    g_menu.OptionPlus("Brake gamma", extras, &showBrakeGammaBox,
        [=] { return incVal(g_settings.Wheel.Brake.Gamma, 5.0f, 0.01f); },
        [=] { return decVal(g_settings.Wheel.Brake.Gamma, 0.1f, 0.01f); },
        "Brake gamma",
        { "Linearity of the brake pedal. Values over 1.0 feel more real if you have progressive springs." });

    if (showBrakeGammaBox) {
        extras = ShowGammaCurve("Brake", g_controls.BrakeVal, g_settings.Wheel.Brake.Gamma);
        g_menu.OptionPlusPlus(extras, "Brake gamma");
    }

    bool showThrottleGammaBox = false;
    extras = {};
    g_menu.OptionPlus("Throttle gamma", extras, &showThrottleGammaBox,
        [=] { return incVal(g_settings.Wheel.Throttle.Gamma, 5.0f, 0.01f); },
        [=] { return decVal(g_settings.Wheel.Throttle.Gamma, 0.1f, 0.01f); },
        "Throttle gamma",
        { "Linearity of the throttle pedal." });

    if (showThrottleGammaBox) {
        extras = ShowGammaCurve("Throttle", g_controls.ThrottleVal, g_settings.Wheel.Throttle.Gamma);
        g_menu.OptionPlusPlus(extras, "Throttle gamma");
    }

    bool showSteeringGammaBox = false;
    extras = {};
    g_menu.OptionPlus("Steering gamma", extras, &showSteeringGammaBox,
        [=] { return incVal(g_settings.Wheel.Steering.Gamma, 5.0f, 0.01f); },
        [=] { return decVal(g_settings.Wheel.Steering.Gamma, 0.1f, 0.01f); },
        "Steering gamma",
        { "Linearity of the steering wheel." });

    if (showSteeringGammaBox) {
        float steerValL = map(g_controls.SteerVal, 0.0f, 0.5f, 1.0f, 0.0f);
        float steerValR = map(g_controls.SteerVal, 0.5f, 1.0f, 0.0f, 1.0f);
        float steerVal = g_controls.SteerVal < 0.5f ? steerValL : steerValR;
        extras = ShowGammaCurve("Steering", steerVal, g_settings.Wheel.Steering.Gamma);
        g_menu.OptionPlusPlus(extras, "Steering gamma");
    }
}

void update_forcefeedbackmenu() {
    g_menu.Title("Force feedback");
    g_menu.Subtitle("");

    g_menu.BoolOption("Enable", g_settings.Wheel.FFB.Enable,
        { "Enable or disable force feedback entirely." });

    g_menu.FloatOption("FFB SAT scale", g_settings.Wheel.FFB.SATAmpMult, 0.05f, 10.0f, 0.05f,
        { "Overall FFB scaling for self-aligning torque.",
            "Recommended to leave at 1.0 and use FFB response to tune FFB.",
            "Values over 1.0 clips FFB. Lower values decrease overall SAT FFB strength.",
            "Note: Final SAT scale affected by vehicle-specific multiplier." });

    g_menu.StringArray("FFB linearity type", { ffbCurveTypes }, g_settings.Wheel.FFB.FFBProfile,
        { "Both types allow modifying the FFB response.",
            "Gamma normally adjusts the curve.",
            "Boosted has a default initial ramp-up." });

    bool showDynamicFfbCurveBox = false;
    if (g_menu.OptionPlus(fmt::format("FFB response curve: < {:.2f} >", g_settings.Wheel.FFB.ResponseCurve), {}, &showDynamicFfbCurveBox,
            [=] { return incVal(g_settings.Wheel.FFB.ResponseCurve, 5.00f, 0.01f); },
            [=] { return decVal(g_settings.Wheel.FFB.ResponseCurve, 0.01f, 0.01f); },
            "Response curve", {
                "Response curve of the self-aligning torque.",
                "Decrease to ramp up force quicker.",
                "Increase to get a more linear force buildup.",
                "Note: Final force curve affected by vehicle-specific multiplier." })) {
        float val = g_settings.Wheel.FFB.ResponseCurve;
        if (GetKbEntryFloat(val)) {
            g_settings.Wheel.FFB.ResponseCurve = val;
        }
    }

    if (showDynamicFfbCurveBox) {
        auto extras = ShowDynamicFfbCurve(abs(WheelInput::GetFFBConstantForce()),
                                          g_settings.Wheel.FFB.ResponseCurve,
                                          g_settings.Wheel.FFB.FFBProfile);
        g_menu.OptionPlusPlus(extras, "Response Curve");
    }

    g_menu.FloatOption("Detail effect multiplier", g_settings.Wheel.FFB.DetailMult, 0.0f, 10.0f, 0.1f,
        { "Force feedback caused by the suspension.",
          "This force stacks on top of the main SAT force." });

    g_menu.IntOption("Detail effect limit", g_settings.Wheel.FFB.DetailLim, 0, 20000, 100, 
        { "Clamp detail force to this value.",
          "20000 allows maxing out detail force in the other direction the SAT is acting on." });

    g_menu.IntOption("Detail effect averaging", g_settings.Wheel.FFB.DetailMAW, 1, 100, 1,
        { "Averages the detail force to prevent force feedback spikes.",
        "Recommended to keep as low as possible, as detail is lost with higher values."});

    g_menu.FloatOption("Collision effect multiplier", g_settings.Wheel.FFB.CollisionMult, 0.0f, 10.0f, 0.1f,
        { "Force feedback effect caused by frontal/rear collisions." });

    g_menu.IntOption("Damper max (low speed)", g_settings.Wheel.FFB.DamperMax, 0, 200, 1,
        { "Wheel friction at low speed." });

    g_menu.IntOption("Damper min (high speed)", g_settings.Wheel.FFB.DamperMin, 0, 200, 1,
        { "Wheel friction at high speed." });

    g_menu.FloatOption("Damper min speed", g_settings.Wheel.FFB.DamperMinSpeed, 0.0f, 40.0f, 0.2f,
        { "Speed where the damper strength should be minimal.", "In m/s." });

    if (g_settings.Wheel.FFB.LUTFile.empty()) {
        if (g_menu.Option("FFB LUT inactive", {
            "Use an FFB lookup table (LUT) to customize FFB response, and correct the non-linear response of your wheels' motors.",
            "Select to open the readme on how to activate this."
        })) {
            WAIT(20);
            PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, ControlFrontendPause, 1.0f);
            ShellExecuteA(0, 0, "https://github.com/E66666666/GTAVManualTransmission/blob/master/doc/README.md#wheel-ffb-lut", 0, 0, SW_SHOW);
        }

        if (g_menu.Option("Tune FFB anti-deadzone")) {
            g_controls.PlayFFBCollision(0);
            g_controls.PlayFFBDynamics(0, 0);
            while (true) {
                if (IsKeyJustUp(GetKeyFromName(escapeKey))) {
                    break;
                }

                if (IsKeyJustUp(GetKeyFromName("LEFT"))) {
                    g_settings.Wheel.FFB.AntiDeadForce -= 100;
                    if (g_settings.Wheel.FFB.AntiDeadForce < 100) {
                        g_settings.Wheel.FFB.AntiDeadForce = 0;
                    }
                }

                if (IsKeyJustUp(GetKeyFromName("RIGHT"))) {
                    g_settings.Wheel.FFB.AntiDeadForce += 100;
                    if (g_settings.Wheel.FFB.AntiDeadForce > 10000 - 100) {
                        g_settings.Wheel.FFB.AntiDeadForce = 10000;
                    }
                }
                g_controls.UpdateValues(CarControls::InputDevices::Wheel, true);

                g_controls.PlayFFBDynamics(g_settings.Wheel.FFB.AntiDeadForce, 0);

                UI::ShowHelpText(fmt::format("Press [Left] and [Right] to decrease and increase force feedback anti-deadzone. "
                    "Use the highest value before your wheel starts moving. Currently [{}]. Press [{}] to exit.", g_settings.Wheel.FFB.AntiDeadForce, escapeKey));
                WAIT(0);
            }
        }
    }
    else {
        g_menu.Option("FFB LUT active", {
            fmt::format("Using LUT: {}", g_settings.Wheel.FFB.LUTFile),
            "FFB anti-deadzone disabled."
        });
    }

    g_menu.MenuOption("FFB normalization options", "ffbnormalizationmenu",
        { "Different fTractionCurveLateral values affect FFB response.",
          "These values normalize the steering forces across different handlings." });

    if (g_settings.Debug.ShowAdvancedFFBOptions) {
        g_menu.OptionPlus("Legacy FFB options:",
            { "This is an old FFB implementation, based on movement versus steering inputs.",
              "Road vehicles use better FFB with wheel data, but old FFB is still used for non-road vehicles." });

        g_menu.FloatOption("SAT factor", g_settings.Wheel.FFB.SATFactor, 0.0f, 1.0f, 0.01f,
            { "Reactive force offset/multiplier, when not going straight.",
              "Depending on wheel strength, a larger value helps reaching countersteer faster or reduce overcorrection." });

        g_menu.FloatOption("SAT gamma", g_settings.Wheel.FFB.Gamma, 0.01f, 2.0f, 0.01f,
            { "< 1.0: More FFB at low speeds, FFB tapers off towards speed cap.",
              "> 1.0: Less FFB at low speeds, FFB increases towards speed cap.",
              "Keep at 1.0 for linear response. ~h~Not~h~ recommended to go higher than 1.0!" });

        g_menu.FloatOption("SAT max speed", g_settings.Wheel.FFB.MaxSpeed, 10.0f, 1000.0f, 1.0f,
            { "Speed where FFB stops increasing. Helpful against too strong FFB at extreme speeds.",
              fmt::format("{} kph / {} mph", g_settings.Wheel.FFB.MaxSpeed * 3.6f, g_settings.Wheel.FFB.MaxSpeed * 2.23694f) });
    }
}

void update_ffbnormalizationmenu() {
    g_menu.Title("FFB Normalization");
    g_menu.Subtitle("");

    g_menu.OptionPlus("Description",
        {
            "These values affect the usage of 'FFB response curve' value, relative to the fTractionCurveLateral (optimal slip angle) value of the vehicle.",
            "Higher optimal slip angles cause FFB to respond slowly to small changes, lower optimal slip angles cause FFB to respond quick to small changes.",
            "These values normalize this, to make the high slip values used by default GTA V handlings to respond quickly.",
            "For the best experience, use a custom handling with low fTractionCurveMin/Max and low fTractionCurveLateral values.",
            "No normalization: 5.0/1.0, 20.0/1.0",
            "Default normalization: 7.5/1.6, 20.0/1.0"
        }
    );

    if (g_menu.Option("No normalization", { "Use this option to quickly disable normalization." })) {
        g_settings.Wheel.FFB.SlipOptMin = 5.0f;
        g_settings.Wheel.FFB.SlipOptMinMult = 1.0f;
        g_settings.Wheel.FFB.SlipOptMax = 20.0f;
        g_settings.Wheel.FFB.SlipOptMaxMult = 1.0f;
    }

    if (g_menu.Option("Default normalization", { "Use this option to quickly apply default normalization." })) {
        g_settings.Wheel.FFB.SlipOptMin = 7.5f;
        g_settings.Wheel.FFB.SlipOptMinMult = 1.6f;
        g_settings.Wheel.FFB.SlipOptMax = 20.0f;
        g_settings.Wheel.FFB.SlipOptMaxMult = 1.0f;
    }

    g_menu.FloatOptionCb("Optimal slip low", g_settings.Wheel.FFB.SlipOptMin, 0.0f, 90.0f, 0.1f, GetKbEntryFloat);
    g_menu.FloatOptionCb("Optimal slip low mult", g_settings.Wheel.FFB.SlipOptMinMult, 0.0f, 10.0f, 0.1f, GetKbEntryFloat,
        { "Higher values give weaker FFB at low steering angles.",
          "For low optimal slip angles, this value should be high." });
    g_menu.FloatOptionCb("Optimal slip high", g_settings.Wheel.FFB.SlipOptMax, 0.0f, 90.0f, 0.1f, GetKbEntryFloat);
    g_menu.FloatOptionCb("Optimal slip high mult", g_settings.Wheel.FFB.SlipOptMaxMult, 0.0f, 10.0f, 0.1f, GetKbEntryFloat,
        { "Higher values give weaker FFB at low steering angles.",
          "For high optimal slip angles, this value should be low." });
}

void update_buttonsmenu() {
    g_menu.Title("Buttons");
    g_menu.Subtitle("");

    std::vector<std::string> wheelToKeyInfo = {
        "Active wheel-to-key options:",
        "Press RIGHT to clear all keys bound to button",
    };

    if (g_controls.WheelToKey.empty()) {
        wheelToKeyInfo.push_back("No keys assigned");
    }
    for (const auto& input : g_controls.WheelToKey) {
        int index = g_settings.GUIDToDeviceIndex(input.Guid);
        std::string name = g_settings.GUIDToDeviceName(input.Guid);
        wheelToKeyInfo.push_back(fmt::format("DEV{}BUTTON{} = {} ({})", index, input.Control, input.ConfigTag, name));
        if (g_controls.GetWheel().IsButtonPressed(input.Control, input.Guid)) {
            wheelToKeyInfo.back() = fmt::format("{} (Pressed)", wheelToKeyInfo.back());
        }
    }

    if (g_menu.OptionPlus("Set up WheelToKey", wheelToKeyInfo, nullptr, clearWheelToKey, nullptr, "Info",
        { "Set up wheel buttons that press a keyboard key." })) {
        bool result = configWheelToKey();
        UI::Notify(WARN, result ? "Entry added" : "Cancelled entry addition");
    }

    std::vector<std::string> buttonInfo;
    buttonInfo.emplace_back("Press RIGHT to clear this button");
    buttonInfo.emplace_back("Active buttons:");

    for (uint32_t i = 0; i < static_cast<uint32_t>(CarControls::WheelControlType::SIZEOF_WheelControlType); ++i) {
        const auto& input = g_controls.WheelButton[i];

        // Don't show the H-pattern shifter in this menu, it has its own options
        if (input.ConfigTag.rfind("HPATTERN_", 0) == 0 ||
            input.ConfigTag.rfind("AUTO_", 0) == 0)
            continue;

        if (g_controls.ButtonIn(static_cast<CarControls::WheelControlType>(i)))
            buttonInfo.emplace_back(input.Name);
    }
    if (buttonInfo.size() == 2)
        buttonInfo.emplace_back("None");

    for (const auto& input : g_controls.WheelButton) {
        // Don't show the H-pattern shifter in this menu, it has its own options
        if (input.ConfigTag.rfind("HPATTERN_", 0) == 0 ||
            input.ConfigTag.rfind("AUTO_", 0) == 0 ||
            input.ConfigTag.empty())
            continue;

        bool popTwo = false;
        if (input.Control != -1) {
            buttonInfo.push_back(fmt::format("Device: {}", FormatDeviceGuidName(input.Guid)));
            buttonInfo.push_back(fmt::format("Button: {}", input.Control));
            popTwo = true;
        }
        else {
            buttonInfo.push_back(fmt::format("{} unassigned", input.Name));
        }
        if (g_menu.OptionPlus(
            fmt::format("Assign [{}]", input.Name),
            buttonInfo,
            nullptr,
            [&input] { return clearButton(input.ConfigTag); },
            nullptr,
            "Current inputs")) {
            bool result = configButton(input.ConfigTag);
            UI::Notify(WARN, result ?
                fmt::format("[{}] saved", input.Name) :
                fmt::format("[{}] cancelled", input.Name));
            if (result)
                initWheel();
        }
        buttonInfo.pop_back();
        if (popTwo)
            buttonInfo.pop_back();
    }
}

void update_hudmenu() {
    g_menu.Title("HUD options");
    g_menu.Subtitle("");

    g_menu.BoolOption("Enable", g_settings.HUD.Enable,
        { "Display HUD elements." });

    g_menu.BoolOption("Always enable", g_settings.HUD.Always,
        { "Display HUD even if manual transmission is off." });

    auto fontIt = std::find_if(fonts.begin(), fonts.end(), [](const SFont& font) { return font.ID == g_settings.HUD.Font; });
    if (fontIt != fonts.end()) {
        std::vector<std::string> strFonts;
        strFonts.reserve(fonts.size());
        for (const auto& font : fonts)
            strFonts.push_back(font.Name);
        int fontIndex = static_cast<int>(fontIt - fonts.begin());
        if (g_menu.StringArray("Font", strFonts, fontIndex, { "Select the font for speed, gearbox mode, current gear." })) {
            g_settings.HUD.Font = fonts.at(fontIndex).ID;
        }
    }
    else {
        g_menu.Option("Invalid font ID in settings!", NativeMenu::solidRed, { "Fix the font index in settings_general.ini." });
    }

    g_menu.BoolOption("Outline text", g_settings.HUD.Outline);

    g_menu.StringArray("Notification level", notifyLevelStrings, g_settings.HUD.NotifyLevel,
        { "What kind of notifications to display.",
        "Debug: All",
        "Info: Mode switching",
        "UI: Menu actions and setup",
        "None: Hide all notifications" });

    g_menu.MenuOption("Gear and shift mode", "geardisplaymenu");
    g_menu.MenuOption("Speedometer", "speedodisplaymenu");
    g_menu.MenuOption("RPM gauge", "rpmdisplaymenu");
    g_menu.MenuOption("Wheel & Pedal info", "wheelinfomenu");
    g_menu.MenuOption("Dashboard indicators", "dashindicatormenu", 
        { "Indicator icons for ABS, TCS, ESC and the hand brake." });
    g_menu.MenuOption("Downshift protection", "dsprotmenu",
        { "If and where the downshift protection notification should appear." });
    g_menu.MenuOption("Mouse steering", "mousehudmenu");
}

void update_geardisplaymenu() {
    g_menu.Title("Gear options");
    g_menu.Subtitle("");

    g_menu.BoolOption("Gear", g_settings.HUD.Gear.Enable);
    g_menu.BoolOption("Shift Mode", g_settings.HUD.ShiftMode.Enable);

    g_menu.FloatOption("Gear X", g_settings.HUD.Gear.XPos, 0.0f, 1.0f, 0.005f);
    g_menu.FloatOption("Gear Y", g_settings.HUD.Gear.YPos, 0.0f, 1.0f, 0.005f);
    g_menu.FloatOption("Gear Size", g_settings.HUD.Gear.Size, 0.0f, 3.0f, 0.05f);
    g_menu.IntOption("Gear Color Red", g_settings.HUD.Gear.ColorR, 0, 255);
    g_menu.IntOption("Gear Color Green", g_settings.HUD.Gear.ColorG, 0, 255);
    g_menu.IntOption("Gear Color Blue", g_settings.HUD.Gear.ColorB, 0, 255);
    g_menu.IntOption("Gear Top Color Red", g_settings.HUD.Gear.TopColorR, 0, 255);
    g_menu.IntOption("Gear Top Color Green", g_settings.HUD.Gear.TopColorG, 0, 255);
    g_menu.IntOption("Gear Top Color Blue", g_settings.HUD.Gear.TopColorB, 0, 255);

    g_menu.FloatOption("Shift Mode X", g_settings.HUD.ShiftMode.XPos, 0.0f, 1.0f, 0.005f);
    g_menu.FloatOption("Shift Mode Y", g_settings.HUD.ShiftMode.YPos, 0.0f, 1.0f, 0.005f);
    g_menu.FloatOption("Shift Mode Size", g_settings.HUD.ShiftMode.Size, 0.0f, 3.0f, 0.05f);
    g_menu.IntOption("Shift Mode Red", g_settings.HUD.ShiftMode.ColorR, 0, 255);
    g_menu.IntOption("Shift Mode Green", g_settings.HUD.ShiftMode.ColorG, 0, 255);
    g_menu.IntOption("Shift Mode Blue", g_settings.HUD.ShiftMode.ColorB, 0, 255);
}

void update_speedodisplaymenu() {
    g_menu.Title("Speedometer options");
    g_menu.Subtitle("");

    int oldPos = 0;
    auto speedoTypeIt = std::find(speedoTypes.begin(), speedoTypes.end(), g_settings.HUD.Speedo.Speedo);
    if (speedoTypeIt != speedoTypes.end()) {
        oldPos = static_cast<int>(speedoTypeIt - speedoTypes.begin());
    }
    int newPos = oldPos;
    g_menu.StringArray("Speedometer", speedoTypes, newPos);
    if (newPos != oldPos) {
        g_settings.HUD.Speedo.Speedo = speedoTypes.at(newPos);
    }
    g_menu.BoolOption("Use drivetrain speed", g_settings.HUD.Speedo.UseDrivetrain,
        { "Uses speed from the driven wheels or dashboard if selected, otherwise uses physics speed." });
    g_menu.BoolOption("Show units", g_settings.HUD.Speedo.ShowUnit);

    g_menu.FloatOption("Speedometer X", g_settings.HUD.Speedo.XPos, 0.0f, 1.0f, 0.005f);
    g_menu.FloatOption("Speedometer Y", g_settings.HUD.Speedo.YPos, 0.0f, 1.0f, 0.005f);
    g_menu.FloatOption("Speedometer Size", g_settings.HUD.Speedo.Size, 0.0f, 3.0f, 0.05f);

    g_menu.IntOption("Speedometer Color Red", g_settings.HUD.Speedo.ColorR, 0, 255);
    g_menu.IntOption("Speedometer Color Green", g_settings.HUD.Speedo.ColorG, 0, 255);
    g_menu.IntOption("Speedometer Color Blue", g_settings.HUD.Speedo.ColorB, 0, 255);
}

void update_rpmdisplaymenu() {
    g_menu.Title("RPM Gauge options");
    g_menu.Subtitle("");

    g_menu.BoolOption("Enable", g_settings.HUD.RPMBar.Enable);
    g_menu.FloatOption("Redline RPM", g_settings.HUD.RPMBar.Redline, 0.0f, 1.0f, 0.01f);

    g_menu.FloatOption("X position", g_settings.HUD.RPMBar.XPos, 0.0f, 1.0f, 0.0025f);
    g_menu.FloatOption("Y position", g_settings.HUD.RPMBar.YPos, 0.0f, 1.0f, 0.0025f);
    g_menu.FloatOption("Width", g_settings.HUD.RPMBar.XSz, 0.0f, 1.0f, 0.0025f);
    g_menu.FloatOption("Height", g_settings.HUD.RPMBar.YSz, 0.0f, 1.0f, 0.0025f);

    g_menu.IntOption("Background Red"  , g_settings.HUD.RPMBar.BgR, 0, 255);
    g_menu.IntOption("Background Green", g_settings.HUD.RPMBar.BgG, 0, 255);
    g_menu.IntOption("Background Blue" , g_settings.HUD.RPMBar.BgB, 0, 255);
    g_menu.IntOption("Background Alpha", g_settings.HUD.RPMBar.BgA, 0, 255);

    g_menu.IntOption("Foreground Red"  , g_settings.HUD.RPMBar.FgR, 0, 255);
    g_menu.IntOption("Foreground Green", g_settings.HUD.RPMBar.FgG, 0, 255);
    g_menu.IntOption("Foreground Blue" , g_settings.HUD.RPMBar.FgB, 0, 255);
    g_menu.IntOption("Foreground Alpha", g_settings.HUD.RPMBar.FgA, 0, 255);

    g_menu.IntOption("Redline Red"  , g_settings.HUD.RPMBar.RedlineR, 0, 255);
    g_menu.IntOption("Redline Green", g_settings.HUD.RPMBar.RedlineG, 0, 255);
    g_menu.IntOption("Redline Blue" , g_settings.HUD.RPMBar.RedlineB, 0, 255);
    g_menu.IntOption("Redline Alpha", g_settings.HUD.RPMBar.RedlineA, 0, 255);

    g_menu.IntOption("Revlimit Red"  , g_settings.HUD.RPMBar.RevLimitR, 0, 255);
    g_menu.IntOption("Revlimit Green", g_settings.HUD.RPMBar.RevLimitG, 0, 255);
    g_menu.IntOption("Revlimit Blue" , g_settings.HUD.RPMBar.RevLimitB, 0, 255);
    g_menu.IntOption("Revlimit Alpha", g_settings.HUD.RPMBar.RevLimitA, 0, 255);

    g_menu.IntOption("LaunchControlStaged Red"  , g_settings.HUD.RPMBar.LaunchControlStagedR, 0, 255);
    g_menu.IntOption("LaunchControlStaged Green", g_settings.HUD.RPMBar.LaunchControlStagedG, 0, 255);
    g_menu.IntOption("LaunchControlStaged Blue" , g_settings.HUD.RPMBar.LaunchControlStagedB, 0, 255);
    g_menu.IntOption("LaunchControlStaged Alpha", g_settings.HUD.RPMBar.LaunchControlStagedA, 0, 255);

    g_menu.IntOption("LaunchControlActive Red"  , g_settings.HUD.RPMBar.LaunchControlActiveR, 0, 255);
    g_menu.IntOption("LaunchControlActive Green", g_settings.HUD.RPMBar.LaunchControlActiveG, 0, 255);
    g_menu.IntOption("LaunchControlActive Blue" , g_settings.HUD.RPMBar.LaunchControlActiveB, 0, 255);
    g_menu.IntOption("LaunchControlActive Alpha", g_settings.HUD.RPMBar.LaunchControlActiveA, 0, 255);
}

void update_wheelinfomenu() {
    g_menu.Title("Wheel & pedal info");
    g_menu.Subtitle("");

    g_menu.BoolOption("Display steering wheel info", g_settings.HUD.Wheel.Enable, { "Show input info graphically." });
    g_menu.BoolOption("Display FFB level", g_settings.HUD.Wheel.FFB.Enable, { "Show a bar with force feedback level." });
    g_menu.BoolOption("Always display info", g_settings.HUD.Wheel.Always, { "Display the info even without a steering wheel." });

    g_menu.FloatOption("Wheel image X", g_settings.HUD.Wheel.ImgXPos, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("Wheel image Y", g_settings.HUD.Wheel.ImgYPos, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("Wheel image size", g_settings.HUD.Wheel.ImgSize, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("Pedals X", g_settings.HUD.Wheel.PedalXPos, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("Pedals Y", g_settings.HUD.Wheel.PedalYPos, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("Pedals Width", g_settings.HUD.Wheel.PedalXSz, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("Pedals Height", g_settings.HUD.Wheel.PedalYSz, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("Pedals Pad X", g_settings.HUD.Wheel.PedalXPad, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("Pedals Pad Y", g_settings.HUD.Wheel.PedalYPad, 0.0f, 1.0f, 0.01f);
    g_menu.IntOption("Pedals Background Alpha", g_settings.HUD.Wheel.PedalBgA, 0, 255);
    g_menu.IntOption("Throttle Bar Red    ", g_settings.HUD.Wheel.PedalThrottleR, 0, 255);
    g_menu.IntOption("Throttle Bar Green  ", g_settings.HUD.Wheel.PedalThrottleG, 0, 255);
    g_menu.IntOption("Throttle Bar Blue   ", g_settings.HUD.Wheel.PedalThrottleB, 0, 255);
    g_menu.IntOption("Throttle Bar Alpha  ", g_settings.HUD.Wheel.PedalThrottleA, 0, 255);
    g_menu.IntOption("Brake Bar Red    ", g_settings.HUD.Wheel.PedalBrakeR, 0, 255);
    g_menu.IntOption("Brake Bar Green  ", g_settings.HUD.Wheel.PedalBrakeG, 0, 255);
    g_menu.IntOption("Brake Bar Blue   ", g_settings.HUD.Wheel.PedalBrakeB, 0, 255);
    g_menu.IntOption("Brake Bar Alpha  ", g_settings.HUD.Wheel.PedalBrakeA   , 0, 255);
    g_menu.IntOption("Clutch Bar Red    ", g_settings.HUD.Wheel.PedalClutchR, 0, 255);
    g_menu.IntOption("Clutch Bar Green  ", g_settings.HUD.Wheel.PedalClutchG, 0, 255);
    g_menu.IntOption("Clutch Bar Blue   ", g_settings.HUD.Wheel.PedalClutchB, 0, 255);
    g_menu.IntOption("Clutch Bar Alpha  ", g_settings.HUD.Wheel.PedalClutchA  , 0, 255);

    g_menu.FloatOption("FFB Position X", g_settings.HUD.Wheel.FFB.XPos, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("FFB Position Y", g_settings.HUD.Wheel.FFB.YPos, 0.0f, 1.0f, 0.01f);

    g_menu.FloatOption("FFB Width", g_settings.HUD.Wheel.FFB.XSz, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("FFB Height", g_settings.HUD.Wheel.FFB.YSz, 0.0f, 1.0f, 0.01f);

    g_menu.IntOption("FFB Background Red",   g_settings.HUD.Wheel.FFB.BgR, 0, 255);
    g_menu.IntOption("FFB Background Green", g_settings.HUD.Wheel.FFB.BgG, 0, 255);
    g_menu.IntOption("FFB Background Blue",  g_settings.HUD.Wheel.FFB.BgB, 0, 255);
    g_menu.IntOption("FFB Background Alpha", g_settings.HUD.Wheel.FFB.BgA, 0, 255);

    g_menu.IntOption("FFB Foreground Red",   g_settings.HUD.Wheel.FFB.FgR, 0, 255);
    g_menu.IntOption("FFB Foreground Green", g_settings.HUD.Wheel.FFB.FgG, 0, 255);
    g_menu.IntOption("FFB Foreground Blue",  g_settings.HUD.Wheel.FFB.FgB, 0, 255);
    g_menu.IntOption("FFB Foreground Alpha", g_settings.HUD.Wheel.FFB.FgA, 0, 255);

    g_menu.IntOption("FFB Clipping Red",   g_settings.HUD.Wheel.FFB.LimitR, 0, 255);
    g_menu.IntOption("FFB Clipping Green", g_settings.HUD.Wheel.FFB.LimitG, 0, 255);
    g_menu.IntOption("FFB Clipping Blue",  g_settings.HUD.Wheel.FFB.LimitB, 0, 255);
    g_menu.IntOption("FFB Clipping Alpha", g_settings.HUD.Wheel.FFB.LimitA, 0, 255);
}

void update_dashindicatormenu() {
    g_menu.Title("Dashboard indicators");
    g_menu.Subtitle("");

    g_menu.BoolOption("Enable", g_settings.HUD.DashIndicators.Enable);
    g_menu.FloatOption("Position X", g_settings.HUD.DashIndicators.XPos, 0.0f, 1.0f, 0.005f);
    g_menu.FloatOption("Position Y", g_settings.HUD.DashIndicators.YPos, 0.0f, 1.0f, 0.005f);
    g_menu.FloatOption("Size", g_settings.HUD.DashIndicators.Size, 0.25f, 4.0f, 0.05f);
}

void update_dsprotmenu() {
    g_menu.Title("Downshift protection");
    g_menu.Subtitle("");

    extern int g_textureDsProtId;
    drawTexture(g_textureDsProtId, 0, -9998, 100,
        g_settings.HUD.DsProt.Size, g_settings.HUD.DsProt.Size,
        0.5f, 0.5f, // center of texture
        g_settings.HUD.DsProt.XPos, g_settings.HUD.DsProt.YPos,
        0.0f, GRAPHICS::GET_ASPECT_RATIO(FALSE), 1.0f, 1.0f, 1.0f, 1.0f);

    g_menu.BoolOption("Enable", g_settings.HUD.DsProt.Enable,
        { "When enabled, a warning flashes and a warning sound plays." });

    g_menu.FloatOption("Position X", g_settings.HUD.DsProt.XPos, 0.0f, 1.0f, 0.005f);
    g_menu.FloatOption("Position Y", g_settings.HUD.DsProt.YPos, 0.0f, 1.0f, 0.005f);
    g_menu.FloatOption("Size", g_settings.HUD.DsProt.Size, 0.01f, 1.0f, 0.01f);
}

void update_mousehudmenu() {
    g_menu.Title("Mouse steering HUD options");
    g_menu.Subtitle("");

    g_menu.BoolOption("Enable", g_settings.HUD.MouseSteering.Enable);

    g_menu.FloatOption("Pos X", g_settings.HUD.MouseSteering.XPos, 0.0f, 1.0f, 0.0025f);
    g_menu.FloatOption("Pos Y", g_settings.HUD.MouseSteering.YPos, 0.0f, 1.0f, 0.0025f);
    g_menu.FloatOption("Width", g_settings.HUD.MouseSteering.XSz, 0.0f, 1.0f, 0.0025f);
    g_menu.FloatOption("Height", g_settings.HUD.MouseSteering.YSz, 0.0f, 1.0f, 0.0025f);
    g_menu.FloatOption("Marker width", g_settings.HUD.MouseSteering.MarkerXSz, 0.0f, 1.0f, 0.0025f);

    g_menu.IntOption("Background Red  ", g_settings.HUD.MouseSteering.BgR, 0, 255);
    g_menu.IntOption("Background Green", g_settings.HUD.MouseSteering.BgG, 0, 255);
    g_menu.IntOption("Background Blue ", g_settings.HUD.MouseSteering.BgB, 0, 255);
    g_menu.IntOption("Background Alpha", g_settings.HUD.MouseSteering.BgA, 0, 255);

    g_menu.IntOption("Foreground Red  ", g_settings.HUD.MouseSteering.FgR, 0, 255);
    g_menu.IntOption("Foreground Green", g_settings.HUD.MouseSteering.FgG, 0, 255);
    g_menu.IntOption("Foreground Blue ", g_settings.HUD.MouseSteering.FgB, 0, 255);
    g_menu.IntOption("Foreground Alpha", g_settings.HUD.MouseSteering.FgA, 0, 255);
}

void update_driveassistmenu() {
    g_menu.Title("Driving assists");
    g_menu.Subtitle(MenuSubtitleConfig());

    g_menu.BoolOption("Anti-lock Braking", g_settings().DriveAssists.ABS.Enable,
        { "Custom script-driven anti-lock braking system (ABS).",
          "Prevent wheels from locking up during braking." });

    g_menu.MenuOption("ABS settings", "abssettingsmenu",
        { "Change how the anti-lock braking system works." });

    g_menu.BoolOption("Traction Control", g_settings().DriveAssists.TCS.Enable,
        { "Script-driven traction control system (TCS).",
          "Prevent traction loss under acceleration." });

    g_menu.MenuOption("TCS settings", "tcssettingsmenu",
        { "Change how and when the traction control system works." });

    g_menu.BoolOption("Stability Control", g_settings().DriveAssists.ESP.Enable,
        { "Script-driven stability control system (ESC).",
          "Prevent excessive over- and understeer." });

    g_menu.MenuOption("ESC settings", "espsettingsmenu", 
        { "Change the behaviour and tolerances of the stability control system." });

    g_menu.BoolOption("Launch Control", g_settings().DriveAssists.LaunchControl.Enable,
        { "Keeps a stable RPM and manages throttle in first gear, for maximum launch performance." });

    g_menu.MenuOption("Launch Control settings", "lcssettings",
        { "Change how and when the launch control works." });

    g_menu.BoolOption("Limited Slip Differential", g_settings().DriveAssists.LSD.Enable,
        { "Simulate a viscous limited slip differential (LSD).",
          "Thanks to any333!"});

    g_menu.MenuOption("LSD settings", "lsdsettingsmenu",
        { "Change how the limited slip differential works." });

    g_menu.BoolOption("Adaptive AWD", g_settings().DriveAssists.AWD.Enable,
        { "Transfers torque to stabilize the car. See Nissans' ATTESA, Audis' Quattro, etc.",
          "Only works on AWD cars. Recommended to use only in vehicle-specific configurations!"});

    g_menu.MenuOption("Adaptive AWD settings", "awdsettingsmenu");

    g_menu.BoolOption("Cruise Control", g_settings().DriveAssists.CruiseControl.Enable);

    g_menu.MenuOption("Cruise Control settings", "cruisecontrolsettingsmenu");
}

void update_abssettingsmenu() {
    g_menu.Title("Anti-lock Braking");
    g_menu.Subtitle(MenuSubtitleConfig());

    g_menu.BoolOption("Only enable ABS when missing", g_settings().DriveAssists.ABS.Filter,
        { "Only enables custom ABS on cars without ABS.",
          "Keep this enabled unless you know what you're doing." });

    g_menu.BoolOption("Flash brake lights", g_settings().DriveAssists.ABS.Flash,
        { "Flash brakes light when ABS is active." });

}

void update_tcssettingsmenu() {
    g_menu.Title("Traction Control");
    g_menu.Subtitle(MenuSubtitleConfig());

    g_menu.StringArray("Traction control mode", tcsStrings, g_settings().DriveAssists.TCS.Mode,
        { "Different methods to stop wheels from losing traction.",
          "Brakes: Applies brakes for slipping wheels.",
          "Throttle: Reduce throttle when any slip is detected." });

    g_menu.FloatOption("Minimum slip", g_settings().DriveAssists.TCS.SlipMin, 1.0f, 20.0f, 0.05f,
        { "Wheel speed relative to actual speed, where traction control starts to intervene.",
          "Recommended value: 1.20x.",
          "Should be higher than 1.00x." });

    float min = g_settings().DriveAssists.TCS.SlipMin;
    g_menu.FloatOption("Maximum slip", g_settings().DriveAssists.TCS.SlipMax, min, 20.0f, 0.05f,
        { "Wheel speed relative to actual speed, where traction control intervention is maximum.",
          "Recommended value: 1.40x.",
          "Has to be higher than 'Minimum slip'." });

    g_menu.FloatOptionCb("Brake multiplier", g_settings().DriveAssists.TCS.BrakeMult, 0.0f, 10.0f, 0.05f, GetKbEntryFloat,
        { "How much traction control may brake a wheel at most.",
          "Only applies for Brake-mode traction control.",
          "0.5:  50% braking force applied.",
          "1.0: 100% braking force applied.",
          "2.0: 200% braking force applied." });
}

void update_espsettingsmenu() {
    g_menu.Title("Stability Control");
    g_menu.Subtitle(MenuSubtitleConfig());

    g_menu.FloatOption("Oversteer starting angle", g_settings().DriveAssists.ESP.OverMin, 0.0f, 90.0f, 0.1f,
        { "Angle (degrees) where ESC starts correcting for oversteer." });
    g_menu.FloatOption("Oversteer starting correction", g_settings().DriveAssists.ESP.OverMinComp, 0.0f, 10.0f, 0.1f,
        { "Starting ESC oversteer correction value. Additional braking force for the affected wheel." });
    g_menu.FloatOption("Oversteer max angle", g_settings().DriveAssists.ESP.OverMax, 0.0f, 90.0f, 0.1f,
        { "Angle (degrees) where ESC oversteer correction is maximized." });
    g_menu.FloatOption("Oversteer max correction", g_settings().DriveAssists.ESP.OverMaxComp, 0.0f, 10.0f, 0.1f,
        { "Max ESC oversteer correction value. Additional braking force for the affected wheel." });

    g_menu.FloatOption("Understeer starting angle", g_settings().DriveAssists.ESP.UnderMin, 0.0f, 90.0f, 0.1f,
        { "Angle (degrees) where ESC starts correcting for understeer." });
    g_menu.FloatOption("Understeer starting correction", g_settings().DriveAssists.ESP.UnderMinComp, 0.0f, 10.0f, 0.1f,
        { "Starting ESC understeer correction value. Additional braking force for the affected wheel." });
    g_menu.FloatOption("Understeer max angle", g_settings().DriveAssists.ESP.UnderMax, 0.0f, 90.0f, 0.1f,
        { "Angle (degrees) where ESC understeer correction is maximized." });
    g_menu.FloatOption("Understeer max correction", g_settings().DriveAssists.ESP.UnderMaxComp, 0.0f, 10.0f, 0.1f,
        { "Max ESC oversteer understeer value. Additional braking force for the affected wheel." });
}

void update_lcssettingsmenu() {
    g_menu.Title("Launch Control");
    g_menu.Subtitle(MenuSubtitleConfig());

    g_menu.FloatOption("Launch control RPM", g_settings().DriveAssists.LaunchControl.RPM, 0.3f, 0.9f, 0.025f,
        { "When staged, hold this RPM." });

    g_menu.FloatOption("Minimum slip", g_settings().DriveAssists.LaunchControl.SlipMin, 0.0f, 20.0f, 0.1f,
        { "Wheel speed relative to actual speed, where launch control starts to intervene (reduce throttle).",
          "Recommended value: 1.20x.",
          "Should be higher than 1.00x." });

    float min = g_settings().DriveAssists.LaunchControl.SlipMin;
    g_menu.FloatOption("Maximum slip", g_settings().DriveAssists.LaunchControl.SlipMax, min, 20.0f, 0.1f,
        { "Wheel speed relative to actual speed, where launch control intervention is maximum (zero throttle).",
          "Recommended value: 1.40x.",
          "Has to be higher than 'Minimum slip'." });

    const std::vector<std::string> instructions{
        "Staging for launch control:",
        "1: Stop car, select 1st gear",
        "2: Hold brakes",
        "3: Apply throttle",
        "4: Release brake to launch!",
        "While staged, clutch can be released. Launch control manages the clutch.",
    };

    g_menu.OptionPlus("Instructions", instructions);
}

void update_lsdsettingsmenu() {
    g_menu.Title("Limited Slip Differential");
    g_menu.Subtitle(MenuSubtitleConfig());

    g_menu.FloatOption("LSD viscosity", g_settings().DriveAssists.LSD.Viscosity, 0.0f, 100.0f, 1.0f,
        { "How much the slower wheel tries to match the faster wheel.",
          "A very high value might speed up the car too much, because this LSD adds power to the slower wheel. "
          "About 10 is decent and doesn't affect acceleration." });

    bool statusSelected = false;
    g_menu.OptionPlus("Status", {}, &statusSelected, nullptr, nullptr, "Status", 
        { "Live LSD status" });

    if (statusSelected) {
        std::vector<std::string> extra;

        if (!g_settings().DriveAssists.LSD.Enable) {
            extra.push_back("LSD Disabled");
        }
        else {
            auto lsdData = DrivingAssists::GetLSD();
            std::string fddcol;
            if (lsdData.FDD > 0.1f) { fddcol = "~r~"; }
            if (lsdData.FDD < -0.1f) { fddcol = "~b~"; }

            std::string rddcol;
            if (lsdData.RDD > 0.1f) { rddcol = "~r~"; }
            if (lsdData.RDD < -0.1f) { rddcol = "~b~"; }

            extra.push_back(fmt::format("{}Front: L-R: {:.2f}", fddcol, lsdData.FDD));
            extra.push_back(fmt::format("{}Rear:  L-R: {:.2f}", rddcol, lsdData.RDD));

            extra.push_back(fmt::format("LF/RF [{:.2f}]/[{:.2f}]", lsdData.BrakeLF, lsdData.BrakeRF));
            extra.push_back(fmt::format("LR/RR [{:.2f}]/[{:.2f}]", lsdData.BrakeLR, lsdData.BrakeRR));
            extra.push_back(fmt::format("{}LSD: {}",
                lsdData.Use ? "~g~" : "~r~",
                lsdData.Use ? "Active" : "Idle/Off"));
        }

        g_menu.OptionPlusPlus(extra, "Status");
    }
}

void update_awdsettingsmenu() {
    g_menu.Title("Adaptive AWD");
    g_menu.Subtitle(MenuSubtitleConfig());

    if (!HandlingReplacement::Available()) {
        if (g_menu.Option("HandlingReplacement.asi missing",
            { "Adaptive AWD needs HandlingReplacement. Press Select to go to the download page." })) {
            WAIT(20);
            PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, ControlFrontendPause, 1.0f);
            ShellExecuteA(0, 0, "https://www.gta5-mods.com/tools/handling-replacement-library", 0, 0, SW_SHOW);
        }
    }

    bool resolveAWD = false;
    g_menu.OptionPlus("AWD Status", {}, &resolveAWD, nullptr, nullptr, "", { "Real-time AWD overview status" });
    if (resolveAWD) {
        std::vector<std::string> awdStats;
        extern Vehicle g_playerVehicle;
        if (ENTITY::DOES_ENTITY_EXIST(g_playerVehicle)) {
            awdStats = GetAWDInfo(g_playerVehicle);
        }
        else {
            awdStats = { "No current vehicle" };
        }
        g_menu.OptionPlusPlus(awdStats, "Status");
    }

    g_menu.BoolOption("Use custom drive bias", g_settings().DriveAssists.AWD.UseCustomBaseBias, 
        { "Override the front drive bias." });
    g_menu.FloatOption("Custom front drive bias", g_settings().DriveAssists.AWD.CustomBaseBias, 0.01f, 0.99f, 0.01f,
        { "Value is front bias based: 0.01 is 1% front, 99% rear. 0.99 is 99% front, 1% rear.",
          "Useful to set 1% to 9% and 91% to 99% drive biases, as GTA snaps to full FWD at more than 0.9 fDriveBiasFront and full RWD on less than 0.1 fDriveBiasFront." });

    g_menu.FloatOption("Custom bias min", g_settings().DriveAssists.AWD.CustomMin, 0.01f, 0.99f, 0.01f,
        { "Lowest value custom bias can be when manually adjusting drive bias." });
    g_menu.FloatOption("Custom bias max", g_settings().DriveAssists.AWD.CustomMax, 0.01f, 0.99f, 0.01f,
        { "Highest value custom bias can be when manually adjusting drive bias." });

    g_menu.FloatOption("Drive bias @ max transfer", g_settings().DriveAssists.AWD.BiasAtMaxTransfer, 0.01f, 0.99f, 0.01f,
        { "Value is front bias based: 0.01 is 1% front, 99% rear. 0.99 is 99% front, 1% rear.",
          "Example: if your usual Drive bias is 0.1 (so 90/10), and traction-loss transfer is enabled, a value here of 0.9 will send 90% of the torque to the front." });

    g_menu.BoolOption("On traction loss", g_settings().DriveAssists.AWD.UseTraction, 
        { "Transfer drive bias to the weaker wheels when the strong wheels break traction." } );

    g_menu.FloatOption("Speed min difference", g_settings().DriveAssists.AWD.TractionLossMin, 1.0f, 2.0f, 0.05f,
        { "Speed difference for the transfer to kick in.", "1.05 is 5% faster, 1.50 is 50% faster, etc." });
    g_menu.FloatOption("Speed max difference", g_settings().DriveAssists.AWD.TractionLossMax, 1.0f, 2.0f, 0.05f,
        { "Speed difference for max transfer.", "1.05 is 5% faster, 1.50 is 50% faster, etc." });

    // Should only be used for RWD-biased cars
    g_menu.BoolOption("On oversteer", g_settings().DriveAssists.AWD.UseOversteer,
        { "Transfer drive bias when oversteer is detected. Mostly useful for RWD-biased cars." });
    g_menu.FloatOption("Oversteer min", g_settings().DriveAssists.AWD.OversteerMin, 0.0f, 90.0f, 1.0f); // degrees
    g_menu.FloatOption("Oversteer max", g_settings().DriveAssists.AWD.OversteerMax, 0.0f, 90.0f, 1.0f); // degrees
    
    // Should only be used for FWD-biased cars
    g_menu.BoolOption("On understeer", g_settings().DriveAssists.AWD.UseUndersteer,
        { "Transfer drive bias when understeer is detected. Mostly useful for FWD-biased cars." });
    g_menu.FloatOption("Understeer min", g_settings().DriveAssists.AWD.UndersteerMin, 0.0f, 90.0f, 1.0f); // degrees
    g_menu.FloatOption("Understeer max", g_settings().DriveAssists.AWD.UndersteerMax, 0.0f, 90.0f, 1.0f); // degrees

    const std::vector<std::string> specialFlagsDescr = 
    {
        "Flags for extra features. Current flags:",
        "Bit 0: Enable torque transfer dial on y97y's BNR32",
        "Bit 1: Enable torque transfer dial on Wanted188's GT-R R32 (Remember to disable VehFuncs for torque dial)"
    };
    std::string specialFlagsStr = fmt::format("{:08X}", g_settings().DriveAssists.AWD.SpecialFlags);
    if (g_menu.Option(fmt::format("Special flags (hex): {}", specialFlagsStr), specialFlagsDescr)) {
        std::string newFlags = GetKbEntryStr(specialFlagsStr);
        SetFlags(g_settings().DriveAssists.AWD.SpecialFlags, newFlags);
    }
}

void update_cruisecontrolsettingsmenu() {
    g_menu.Title("Cruise control");
    g_menu.Subtitle(MenuSubtitleConfig());

    bool ccActive = CruiseControl::GetActive();
    if (g_menu.BoolOption("Active", ccActive,
        { "Activate or deactivate cruise control. Cancelled on throttle, brake or clutch.",
          "Can also be activated and deactivated from hotkeys or wheel buttons." })) {
        CruiseControl::SetActive(ccActive);
    }

    float speedValMul;
    float speedValRaw = g_settings().DriveAssists.CruiseControl.Speed;
    std::string speedNameUnit = GetSpeedUnitMultiplier(g_settings.HUD.Speedo.Speedo, speedValMul);
    float speedValUnit = speedValRaw * speedValMul;

    if (g_menu.FloatOptionCb(fmt::format("Speed ({})", speedNameUnit), speedValUnit, 0.0f, 500.0f, 5.0f,
        GetKbEntryFloat,
        { "Speed for cruise control. Speeds higher than vehicle top speed are ignored.",
          "Can also be changed with hotkeys or wheel buttons." })) {

        g_settings().DriveAssists.CruiseControl.Speed = speedValUnit / speedValMul;
    }

    g_menu.FloatOptionCb("Max acceleration", g_settings().DriveAssists.CruiseControl.MaxAcceleration, 0.5f, 40.0f, 0.5f, GetKbEntryFloat,
        { "How quickly cruise control accelerates or slows down to the desired speed.",
          "In m/s^2. 9.8 m/s^2 is 1G." });

    g_menu.BoolOption("Adaptive", g_settings().DriveAssists.CruiseControl.Adaptive,
        { "Adapts the speed to the car in front, if there is one." });

    g_menu.FloatOptionCb("Minimum distance", g_settings().DriveAssists.CruiseControl.MinFollowDistance, 1.0f, 50.0f, 1.0f, GetKbEntryFloat,
        { "Minimum distance to the car ahead, when stopped. In meters.",
          "Default: 5m" });

    g_menu.FloatOptionCb("Maximum distance", g_settings().DriveAssists.CruiseControl.MaxFollowDistance, 50.0f, 200.0f, 0.5f, GetKbEntryFloat,
        { "Maximum distance where adaptive cruise control starts working. In meters.",
          "Default: 100m." });

    g_menu.FloatOptionCb("Follow distance", g_settings().DriveAssists.CruiseControl.MinDistanceSpeedMult, 0.1f, 10.0f, 0.1f, GetKbEntryFloat,
        { "How close to follow the car in front.",
          "Approximates 'seconds between cars'.",
          "Lower: follows closer.",
          "Default: 1.0." });

    g_menu.FloatOptionCb("Lifting distance", g_settings().DriveAssists.CruiseControl.MaxDistanceSpeedMult, 0.1f, 10.0f, 0.1f, GetKbEntryFloat,
        { "How close to the car in front to start reducing speed.",
          "Approximates 'seconds between cars'.",
          "Lower: starts matching closer while driving.",
          "Default: 2.0." });

    g_menu.FloatOptionCb("Start brake speed margin", g_settings().DriveAssists.CruiseControl.MinDeltaBrakeMult, 0.1f, 10.0f, 0.1f, GetKbEntryFloat,
        { "How soon to start braking for the car in front.",
          "Higher: Brake for a smaller speed difference.",
          "Default: 2.4." });

    g_menu.FloatOptionCb("Full brake speed margin", g_settings().DriveAssists.CruiseControl.MaxDeltaBrakeMult, 0.1f, 10.0f, 0.1f, GetKbEntryFloat,
        { "How soon to fully brake for the car in front.",
          "Higher: Brake harder for a smaller speed difference.",
          "Keep lower than the above option.",
          "Default: 1.2." });
}

void update_speedlimitersettingsmenu() {
    g_menu.Title("Speed limiter");
    g_menu.Subtitle(MenuSubtitleConfig());

    float speedValMul;
    float speedValRaw = g_settings().MTOptions.SpeedLimiter.Speed;
    std::string speedNameUnit = GetSpeedUnitMultiplier(g_settings.HUD.Speedo.Speedo, speedValMul);
    float speedValUnit = speedValRaw * speedValMul;

    if (g_menu.FloatOptionCb(fmt::format("Max speed ({})", speedNameUnit), speedValUnit, 0.0f, 500.0f, 5.0f,
        GetKbEntryFloat,
        { "Electronically limit speed to this." })) {

        g_settings().MTOptions.SpeedLimiter.Speed = speedValUnit / speedValMul;
    }
}

void update_gameassistmenu() {
    g_menu.Title("Gameplay assists");
    g_menu.Subtitle("");

    g_menu.BoolOption("Default neutral gear", g_settings.GameAssists.DefaultNeutral,
        { "The car will be in neutral when you get in." });

    g_menu.BoolOption("Disable autostart", g_settings.GameAssists.DisableAutostart,
        { "The character will not start the vehicle when getting in." });

    g_menu.BoolOption("Leave engine running", g_settings.GameAssists.LeaveEngineRunning,
        { "The character will not turn the engine off when exiting the car, if you use a short press." });

    g_menu.BoolOption("Simplified bike", g_settings.GameAssists.SimpleBike,
        { "Disables bike engine stalling and the clutch bite simulation." });

    g_menu.BoolOption("Hill gravity workaround", g_settings.GameAssists.HillGravity,
        { "Gives the car a push to overcome the games' default brakes when stopped." });

    g_menu.BoolOption("Auto 1st gear", g_settings.GameAssists.AutoGear1,
        { "Automatically switch to first gear when the car reaches a standstill." });

    g_menu.BoolOption("Auto look back", g_settings.GameAssists.AutoLookBack,
        { "Automatically look back whenever in reverse gear." });

    g_menu.BoolOption("Clutch & throttle start", g_settings.GameAssists.ThrottleStart,
        { "Allow starting the engine by pressing clutch and throttle simultaneously." });
}

void update_steeringassistmenu() {
    g_menu.Title("Steering assists");
    g_menu.Subtitle("");

    std::vector<std::string> steeringModeDescription;
    switch(g_settings.CustomSteering.Mode) {
    case 0:
        steeringModeDescription.emplace_back("Default: Original game steering.");
        steeringModeDescription.emplace_back("Only applies Steering Multiplier.");
        break;
    case 1:
        steeringModeDescription.emplace_back("Enhanced: Custom countersteer, disabled magic forces.");
        steeringModeDescription.emplace_back("Applies all settings below.");
        break;
    default:
        steeringModeDescription.emplace_back("Invalid option?");
    }
    g_menu.StringArray("Steering mode", { "Default", "Enhanced" }, g_settings.CustomSteering.Mode, 
        steeringModeDescription);
    g_menu.FloatOptionCb("Countersteer multiplier", g_settings.CustomSteering.CountersteerMult, 0.0f, 2.0f, 0.05f, GetKbEntryFloat,
        { "How much countersteer should be given." });
    g_menu.FloatOptionCb("Countersteer limit", g_settings.CustomSteering.CountersteerLimit, 0.0f, 360.0f, 1.0f, GetKbEntryFloat,
        { "Maximum angle in degrees for automatic countersteering. Game default is 15 degrees." });
    g_menu.BoolOption("No reduction on handbrake", g_settings.CustomSteering.NoReductionHandbrake,
        { "Disable reduction when the handbrake is used." });
    g_menu.FloatOptionCb("Steering gamma", g_settings.CustomSteering.Gamma, 0.01f, 5.0f, 0.01f, GetKbEntryFloat,
        { "Change linearity of steering input." });
    g_menu.FloatOptionCb("Steering time", g_settings.CustomSteering.SteerTime, 0.000001f, 0.90f, 0.000001f,
        GetKbEntryFloat,
        { "The lower the value, the faster the steering is.",
          "Press Enter to enter a value manually." });
    g_menu.FloatOptionCb("Centering time", g_settings.CustomSteering.CenterTime, 0.000001f, 0.99f, 0.000001f,
        GetKbEntryFloat,
        { "The lower the value, the faster the wheels return to center after letting go.",
          "Press Enter to enter a value manually." });

    g_menu.MenuOption("Mouse steering options", "mousesteeringoptionsmenu");
}

void update_mousesteeringoptionsmenu() {
    g_menu.Title("Mouse steering");
    g_menu.Subtitle("");

    g_menu.BoolOption("Enable mouse steering", g_settings.CustomSteering.Mouse.Enable,
        { "When enabled, hold the mouse steering override button to use mouse steering." });

    g_menu.FloatOptionCb("Mouse sensitivity", g_settings.CustomSteering.Mouse.Sensitivity, 0.05f, 2.0f, 0.05f, GetKbEntryFloat,
        { "Sensitivity for mouse steering." });

    g_menu.BoolOption("Disable countersteer", g_settings.CustomSteering.Mouse.DisableSteerAssist,
        { "Disables countersteer assist." });

    g_menu.BoolOption("Disable reduction", g_settings.CustomSteering.Mouse.DisableReduction,
        { "Disables countersteer assist." });
}

void update_miscoptionsmenu() {
    g_menu.Title("Misc options");
    g_menu.Subtitle("");

    if (SteeringAnimation::FileProblem()) {
        g_menu.Option("Animation file error", NativeMenu::solidRed,
            { "An error occurred reading the animation file. Check Gears.log for more details." });
    }
    else {
        g_menu.BoolOption("Sync steering animation", g_settings.Misc.SyncAnimations,
            { "Synchronize animations with wheel rotation using third-person animations.",
              "Only active for synced steering wheel rotation or custom controller wheel rotation.",
              "Limits FPV camera angle, recommended to enable custom FPV camera.",
              "Check animations.yml for more details!" });
    }

    if (g_menu.BoolOption("Hide player in FPV", g_settings.Misc.HidePlayerInFPV,
        { "Hides the player in first person view.",
          "If you use another script that does the same, "
            "enable [Disable Player Hiding] in dev settings."})) {
        functionHidePlayerInFPV(true);
    }

    g_menu.BoolOption("Hide wheel in FPV", g_settings.Misc.HideWheelInFPV);

    g_menu.BoolOption("Enable dashboard extensions", g_settings.Misc.DashExtensions,
        { "If DashHook is installed, the script controls some dashboard lights such as the ABS light." });

    if (g_menu.BoolOption("Enable UDP telemetry", g_settings.Misc.UDPTelemetry,
        { "Allows programs like SimHub to use data from this script.",
            "This script uses the DIRT 4 format for telemetry data.",
            fmt::format("Endpoint: {}:{}", g_settings.Misc.UDPAddress, g_settings.Misc.UDPPort),
            "Restart the game if the endpoints are changed." })) {
        StartUDPTelemetry();
    }
}

void update_devoptionsmenu() {
    g_menu.Title("Developer options");
    g_menu.Subtitle("");

    g_menu.MenuOption("Debug settings", "debugmenu");

    g_menu.MenuOption("Metrics settings", "metricsmenu",
        { "Show the G-Force graph and speed timers." });

    g_menu.MenuOption("Compatibility settings", "compatmenu",
        { "Disable various features to improve compatibility." });

    if (g_menu.Option("Export base VehicleConfig", 
        { "Exports the base configuration to the Vehicles folder. If you lost it, or something." })) {
        const std::string saveFile = "baseVehicleConfig";
        const std::string vehConfigDir = Paths::GetModPath() + "\\Vehicles";
        const std::string finalFile = fmt::format("{}\\{}.ini", vehConfigDir, saveFile);

        VehicleConfig config;
        config.ModelNames = { "Model0", "Model1", "Model2" };
        config.Plates = { "46EEK572", "   MT   ", " NO GRIP" };
        config.Description = "This is a base configuration file with all possible sections and keys.";
        config.SaveSettings(&config, finalFile);
    }

    g_menu.BoolOption("Enable update check", g_settings.Update.EnableUpdate,
        { "Check for mod updates."});

    if (g_menu.Option("Check for updates", { "Manually check for updates. "
        "Re-enables update checking and clears ignored update." })) {
        g_settings.Update.EnableUpdate = true;
        g_settings.Update.IgnoredVersion = "v0.0.0";

        threadCheckUpdate(0);
    }

    g_menu.Option("Mod path",
        { "This script currently uses the following folder to store data:",
          fmt::format("{}", Paths::GetModPath()) });
}

void update_debugmenu() {
    g_menu.Title("Debug settings");
    g_menu.Subtitle("Under the hood");

    g_menu.BoolOption("Show legacy FFB options", g_settings.Debug.ShowAdvancedFFBOptions,
        { "Show legacy options in FFB menu." });
    g_menu.BoolOption("Display debug info", g_settings.Debug.DisplayInfo,
        { "Show all detailed technical info of the gearbox and inputs calculations." });
    g_menu.BoolOption("Display wheel drive info", g_settings.Debug.DisplayWheelInfo,
        { "Show wheel info with power, brake, .",
          "[TCS: Yellow] [ESP: Blue] [ABS: Red] [Locked up: Purple] [Off ground: Transparent]"});
    g_menu.BoolOption("Display wheel material info", g_settings.Debug.DisplayMaterialInfo,
        { "Show material info and various material multipliers.",
          "Note: WET_GRIP is displayed as 1.0f - (WET_GRIP * Wetness)." });
    g_menu.BoolOption("Display wheel traction info", g_settings.Debug.DisplayTractionInfo,
        { "Show traction/force vectors (?). Green = forward, red = backward." });
    g_menu.BoolOption("Display gearing info", g_settings.Debug.DisplayGearingInfo,
        { "Show gear ratios and shift points from auto mode." });
    g_menu.BoolOption("Show NPC info", g_settings.Debug.DisplayNPCInfo,
        { "Show vehicle info of NPC vehicles near you." });

    if (SteeringAnimation::FileProblem()) {
        g_menu.Option("Animation file error", NativeMenu::solidRed, 
            { "An error occurred reading the animation file. Check Gears.log for more details." });
    }
    else {
        std::vector <std::string> extras;

        extras.emplace_back("Available animation dictionaries:");

        const auto& anims = SteeringAnimation::GetAnimations();
        const size_t index = SteeringAnimation::GetAnimationIndex();
        for (size_t i = 0; i < anims.size(); ++i) {
            const auto& anim = anims[i];
            std::string mark = "[ ]";
            if (i == index) {
                mark = "[*]";
            }
            extras.emplace_back(fmt::format("{} {}", mark, anim.Dictionary));
        }

        extras.emplace_back("");
        extras.emplace_back("* marks active dictionary.");
        if (index >= SteeringAnimation::GetAnimations().size()) {
            extras.push_back(fmt::format("Index out of range ({})", index));
        }

        extras.emplace_back("");
        extras.emplace_back("Press left/right to change animation manually.");

        std::function<void()> onLeft = [index, anims]() {
            if (!anims.empty()) {
                if (index == 0) {
                    // Set to "none"
                    SteeringAnimation::SetAnimationIndex(anims.size());
                }
                else {
                    SteeringAnimation::SetAnimationIndex(index - 1);
                }
            }
        };

        std::function<void()> onRight = [index, anims]() {
            if (!anims.empty()) {
                // allow 1 past, to set to none
                if (index >= anims.size()) {
                    SteeringAnimation::SetAnimationIndex(0);
                }
                else {
                    SteeringAnimation::SetAnimationIndex(index + 1);
                }
            }
        };

        if (g_menu.OptionPlus("Animation info", extras,
            nullptr, onRight, onLeft, "Animations", 
            { "Shows current animation override status. Enter to reload." })) {
            SteeringAnimation::Load();
        }
    }

    {
        auto fetchInfo = [](std::vector<std::string>& diDevicesInfo_) {
            logger.Write(DEBUG, "Re-scanning DirectInput devices");
            diDevicesInfo_.clear();

            g_controls.GetWheel().InitWheel();
            const auto& devices = g_controls.GetWheel().GetDevices();
            diDevicesInfo_.push_back(fmt::format("Devices: {}", devices.size()));
            diDevicesInfo_.push_back("");

            for (const auto& [guid, device] : devices) {
                diDevicesInfo_.push_back(fmt::format("{}", device.DeviceInstance.tszInstanceName));
                diDevicesInfo_.push_back(fmt::format("    GUID: {}", GUID2String(guid)));
                diDevicesInfo_.push_back(fmt::format("    Type: 0x{:X}", device.DeviceCapabilities.dwDevType));
                diDevicesInfo_.push_back(fmt::format("    FFB: {}", device.DeviceCapabilities.dwFlags & DIDC_FORCEFEEDBACK));
                diDevicesInfo_.push_back("");
            }
        };

        bool selected = false;
        if (g_menu.OptionPlus("DirectInput devices", diDevicesInfo, 
            &selected, nullptr, nullptr, "DirectInput info", { "Enter to refresh." })) {
            fetchInfo(diDevicesInfo);
        }

        if (selected) {
            g_menu.OptionPlusPlus(diDevicesInfo, "DirectInput info");
        }
    }

    g_menu.BoolOption("Disable general RPM limit", g_settings.Debug.DisableRPMLimit,
        { "Disables redline RPM limit in all gears, allowing for unlimited acceleration in any gear." });
}

void update_metricsmenu() {
    g_menu.Title("Metrics settings");
    g_menu.Subtitle("");

    g_menu.BoolOption("Display G force meter", g_settings.Debug.Metrics.GForce.Enable,
        { "Show a graph with G forces.", 
            "Change the screen coordinates and sizes in settings_general.ini." });

    if (g_menu.BoolOption("Enable timers", g_settings.Debug.Metrics.EnableTimers,
        { "Enable speed timers as defined in settings_general.ini, [DEBUG]. Example:",
            "Timer0Unit = kph",
            "Timer0LimA = 0.0",
            "Timer0LimB = 120.0",
            "Timer0Tolerance = 0.1"})) {
        saveAllSettings();
        g_settings.Read(&g_controls);
        initTimers();
    }

    if (g_settings.Debug.Metrics.EnableTimers) {
        for (const auto& timerParams : g_settings.Debug.Metrics.Timers) {
            g_menu.Option(fmt::format("Timer {}-{} {} (+/- {})",
                timerParams.LimA,
                timerParams.LimB,
                timerParams.Unit,
                timerParams.Tolerance));
        }
    }
}

void update_compatmenu() {
    g_menu.Title("Compatibility settings");
    g_menu.Subtitle("");

    g_menu.BoolOption("Disable NPC gearbox", g_settings.Debug.DisableNPCGearbox,
        { "Disable the script from controlling NPC vehicle gearboxes, when Manual Transmission is enabled.",
            "Disabling makes NPC vehicles drive unpredictably and never shift up.",
            "Use the script API to disable script control on a per-vehicle basis, "
            "if you're a developer and wish to implement your own NPC vehicle shifting logic." });

    g_menu.BoolOption("Disable NPC brakes", g_settings.Debug.DisableNPCBrake,
        { "While ABS, TCS or ESC are active, NPC braking is replaced by script.",
            "Disabling hampers AI braking, but can improve performance." });

    g_menu.BoolOption("Disable input detection", g_settings.Debug.DisableInputDetect,
        {  "Disable automatic detection and switching of input method.",
            "Allows for manual input selection on the main menu." });

    g_menu.BoolOption("Disable player hiding", g_settings.Debug.DisablePlayerHide,
        { "Disables toggling player visibility by script.",
            "Useful when another script hides the player." });
}

void update_menu() {
    g_menu.CheckKeys();

    /* mainmenu */
    if (g_menu.CurrentMenu("mainmenu")) { update_mainmenu(); }

    /* mainmenu -> settingsmenu */
    if (g_menu.CurrentMenu("settingsmenu")) { update_settingsmenu(); }

    /* mainmenu -> settingsmenu -> featuresmenu */
    if (g_menu.CurrentMenu("featuresmenu")) { update_featuresmenu(); }

    /* mainmenu -> settingsmenu -> featuresmenu -> speedlimitersettingsmenu */
    if (g_menu.CurrentMenu("speedlimitersettingsmenu")) { update_speedlimitersettingsmenu(); }

    /* mainmenu -> settingsmenu -> finetuneoptionsmenu */
    if (g_menu.CurrentMenu("finetuneoptionsmenu")) { update_finetuneoptionsmenu(); }

    /* mainmenu -> settingsmenu -> shiftingoptionsmenu */
    if (g_menu.CurrentMenu("shiftingoptionsmenu")) { update_shiftingoptionsmenu(); }

    /* mainmenu -> settingsmenu -> finetuneautooptionsmenu */
    if (g_menu.CurrentMenu("finetuneautooptionsmenu")) { update_finetuneautooptionsmenu(); }

    /* mainmenu -> vehconfigmenu */
    if (g_menu.CurrentMenu("vehconfigmenu")) { update_vehconfigmenu(); }

    /* mainmenu -> controlsmenu */
    if (g_menu.CurrentMenu("controlsmenu")) { update_controlsmenu(); }

    /* mainmenu -> controlsmenu -> controllermenu */
    if (g_menu.CurrentMenu("controllermenu")) { update_controllermenu(); }

    /* mainmenu -> controlsmenu -> controllermenu -> controllerbindingsnativemenu */
    if (g_menu.CurrentMenu("controllerbindingsnativemenu")) { update_controllerbindingsnativemenu(); }

    /* mainmenu -> controlsmenu -> controllermenu -> controllerbindingsxinputmenu */
    if (g_menu.CurrentMenu("controllerbindingsxinputmenu")) { update_controllerbindingsxinputmenu(); }

    /* mainmenu -> controlsmenu -> keyboardmenu */
    if (g_menu.CurrentMenu("keyboardmenu")) { update_keyboardmenu(); }

    /* mainmenu -> controlsmenu -> steeringassistmenu */
    if (g_menu.CurrentMenu("steeringassistmenu")) { update_steeringassistmenu(); }

    /* mainmenu -> controlsmenu -> steeringassistmenu -> mousesteeringoptionsmenu */
    if (g_menu.CurrentMenu("mousesteeringoptionsmenu")) { update_mousesteeringoptionsmenu(); }

    /* mainmenu -> controlsmenu -> wheelmenu */
    if (g_menu.CurrentMenu("wheelmenu")) { update_wheelmenu(); }

    /* mainmenu -> controlsmenu -> wheelmenu -> anglemenu */
    if (g_menu.CurrentMenu("anglemenu")) { update_anglemenu(); }

    /* mainmenu -> controlsmenu -> wheelmenu -> axesmenu */
    if (g_menu.CurrentMenu("axesmenu")) { update_axesmenu(); }

    /* mainmenu -> controlsmenu -> wheelmenu -> forcefeedbackmenu */
    if (g_menu.CurrentMenu("forcefeedbackmenu")) { update_forcefeedbackmenu(); }

    /* mainmenu -> controlsmenu -> wheelmenu -> forcefeedbackmenu */
    if (g_menu.CurrentMenu("ffbnormalizationmenu")) { update_ffbnormalizationmenu(); }

    /* mainmenu -> controlsmenu -> wheelmenu -> buttonsmenu */
    if (g_menu.CurrentMenu("buttonsmenu")) { update_buttonsmenu(); }

    /* mainmenu -> controlsmenu -> update_controlsvehconfmenu */
    if (g_menu.CurrentMenu("controlsvehconfmenu")) { update_controlsvehconfmenu(); }

    /* mainmenu -> hudmenu */
    if (g_menu.CurrentMenu("hudmenu")) { update_hudmenu(); }

    /* mainmenu -> hudmenu -> geardisplaymenu*/
    if (g_menu.CurrentMenu("geardisplaymenu")) { update_geardisplaymenu(); }

    /* mainmenu -> hudmenu -> speedodisplaymenu*/
    if (g_menu.CurrentMenu("speedodisplaymenu")) { update_speedodisplaymenu(); }

    /* mainmenu -> hudmenu -> rpmdisplaymenu*/
    if (g_menu.CurrentMenu("rpmdisplaymenu")) { update_rpmdisplaymenu(); }

    /* mainmenu -> hudmenu -> wheelinfomenu*/
    if (g_menu.CurrentMenu("wheelinfomenu")) { update_wheelinfomenu(); }

    /* mainmenu -> hudmenu -> dashindicatormenu*/
    if (g_menu.CurrentMenu("dashindicatormenu")) { update_dashindicatormenu(); }

    /* mainmenu -> hudmenu -> dsprotmenu*/
    if (g_menu.CurrentMenu("dsprotmenu")) { update_dsprotmenu(); }

    /* mainmenu -> hudmenu -> mousehudmenu*/
    if (g_menu.CurrentMenu("mousehudmenu")) { update_mousehudmenu(); }

    /* mainmenu -> driveassistmenu */
    if (g_menu.CurrentMenu("driveassistmenu")) { update_driveassistmenu(); }

    /* mainmenu -> driveassistmenu -> abssettingsmenu */
    if (g_menu.CurrentMenu("abssettingsmenu")) { update_abssettingsmenu(); }

    /* mainmenu -> driveassistmenu -> tcssettingsmenu */
    if (g_menu.CurrentMenu("tcssettingsmenu")) { update_tcssettingsmenu(); }

    /* mainmenu -> driveassistmenu -> espsettingsmenu */
    if (g_menu.CurrentMenu("espsettingsmenu")) { update_espsettingsmenu(); }

    /* mainmenu -> driveassistmenu -> lcssettings */
    if (g_menu.CurrentMenu("lcssettings")) { update_lcssettingsmenu(); }

    /* mainmenu -> driveassistmenu -> lsdsettingsmenu */
    if (g_menu.CurrentMenu("lsdsettingsmenu")) { update_lsdsettingsmenu(); }

    /* mainmenu -> driveassistmenu -> awdsettingsmenu */
    if (g_menu.CurrentMenu("awdsettingsmenu")) { update_awdsettingsmenu(); }

    /* mainmenu -> driveassistmenu -> cruisecontrolsettingsmenu */
    if (g_menu.CurrentMenu("cruisecontrolsettingsmenu")) { update_cruisecontrolsettingsmenu(); }

    /* mainmenu -> gameassistmenu */
    if (g_menu.CurrentMenu("gameassistmenu")) { update_gameassistmenu(); }

    /* mainmenu -> miscoptionsmenu */
    if (g_menu.CurrentMenu("miscoptionsmenu")) { update_miscoptionsmenu(); }

    /* mainmenu -> devoptionsmenu */
    if (g_menu.CurrentMenu("devoptionsmenu")) { update_devoptionsmenu(); }

    /* mainmenu -> devoptionsmenu -> debugmenu */
    if (g_menu.CurrentMenu("debugmenu")) { update_debugmenu(); }

    /* mainmenu -> devoptionsmenu -> metricsmenu */
    if (g_menu.CurrentMenu("metricsmenu")) { update_metricsmenu(); }

    /* mainmenu -> devoptionsmenu -> compatmenu */
    if (g_menu.CurrentMenu("compatmenu")) { update_compatmenu(); }

    g_menu.EndMenu();
}
