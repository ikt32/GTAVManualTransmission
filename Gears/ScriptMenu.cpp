#include "script.h"

#include "ScriptSettings.hpp"
#include "Input/CarControls.hpp"
#include "Input/NativeController.h"
#include "Input/keyboard.h"
#include "Util/UIUtils.h"
#include "Util/Util.hpp"
#include "Constants.h"
#include "UpdateChecker.h"
#include "Memory/MemoryPatcher.hpp"
#include "Memory/VehicleExtensions.hpp"
#include "Util/MathExt.h"
#include "Util/Logger.hpp"
#include "ScriptUtils.h"
#include "VehicleConfig.h"
#include "SteeringAnim.h"
#include "Camera.h"

#include <menu.h>
#include <menukeyboard.h>
#include <inc/main.h>
#include <inc/natives.h>

#include <fmt/format.h>

#include <Windows.h>
#include <shellapi.h>
#include <string>
#include <mutex>

#include "Compatibility.h"

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
extern VehicleConfig* g_activeConfig;

struct SFont {
    int ID;
    std::string Name;
};

struct STagInfo {
    std::string Tag;
    std::string Info;
};

template <typename T>
struct SControlText {
    T Control;
    std::string Text;
};

void clearAxis(const std::string& confTag);
void clearButton(const std::string& confTag);
void clearWheelToKey();
void clearHShifter();
void clearASelect();
void clearKeyboardKey(const std::string& confTag);
void clearControllerButton(const std::string& confTag);
void clearLControllerButton(const std::string& confTag);

bool configAxis(const std::string& confTag);
bool configWheelToKey();
bool configButton(const std::string& confTag);
bool configHPattern();
bool configASelect();
bool configKeyboardKey(const std::string& confTag);
bool configControllerButton(const std::string& confTag);
bool configLControllerButton(const std::string& confTag);

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

    const std::vector<SControlText<int>> blockableControls {
        { -1,                             "None"},
        { ControlVehicleAim,              "Aim"},
        { ControlVehicleHandbrake,        "Handbrake"},
        { ControlVehicleAttack,           "Attack"},
        { ControlVehicleDuck,             "Duck"},
        { ControlVehicleSelectNextWeapon, "NextWeapon"},
        { ControlVehicleCinCam,           "Cinematic Cam"},
        { ControlVehicleExit,             "Exit Car"}
    };

    const std::vector<std::string> tcsStrings{
        "Brakes", "Throttle"
    };

    const std::vector<std::string> absStrings = {
        "Disabled", "On if missing", "Always on"
    };

    const std::vector<std::string> notifyLevelStrings{
        "Debug",
        "Info",
        "UI",
        "None"
    };

    std::vector<std::string> camAttachPoints {
        "Player head",
        "Vehicle",
        "Vanilla FPV"
    };

    bool getKbEntry(float& val) {
        UI::Notify(INFO, "Enter value");
        MISC::DISPLAY_ONSCREEN_KEYBOARD(LOCALIZATION::GET_CURRENT_LANGUAGE() == 0, "FMMC_KEY_TIP8", "",
            fmt::format("{:f}", val).c_str(), "", "", "", 64);
        while (MISC::UPDATE_ONSCREEN_KEYBOARD() == 0) {
            WAIT(0);
        }
        if (!MISC::GET_ONSCREEN_KEYBOARD_RESULT()) {
            UI::Notify(INFO, "Cancelled value entry");
            return false;
        }

        std::string floatStr = MISC::GET_ONSCREEN_KEYBOARD_RESULT();
        if (floatStr.empty()) {
            UI::Notify(INFO, "Cancelled value entry");
            return false;
        }

        char* pEnd;
        float parsedValue = strtof(floatStr.c_str(), &pEnd);

        if (parsedValue == 0.0f && *pEnd != 0) {
            UI::Notify(INFO, "Failed to parse entry.");
            return false;
        }

        val = parsedValue;
        return true;
    }

    std::vector<std::string> diDevicesInfo { "Press Enter to refresh." };
}

int getBlockableControlIndex(int control) {
    for (size_t i = 0; i < blockableControls.size(); i++) {
        if (control == blockableControls[i].Control)
            return static_cast<int>(i);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//                             Menu stuff
///////////////////////////////////////////////////////////////////////////////
void onMenuInit() {
    g_menu.ReadSettings();
}

void saveChanges() {
    g_settings.SaveGeneral();
    g_settings.SaveController(&g_controls);
    g_settings.SaveWheel();
}

void onMenuClose() {
    saveChanges();

    if (g_activeConfig) {
        auto tempShiftMode = g_activeConfig->MTOptions.ShiftMode;
        loadConfigs();
        setShiftMode(tempShiftMode);
    }
    else {
        loadConfigs();
    }
}

void update_mainmenu() {
    g_menu.Title("Manual Transmission");
    g_menu.Subtitle(fmt::format("~b~{}", Constants::DisplayVersion));

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
                    saveChanges();
                    }, nullptr, "Update info",
                    { "Press Select/Enter to check GTA5-Mods.com.",
                        "Press right to ignore the current update." })) {
                    WAIT(20);
                    PAD::_SET_CONTROL_NORMAL(0, ControlFrontendPause, 1.0f);
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

    int tempShiftMode = EToInt(g_settings().MTOptions.ShiftMode);

    std::vector<std::string> detailsTemp {
        "Choose your gearbox! Options are Sequential, H-pattern and Automatic."
    };

    if (g_settings.MTOptions.Override && g_activeConfig != nullptr) {
        detailsTemp.push_back(fmt::format("Temporarily change shift mode for current override: [{}]", g_activeConfig->Name));
    }

    g_menu.StringArray("Gearbox", gearboxModes, tempShiftMode,
        detailsTemp);

    if (tempShiftMode != EToInt(g_settings().MTOptions.ShiftMode)) {
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
        g_menu.StringArray("Active input", { activeInputName }, activeIndex, { "Active input is automatically detected and can't be changed." });
    }

    for (const auto& device : g_controls.FreeDevices) {
        if (g_menu.Option(device.name, NativeMenu::solidRed,
            { "~r~<b>This device needs to be configured!</b>~w~",
              "Please assign axes and buttons in ~r~<b>Steering Wheel</b>~w~->"
              "~r~<b>Analog Input Setup</b>~w~, ~r~<b>Button Input Setup</b>~w~ before using this device.",
              "Pressing Select discards this warning.",
              fmt::format("Full name: {}", device.name) })) {
            saveChanges();
            g_settings.SteeringAppendDevice(device.guid, device.name);
            g_settings.Read(&g_controls);
            g_controls.CheckGUIDs(g_settings.Wheel.InputDevices.RegisteredGUIDs);
        }
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

    g_menu.MenuOption("Custom vehicle settings", "vehconfigmenu",
        { "Configurations overriding mod-wide settings, for specific vehicles." });
}

void update_featuresmenu() {
    g_menu.Title("Features");
    g_menu.Subtitle("");
    g_menu.BoolOption("Engine Damage", g_settings.MTOptions.EngDamage,
        { "Damage the engine when over-revving and when mis-shifting." });

    g_menu.BoolOption("Engine stalling (H)", g_settings.MTOptions.EngStallH,
        { "Stall the engine when the wheel speed gets too low. Applies to H-pattern shift mode." });

    g_menu.BoolOption("Engine stalling (S)", g_settings.MTOptions.EngStallS,
        { "Stall the engine when the wheel speed gets too low. Applies to sequential shift mode." });

    g_menu.BoolOption("Clutch shift (H)", g_settings.MTOptions.ClutchShiftH,
        { "Require holding the clutch to shift in H-pattern shift mode." });

    g_menu.BoolOption("Clutch shift (S)", g_settings.MTOptions.ClutchShiftS,
        { "Require holding the clutch to shift in sequential mode." });

    g_menu.BoolOption("Engine braking", g_settings.MTOptions.EngBrake,
        { "Help the car braking by slowing down more at high RPMs." });

    g_menu.BoolOption("Gear/RPM lockup", g_settings.MTOptions.EngLock,
        { "Simulate wheel lock-up when mis-shifting to a too low gear for the RPM range." });

    g_menu.BoolOption("Clutch creep", g_settings.MTOptions.ClutchCreep,
        { "Simulate clutch creep when stopped with clutch engaged." });

    g_menu.BoolOption("Hard rev limiter", g_settings.MTOptions.HardLimiter,
        { "Enforce rev limiter for reverse and top speed. No more infinite speed!" });
}

void update_finetuneoptionsmenu() {
    g_menu.Title("Finetuning");
    g_menu.Subtitle("");

    g_menu.FloatOption("Clutch bite threshold", g_settings.MTParams.ClutchThreshold, 0.0f, 1.0f, 0.05f,
        { "How far the clutch has to be lifted to start biting." });
    g_menu.FloatOption("Stalling RPM", g_settings.MTParams.StallingRPM, 0.0f, 0.2f, 0.01f,
        { "Consider stalling when the expected RPM drops below this value.",
          "The range is 0.0 to 0.2. The engine idles at 0.2 in GTA.",
          "Expected RPM is based on car speed and current gear ratio."});
    g_menu.FloatOption("Stalling rate", g_settings.MTParams.StallingRate, 0.0f, 10.0f, 0.05f,
        { "How quick the engine should stall. Higher values make it stall faster." });
    g_menu.FloatOption("Stalling slip", g_settings.MTParams.StallingSlip, 0.0f, 1.0f, 0.05f,
        { "How much the clutch may slip before the stalling rate picks up." });

    g_menu.FloatOption("RPM damage", g_settings.MTParams.RPMDamage, 0.0f, 10.0f, 0.05f,
        { "Damage from redlining too long." });
    g_menu.FloatOption("Misshift damage", g_settings.MTParams.MisshiftDamage, 0, 100, 5,
        { "Damage from being over the rev range." });
    g_menu.FloatOption("Engine braking threshold", g_settings.MTParams.EngBrakeThreshold, 0.0f, 1.0f, 0.05f,
        { "RPM where engine braking starts being effective." });
    g_menu.FloatOption("Engine braking power", g_settings.MTParams.EngBrakePower, 0.0f, 5.0f, 0.05f,
        { "Decrease this value if your wheels lock up when engine braking." });

    // Clutch creep params
    g_menu.FloatOption("Clutch creep idle RPM", g_settings.MTParams.CreepIdleRPM, 0.0f, 0.5f, 0.01f,
        { "RPM at which the engine should be idling and the car wants to move by itself.",
          "Engines in GTA idle at 0.2, but this is rather high."});
    g_menu.FloatOption("Clutch creep throttle", g_settings.MTParams.CreepIdleThrottle, 0.0f, 1.0f, 0.01f,
        { "How much throttle is given when the car speed drops below the idle RPM." });
}

void update_shiftingoptionsmenu() {
    g_menu.Title("Shifting options");
    g_menu.Subtitle("");

    g_menu.BoolOption("Cut throttle on upshift", g_settings.ShiftOptions.UpshiftCut,
        { "Helps rev matching.",
            "Only applies to sequential mode."});
    g_menu.BoolOption("Blip throttle on downshift", g_settings.ShiftOptions.DownshiftBlip,
        { "Helps rev matching.",
            "Only applies to sequential mode." });
    g_menu.FloatOption("Clutch rate multiplier", g_settings.ShiftOptions.ClutchRateMult, 0.05f, 20.0f, 0.05f,
        { "Change how fast clutching is. Below 1 is slower, higher than 1 is faster.",
            "Applies to sequential and automatic mode." });

    g_menu.FloatOption("Shifting RPM tolerance", g_settings.ShiftOptions.RPMTolerance, 0.0f, 1.0f, 0.05f,
        { "RPM mismatch tolerance on shifts",
            "Only applies to H-pattern with \"Clutch Shift\" enabled.",
            fmt::format("Clutch Shift (H) is {}abled.", g_settings.MTOptions.ClutchShiftH ? "~g~en" : "~r~dis")
        });
}

void update_finetuneautooptionsmenu() {
    g_menu.Title("Automatic finetuning");
    g_menu.Subtitle("");

    g_menu.FloatOption("Upshift engine load", g_settings.AutoParams.UpshiftLoad, 0.01f, 0.20f, 0.01f,
        { "Upshift when the engine load drops below this value. "
          "Raise this value if the car can't upshift."});
    g_menu.FloatOption("Downshift engine load", g_settings.AutoParams.DownshiftLoad, 0.30f, 1.00f, 0.01f,
        { "Downshift when the engine load rises over this value. "
          "Raise this value if the car downshifts right after upshifting." });
    g_menu.FloatOption("Downshift timeout multiplier", g_settings.AutoParams.DownshiftTimeoutMult, 0.05f, 10.00f, 0.05f,
        { "Don't downshift while car has just shifted up. "
          "Timeout based on clutch change rate.",
          "Raise for longer timeout, lower to allow earlier downshifting after an upshift." });
    g_menu.FloatOption("Next gear min RPM",    g_settings.AutoParams.NextGearMinRPM, 0.20f, 0.50f, 0.01f, 
        { "Don't upshift until next gears' RPM is over this value." });
    g_menu.FloatOption("Current gear min RPM", g_settings.AutoParams.CurrGearMinRPM, 0.20f, 0.50f, 0.01f, 
        { "Downshift when RPM drops below this value." });
    g_menu.FloatOption("Economy rate", g_settings.AutoParams.EcoRate, 0.01f, 0.50f, 0.01f,
        { "On releasing throttle, high values cause earlier upshifts.",
          "Set this low to stay in gear longer when releasing throttle." });
    g_menu.BoolOption("Using ATCU (experimental)", g_settings.AutoParams.UsingATCU,
        { "Using experimental new Automatic Transmission Control Unit by Nyconing.",
          "ATCU is configurationless, the above settings are ignored." });
}

std::vector<std::string> formatVehicleConfig(const VehicleConfig& config) {
    std::string modelNames;
    for (auto it = config.ModelNames.begin(); it != config.ModelNames.end(); ++it) {
        modelNames += fmt::format("[{}]", *it);
        if (std::next(it) != config.ModelNames.end())
            modelNames += " ";
    }
    if (config.ModelNames.empty())
        modelNames = "None";

    std::string plates;
    for (auto it = config.Plates.begin(); it != config.Plates.end(); ++it) {
        plates += fmt::format("[{}]", *it);
        if (std::next(it) != config.Plates.end())
            plates += " ";
    }
    if (config.Plates.empty())
        plates = "None";

    unsigned absMode = 0;
    if (config.DriveAssists.ABS.Enable) {
        if (config.DriveAssists.ABS.Filter) {
            absMode = 1;
        }
        else {
            absMode = 2;
        }
    }

    std::string shiftAssist;
    if (config.ShiftOptions.UpshiftCut)
        shiftAssist += "Up";
    if (config.ShiftOptions.DownshiftBlip)
        shiftAssist += " & Down";
    if (shiftAssist.empty())
        shiftAssist = "None";

    std::vector<std::string> extras{
        fmt::format("\t{}", config.Description),
        "Compatible cars:",
        fmt::format("\tModels: {}", modelNames),
        fmt::format("\tPlates: {}", plates),
        "Shifting options:",
        fmt::format("\tShift mode: {}", gearboxModes[static_cast<int>(config.MTOptions.ShiftMode)]),
        fmt::format("\tClutch creep: {}", config.MTOptions.ClutchCreep),
        fmt::format("\tSequential assist: {}", shiftAssist),
        "Driving assists:",
        fmt::format("\tABS: {}", absStrings[absMode]),
        fmt::format("\tTCS: {}", tcsStrings[config.DriveAssists.TCS.Mode]),
        "Steering wheel:",
        fmt::format("\tAngle: {:.0f}", config.Wheel.Steering.Angle),
        fmt::format("\tFFB Mult: {:.0f}", config.Wheel.FFB.SATAmpMult),
        fmt::format("\tFFB Limit: {}", config.Wheel.FFB.SATMax)
    };
    return extras;
}

void update_vehconfigmenu() {
    g_menu.Title("Custom vehicle settings");
    std::string cfgName = "No active override";
    if (g_activeConfig != nullptr)
        cfgName = g_activeConfig->Name;
    g_menu.Subtitle(cfgName);

    g_menu.BoolOption("Enable overrides", g_settings.MTOptions.Override);

    if (g_activeConfig != nullptr) {
        bool sel = false;
        g_menu.OptionPlus(fmt::format("[{}] Overview", g_activeConfig->Name), {}, &sel, nullptr, nullptr, "", {
            "For more info, check the file itself." });
        if (sel) {
            auto extras = formatVehicleConfig(*g_activeConfig);
            g_menu.OptionPlusPlus(extras, "Config overview");
        }
    }
    else {
        g_menu.Option("No active override");
    }

    for (const auto& vehConfig : g_vehConfigs) {
        bool sel = false;
        g_menu.OptionPlus(vehConfig.Name, {}, &sel, nullptr, nullptr, "");
        if (sel) {
            auto extras = formatVehicleConfig(vehConfig);
            g_menu.OptionPlusPlus(extras, "Config overview");
        }
    }

    if (g_vehConfigs.empty()) {
        g_menu.OptionPlus("No configurations found", {}, nullptr, nullptr, nullptr, "Instructions", {
            "Overrides allow you to specify custom rules for specific vehicles, that override the global settings.",
            "For example, you can set different default shift modes, set which assists are active, or use different force feedback profiles for each car.",
            R"(Check the "Vehicles" folder inside the "ManualTransmission folder for more information!")"
            });
    }

    if (g_menu.Option("Reload configs")) {
        loadConfigs();
    }
}


void update_controlsmenu() {
    g_menu.Title("Controls");
    g_menu.Subtitle("");

    g_menu.MenuOption("Controller", "controllermenu");

    g_menu.MenuOption("Keyboard", "keyboardmenu");

    g_menu.MenuOption("Wheel & pedals", "wheelmenu");

    g_menu.MenuOption("Steering assists", "steeringassistmenu",
        { "Customize steering input for keyboards and controllers." });
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
}

void update_controllerbindingsnativemenu() {
    g_menu.Title("Controller bindings");
    g_menu.Subtitle("Native mode");

    std::vector<std::string> blockableControlsHelp;
    blockableControlsHelp.reserve(blockableControls.size());
    for (const auto& control : blockableControls) {
        blockableControlsHelp.emplace_back(control.Text);
    }

    int oldIndexUp = getBlockableControlIndex(g_controls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)]);
    if (g_menu.StringArray("Shift Up blocks", blockableControlsHelp, oldIndexUp)) {
        g_controls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)] = blockableControls[oldIndexUp].Control;
    }

    int oldIndexDown = getBlockableControlIndex(g_controls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)]);
    if (g_menu.StringArray("Shift Down blocks", blockableControlsHelp, oldIndexDown)) {
        g_controls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)] = blockableControls[oldIndexDown].Control;
    }

    int oldIndexClutch = getBlockableControlIndex(g_controls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)]);
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
    blockableControlsHelp.reserve(blockableControls.size());
    for (const auto& control : blockableControls) {
        blockableControlsHelp.emplace_back(control.Text);
    }

    int oldIndexUp = getBlockableControlIndex(g_controls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftUp)]);
    if (g_menu.StringArray("Shift Up blocks", blockableControlsHelp, oldIndexUp)) {
        g_controls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftUp)] = blockableControls[oldIndexUp].Control;
    }

    int oldIndexDown = getBlockableControlIndex(g_controls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftDown)]);
    if (g_menu.StringArray("Shift Down blocks", blockableControlsHelp, oldIndexDown)) {
        g_controls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftDown)] = blockableControls[oldIndexDown].Control;
    }

    int oldIndexClutch = getBlockableControlIndex(g_controls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::Clutch)]);
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
        keyboardInfo.push_back(fmt::format("Assigned to {}", key2str(input.Control)));
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
    auto deviceEntry = g_controls.GetWheel().FindEntryFromGUID(wheelGuid);
    std::string wheelName = "No wheel";
    if (deviceEntry) {
        std::wstring wDevName = deviceEntry->diDeviceInstance.tszInstanceName;
        wheelName = StrUtil::utf8_encode(wDevName);
    }
    g_menu.Subtitle(wheelName);

    if (g_menu.BoolOption("Enable wheel", g_settings.Wheel.Options.Enable,
        { "Enable usage of a steering wheel and pedals." })) {
        saveChanges();
        g_settings.Read(&g_controls);
        initWheel();
    }

    g_menu.BoolOption("Sync steering wheel rotation", g_settings.Wheel.Options.SyncRotation, 
        { "Sync the cars' steering wheel with your actual steering wheel." });

    if (g_menu.FloatOption("Steering multiplier (wheel)", g_settings.Wheel.Steering.SteerMult, 0.1f, 2.0f, 0.01f,
        { "Increase steering lock for all cars. You might want to increase it for faster steering and more steering lock." })) {
    }

    if (g_menu.BoolOption("Logitech RPM LEDs", g_settings.Wheel.Options.LogiLEDs,
        { "Show the RPM LEDs on Logitech steering wheels. If the wheel doesn't have compatible RPM LEDs, this might crash." })) {
        g_settings.SaveWheel();
    }

    g_menu.MenuOption("Analog input setup", "axesmenu",
        { "Configure the analog inputs." });

    g_menu.MenuOption("Button input setup", "buttonsmenu",
        { "Set up your buttons on your steering wheel." });

    g_menu.MenuOption("Force feedback options", "forcefeedbackmenu",
        { "Fine-tune your force feedback parameters." });

    g_menu.MenuOption("Soft lock options", "anglemenu",
        { "Set up soft lock options here." });

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

    g_menu.BoolOption("Use shifter for automatic", g_settings.Wheel.Options.UseShifterForAuto,
        { "Use the H-pattern shifter to select the Drive/Reverse gears." });
}

void update_anglemenu() {
    g_menu.Title("Soft lock");
    g_menu.Subtitle("");
    float minLock = 180.0f;
    if (g_menu.FloatOption("Physical degrees", g_settings.Wheel.Steering.AngleMax, minLock, 1440.0, 30.0,
        { "How many degrees your wheel can physically turn." })) {
        if (g_settings.Wheel.Steering.AngleCar > g_settings.Wheel.Steering.AngleMax) { g_settings.Wheel.Steering.AngleCar = g_settings.Wheel.Steering.AngleMax; }
        if (g_settings.Wheel.Steering.AngleBike > g_settings.Wheel.Steering.AngleMax) { g_settings.Wheel.Steering.AngleBike = g_settings.Wheel.Steering.AngleMax; }
        if (g_settings.Wheel.Steering.AngleBoat > g_settings.Wheel.Steering.AngleMax) { g_settings.Wheel.Steering.AngleBoat = g_settings.Wheel.Steering.AngleMax; }
    }
    g_menu.FloatOption("Car soft lock", g_settings.Wheel.Steering.AngleCar, minLock, g_settings.Wheel.Steering.AngleMax, 30.0,
        { "Soft lock for cars and trucks. (degrees)" });

    g_menu.FloatOption("Bike soft lock", g_settings.Wheel.Steering.AngleBike, minLock, g_settings.Wheel.Steering.AngleMax, 30.0,
        { "Soft lock for bikes and quads. (degrees)" });

    g_menu.FloatOption("Boat soft lock", g_settings.Wheel.Steering.AngleBoat, minLock, g_settings.Wheel.Steering.AngleMax, 30.0,
        { "Soft lock for boats. (degrees)" });
}

void incGamma(float& gamma, float max, float step) {
    if (gamma + step > max) return;
    gamma += step;
}

void decGamma(float& gamma, float min, float step) {
    if (gamma - step < min) return;
    gamma -= step;
}

std::vector<std::string> showGammaCurve(const std::string& axis, const float input, const float gamma) {

    std::string larr = "< ";
    std::string rarr = " >";
    if (gamma >= 5.0f - 0.01f) rarr = "";
    if (gamma <= 0.1f + 0.01f) larr = "";

    std::string printVar = fmt::format("{:.{}f}", gamma, 2);

    std::vector<std::string> info{
        fmt::format("{} gamma:", axis),
        fmt::format("{}{}{}", larr, printVar, rarr)
    };

    const int max_samples = 100;
    std::vector<std::pair<float, float>> points;
    for (int i = 0; i < max_samples; i++) {
        float x = static_cast<float>(i) / static_cast<float>(max_samples);
        float y = pow(x, gamma);
        points.emplace_back(x, y);
    }

    float rectX = 0.5f;
    float rectY = 0.5f;
    float rectW = 0.40f / GRAPHICS::_GET_ASPECT_RATIO(FALSE);
    float rectH = 0.40f;
    float blockW = rectW / max_samples;//0.001f * (16.0f / 9.0f) / GRAPHICS::_GET_ASPECT_RATIO(FALSE);
    float blockH = blockW * GRAPHICS::_GET_ASPECT_RATIO(FALSE);

    GRAPHICS::DRAW_RECT(rectX, rectY,
        rectW + 3.0f*blockW, rectH + 3.0f*blockH,
        255, 255, 255, 191, 0);
    GRAPHICS::DRAW_RECT(rectX, rectY,
        rectW + blockW / 2.0f, rectH + blockH / 2.0f,
        0, 0, 0, 239, 0);

    for (auto point : points) {
        float pointX = rectX - 0.5f*rectW + point.first *rectW;
        float pointY = rectY + 0.5f*rectH - point.second *rectH;
        GRAPHICS::DRAW_RECT(pointX, pointY,
            blockW, blockH,
            255, 255, 255, 255, 0);
    }

    std::pair<float, float> currentPoint = { input, pow(input, gamma) };
    float pointX = rectX - 0.5f*rectW + currentPoint.first * rectW;
    float pointY = rectY + 0.5f*rectH - currentPoint.second * rectH;
    GRAPHICS::DRAW_RECT(pointX, pointY,
        3.0f*blockW, 3.0f*blockH,
        255, 0, 0, 255, 0);

    return info;
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

        if (g_menu.OptionPlus(
            fmt::format("Configure {}", input.Name),
            info, 
            nullptr,
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
        [=] { return incGamma(g_settings.Wheel.Brake.Gamma, 5.0f, 0.01f); },
        [=] { return decGamma(g_settings.Wheel.Brake.Gamma, 0.1f, 0.01f); },
        "Brake gamma",
        { "Linearity of the brake pedal. Values over 1.0 feel more real if you have progressive springs." });

    if (showBrakeGammaBox) {
        extras = showGammaCurve("Brake", g_controls.BrakeVal, g_settings.Wheel.Brake.Gamma);
        g_menu.OptionPlusPlus(extras, "Brake gamma");
    }

    bool showThrottleGammaBox = false;
    extras = {};
    g_menu.OptionPlus("Throttle gamma", extras, &showThrottleGammaBox,
        [=] { return incGamma(g_settings.Wheel.Throttle.Gamma, 5.0f, 0.01f); },
        [=] { return decGamma(g_settings.Wheel.Throttle.Gamma, 0.1f, 0.01f); },
        "Throttle gamma",
        { "Linearity of the throttle pedal." });

    if (showThrottleGammaBox) {
        extras = showGammaCurve("Throttle", g_controls.ThrottleVal, g_settings.Wheel.Throttle.Gamma);
        g_menu.OptionPlusPlus(extras, "Throttle gamma");
    }

    bool showSteeringGammaBox = false;
    extras = {};
    g_menu.OptionPlus("Steering gamma", extras, &showSteeringGammaBox,
        [=] { return incGamma(g_settings.Wheel.Steering.Gamma, 5.0f, 0.01f); },
        [=] { return decGamma(g_settings.Wheel.Steering.Gamma, 0.1f, 0.01f); },
        "Steering gamma",
        { "Linearity of the steering wheel." });

    if (showSteeringGammaBox) {
        float steerValL = map(g_controls.SteerVal, 0.0f, 0.5f, 1.0f, 0.0f);
        float steerValR = map(g_controls.SteerVal, 0.5f, 1.0f, 0.0f, 1.0f);
        float steerVal = g_controls.SteerVal < 0.5f ? steerValL : steerValR;
        extras = showGammaCurve("Steering", steerVal, g_settings.Wheel.Steering.Gamma);
        g_menu.OptionPlusPlus(extras, "Steering gamma");
    }
}

void update_forcefeedbackmenu() {
    g_menu.Title("Force feedback");
    g_menu.Subtitle("");

    g_menu.BoolOption("Enable", g_settings.Wheel.FFB.Enable,
        { "Enable or disable force feedback entirely." });

    g_menu.BoolOption("Scale forces", g_settings.Wheel.FFB.Scale,
        { "Scale forces to degree of rotation." });

    g_menu.FloatOption("Self aligning torque multiplier", g_settings.Wheel.FFB.SATAmpMult, 0.1f, 10.0f, 0.05f,
        { "Force feedback strength for steering. Increase for weak wheels, decrease for strong/fast wheels.",
        "Putting this too high clips force feedback. Too low and the car doesn't feel responsive." });

    g_menu.IntOption("Self aligning torque limit", g_settings.Wheel.FFB.SATMax, 0, 10000, 100,
        { "Clamp effect amplitude to this value. 10000 is the technical limit." });

    g_menu.FloatOption("Self aligning torque factor", g_settings.Wheel.FFB.SATFactor, 0.0f, 1.0f, 0.01f,
        { "Reactive force offset/multiplier, when not going straight.",
          "Depending on wheel strength, a larger value helps reaching countersteer faster or reduce overcorrection."});

    g_menu.FloatOption("Self aligning torque gamma", g_settings.Wheel.FFB.Gamma, 0.01f, 2.0f, 0.01f,
        { "< 1.0: More FFB at low speeds, FFB tapers off towards speed cap.",
          "> 1.0: Less FFB at low speeds, FFB increases towards speed cap.",
          "Keep at 1.0 for linear response. ~h~Not~h~ recommended to go higher than 1.0!"});

    g_menu.FloatOption("Self aligning torque speed cap", g_settings.Wheel.FFB.MaxSpeed, 10.0f, 1000.0f, 1.0f,
        { "Speed where FFB stops increasing. Helpful against too strong FFB at extreme speeds.",
          fmt::format("{} kph / {} mph", g_settings.Wheel.FFB.MaxSpeed * 3.6f, g_settings.Wheel.FFB.MaxSpeed * 2.23694f)});

    g_menu.FloatOption("Detail effect multiplier", g_settings.Wheel.FFB.DetailMult, 0.0f, 10.0f, 0.1f,
        { "Force feedback effects caused by the suspension. This effect is muxed with the main effect." });

    g_menu.IntOption("Detail effect limit", g_settings.Wheel.FFB.DetailLim, 0, 20000, 100, 
        { "Clamp effect amplitude to this value. 20000 allows muxing with the main effect." });

    g_menu.IntOption("Detail effect averaging", g_settings.Wheel.FFB.DetailMAW, 1, 100, 1,
        { "Averages the detail effect to prevent force feedback spikes.",
        "Recommended to keep as low as possible, as higher values delay more."});

    g_menu.FloatOption("Collision effect multiplier", g_settings.Wheel.FFB.CollisionMult, 0.0f, 10.0f, 0.1f,
        { "Force feedback effect caused by frontal/rear collisions." });

    g_menu.IntOption("Damper max (low speed)", g_settings.Wheel.FFB.DamperMax, 0, 200, 1,
        { "Wheel friction at low speed." });

    g_menu.IntOption("Damper min (high speed)", g_settings.Wheel.FFB.DamperMin, 0, 200, 1,
        { "Wheel friction at high speed." });

    g_menu.FloatOption("Damper min speed", g_settings.Wheel.FFB.DamperMinSpeed, 0.0f, 40.0f, 0.2f,
        { "Speed where the damper strength should be minimal.", "In m/s." });

    if (g_menu.Option("Tune FFB anti-deadzone")) {
        g_controls.PlayFFBCollision(0);
        g_controls.PlayFFBDynamics(0, 0);
        while (true) {
            if (IsKeyJustUp(str2key(escapeKey))) {
                break;
            }

            if (IsKeyJustUp(str2key("LEFT"))) {
                g_settings.Wheel.FFB.AntiDeadForce -= 100;
                if (g_settings.Wheel.FFB.AntiDeadForce < 100) {
                    g_settings.Wheel.FFB.AntiDeadForce = 0;
                }
            }

            if (IsKeyJustUp(str2key("RIGHT"))) {
                g_settings.Wheel.FFB.AntiDeadForce += 100;
                if (g_settings.Wheel.FFB.AntiDeadForce > 10000 - 100) {
                    g_settings.Wheel.FFB.AntiDeadForce = 10000;
                }
            }
            g_controls.UpdateValues(CarControls::InputDevices::Wheel, true);

            g_controls.PlayFFBDynamics(g_settings.Wheel.FFB.AntiDeadForce, 0);
            
            showSubtitle(fmt::format("Press LEFT and RIGHT to decrease and increase force feedback anti-deadzone. "
                "Use the highest value before your wheel starts moving. Currently [{}]. Press {} to exit.", g_settings.Wheel.FFB.AntiDeadForce, escapeKey));
            WAIT(0);
        }
    }
}

void update_buttonsmenu() {
    g_menu.Title("Buttons");
    g_menu.Subtitle("");

    std::vector<std::string> wheelToKeyInfo = {
        "Active wheel-to-key options:",
        "Press RIGHT to clear all keys bound to button",
        fmt::format("Device: {}", g_settings.GUIDToDeviceIndex(g_controls.WheelToKeyGUID))
    };

    for (int i = 0; i < MAX_RGBBUTTONS; i++) {
        if (g_controls.WheelToKey[i].empty())
            continue;
        
        for (const auto& keyval : g_controls.WheelToKey[i]) {
            wheelToKeyInfo.push_back(fmt::format("{} = {}", i, key2str(keyval)));
            if (g_controls.GetWheel().IsButtonPressed(i, g_controls.WheelToKeyGUID)) {
                wheelToKeyInfo.back() = fmt::format("{} (Pressed)", wheelToKeyInfo.back());
            }
        }
    }

    for (const auto w2kBindingPov : g_controls.WheelToKeyPov) {
        if (w2kBindingPov.second.empty())
            continue;
        auto i = w2kBindingPov.first;
        for (const auto& keyval : w2kBindingPov.second) {
            wheelToKeyInfo.push_back(fmt::format("{} = {}", i, key2str(keyval)));
            if (g_controls.GetWheel().IsButtonPressed(i, g_controls.WheelToKeyGUID)) {
                wheelToKeyInfo.back() = fmt::format("{} (Pressed)", wheelToKeyInfo.back());
            }
        }
    }

    if (g_menu.OptionPlus("Set up WheelToKey", wheelToKeyInfo, nullptr, clearWheelToKey, nullptr, "Info",
        { "Set up wheel buttons that press a keyboard key. Only one device can be used for this." })) {
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

        buttonInfo.push_back(fmt::format("Assigned to {}", input.Control));
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

    ptrdiff_t oldPos = std::find(speedoTypes.begin(), speedoTypes.end(), g_settings.HUD.Speedo.Speedo) - speedoTypes.begin();
    int newPos = static_cast<int>(oldPos);
    g_menu.StringArray("Speedometer", speedoTypes, newPos);
    if (newPos != oldPos) {
        g_settings.HUD.Speedo.Speedo = speedoTypes.at(newPos);
    }
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

    g_menu.BoolOption("RPM Gauge", g_settings.HUD.RPMBar.Enable);
    g_menu.FloatOption("RPM Redline", g_settings.HUD.RPMBar.Redline, 0.0f, 1.0f, 0.01f);

    g_menu.FloatOption("RPM X", g_settings.HUD.RPMBar.XPos, 0.0f, 1.0f, 0.0025f);
    g_menu.FloatOption("RPM Y", g_settings.HUD.RPMBar.YPos, 0.0f, 1.0f, 0.0025f);
    g_menu.FloatOption("RPM Width", g_settings.HUD.RPMBar.XSz, 0.0f, 1.0f, 0.0025f);
    g_menu.FloatOption("RPM Height", g_settings.HUD.RPMBar.YSz, 0.0f, 1.0f, 0.0025f);

    g_menu.IntOption("RPM Background Red  ", g_settings.HUD.RPMBar.BgR, 0, 255);
    g_menu.IntOption("RPM Background Green", g_settings.HUD.RPMBar.BgG, 0, 255);
    g_menu.IntOption("RPM Background Blue ", g_settings.HUD.RPMBar.BgB, 0, 255);
    g_menu.IntOption("RPM Background Alpha", g_settings.HUD.RPMBar.BgA, 0, 255);

    g_menu.IntOption("RPM Foreground Red  ", g_settings.HUD.RPMBar.FgR, 0, 255);
    g_menu.IntOption("RPM Foreground Green", g_settings.HUD.RPMBar.FgG, 0, 255);
    g_menu.IntOption("RPM Foreground Blue ", g_settings.HUD.RPMBar.FgB, 0, 255);
    g_menu.IntOption("RPM Foreground Alpha", g_settings.HUD.RPMBar.FgA, 0, 255);

    g_menu.IntOption("RPM Redline Red     ", g_settings.HUD.RPMBar.RedlineR, 0, 255);
    g_menu.IntOption("RPM Redline Green   ", g_settings.HUD.RPMBar.RedlineG, 0, 255);
    g_menu.IntOption("RPM Redline Blue    ", g_settings.HUD.RPMBar.RedlineB, 0, 255);
    g_menu.IntOption("RPM Redline Alpha   ", g_settings.HUD.RPMBar.RedlineA, 0, 255);

    g_menu.IntOption("RPM Revlimit Red    ", g_settings.HUD.RPMBar.RevLimitR, 0, 255);
    g_menu.IntOption("RPM Revlimit Green  ", g_settings.HUD.RPMBar.RevLimitG, 0, 255);
    g_menu.IntOption("RPM Revlimit Blue   ", g_settings.HUD.RPMBar.RevLimitB, 0, 255);
    g_menu.IntOption("RPM Revlimit Alpha  ", g_settings.HUD.RPMBar.RevLimitA, 0, 255);
}

void update_wheelinfomenu() {
    g_menu.Title("Wheel & pedal info");
    g_menu.Subtitle("");

    g_menu.BoolOption("Display steering wheel info", g_settings.HUD.Wheel.Enable, { "Show input info graphically." });
    g_menu.BoolOption("Always display info", g_settings.HUD.Wheel.Always, { "Display the info even without a steering wheel." });
    g_menu.FloatOption("Wheel image X", g_settings.HUD.Wheel.ImgXPos, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("Wheel image Y", g_settings.HUD.Wheel.ImgYPos, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("Wheel image size", g_settings.HUD.Wheel.ImgSize, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("Pedals X", g_settings.HUD.Wheel.PedalXPos, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("Pedals Y", g_settings.HUD.Wheel.PedalYPos, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("Pedals Height", g_settings.HUD.Wheel.PedalYSz, 0.0f, 1.0f, 0.01f);
    g_menu.FloatOption("Pedals Width", g_settings.HUD.Wheel.PedalXSz, 0.0f, 1.0f, 0.01f);
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
}

void update_dashindicatormenu() {
    g_menu.Title("Dashboard indicators");
    g_menu.Subtitle("");

    g_menu.BoolOption("Enable", g_settings.HUD.DashIndicators.Enable);
    g_menu.FloatOption("Position X", g_settings.HUD.DashIndicators.XPos, 0.0f, 1.0f, 0.005f);
    g_menu.FloatOption("Position Y", g_settings.HUD.DashIndicators.YPos, 0.0f, 1.0f, 0.005f);
    g_menu.FloatOption("Size", g_settings.HUD.DashIndicators.Size, 0.25f, 4.0f, 0.05f);
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

    g_menu.IntOption("RPM Background Red  ", g_settings.HUD.MouseSteering.BgR, 0, 255);
    g_menu.IntOption("RPM Background Green", g_settings.HUD.MouseSteering.BgG, 0, 255);
    g_menu.IntOption("RPM Background Blue ", g_settings.HUD.MouseSteering.BgB, 0, 255);
    g_menu.IntOption("RPM Background Alpha", g_settings.HUD.MouseSteering.BgA, 0, 255);

    g_menu.IntOption("RPM Foreground Red  ", g_settings.HUD.MouseSteering.FgR, 0, 255);
    g_menu.IntOption("RPM Foreground Green", g_settings.HUD.MouseSteering.FgG, 0, 255);
    g_menu.IntOption("RPM Foreground Blue ", g_settings.HUD.MouseSteering.FgB, 0, 255);
    g_menu.IntOption("RPM Foreground Alpha", g_settings.HUD.MouseSteering.FgA, 0, 255);
}

void update_driveassistmenu() {
    g_menu.Title("Driving assists");
    g_menu.Subtitle("");

    g_menu.BoolOption("Enable ABS", g_settings.DriveAssists.ABS.Enable,
        { "Custom script-driven ABS." });

    g_menu.BoolOption("Only enable ABS if not present", g_settings.DriveAssists.ABS.Filter,
        { "Only enables custom ABS on cars without the ABS flag." });

    g_menu.BoolOption("Enable TCS", g_settings.DriveAssists.TCS.Enable,
        { "Script-driven traction control." });

    g_menu.StringArray("Traction Control mode", tcsStrings, g_settings.DriveAssists.TCS.Mode,
        { "On traction loss: ",
            "Brakes: Apply brake per wheel",
            "Throttle: Cut throttle" });

    g_menu.FloatOption("TC Slip threshold", g_settings.DriveAssists.TCS.SlipMax, 0.0f, 20.0f, 0.1f,
        { "Speed in m/s an individual wheel may slip before TC kicks in." });

    g_menu.BoolOption("Enable ESC", g_settings.DriveAssists.ESP.Enable,
        { "Script-driven stability control." });

    g_menu.MenuOption("ESC settings", "espsettingsmenu", 
        { "Change the behaviour and tolerances of the stability control system." });

    g_menu.BoolOption("Enable LSD", g_settings.DriveAssists.LSD.Enable,
        { "Simulate a viscous limited slip differential. Credits to any333.",
          "LSD simulation is overridden when ABS, ESC and TCS kick in."});

    g_menu.FloatOption("LSD viscosity", g_settings.DriveAssists.LSD.Viscosity, 0.0f, 100.0f, 1.0f,
        { "How much the slower wheel tries to match the faster wheel.",
          "A very high value might speed up the car too much, because this LSD adds power to the slower wheel. "
          "About 10 is decent and doesn't affect acceleration."});
}

void update_espsettingsmenu() {
    g_menu.Title("ESC settings");
    g_menu.Subtitle("");
    g_menu.FloatOption("Oversteer starting angle", g_settings.DriveAssists.ESP.OverMin, 0.0f, 90.0f, 0.1f,
        { "Angle (degrees) where ESC starts correcting for oversteer." });
    g_menu.FloatOption("Oversteer starting correction", g_settings.DriveAssists.ESP.OverMinComp, 0.0f, 10.0f, 0.1f,
        { "Starting ESC oversteer correction value. Additional braking force for the affected wheel." });
    g_menu.FloatOption("Oversteer max angle", g_settings.DriveAssists.ESP.OverMax, 0.0f, 90.0f, 0.1f,
        { "Angle (degrees) where ESC oversteer correction is maximized." });
    g_menu.FloatOption("Oversteer max correction", g_settings.DriveAssists.ESP.OverMaxComp, 0.0f, 10.0f, 0.1f,
        { "Max ESC oversteer correction value. Additional braking force for the affected wheel." });

    g_menu.FloatOption("Understeer starting angle", g_settings.DriveAssists.ESP.UnderMin, 0.0f, 90.0f, 0.1f,
        { "Angle (degrees) where ESC starts correcting for understeer." });
    g_menu.FloatOption("Understeer starting correction", g_settings.DriveAssists.ESP.UnderMinComp, 0.0f, 10.0f, 0.1f,
        { "Starting ESC understeer correction value. Additional braking force for the affected wheel." });
    g_menu.FloatOption("Understeer max angle", g_settings.DriveAssists.ESP.UnderMax, 0.0f, 90.0f, 0.1f,
        { "Angle (degrees) where ESC understeer correction is maximized." });
    g_menu.FloatOption("Understeer max correction", g_settings.DriveAssists.ESP.UnderMaxComp, 0.0f, 10.0f, 0.1f,
        { "Max ESC oversteer understeer value. Additional braking force for the affected wheel." });
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
        { "Allow to start the engine by pressing clutch and throttle." });
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
    g_menu.FloatOption("Countersteer multiplier", g_settings.CustomSteering.CountersteerMult, 0.0f, 2.0f, 0.05f,
        { "How much countersteer should be given." });
    g_menu.FloatOption("Countersteer limit", g_settings.CustomSteering.CountersteerLimit, 0.0f, 360.0f, 1.0f, 
        { "Maximum angle in degrees for automatic countersteering. Game default is 15 degrees." });
    g_menu.FloatOption("Steering reduction", g_settings.CustomSteering.SteeringReduction, 0.0f, 1.0f, 0.01f,
        { "Reduce steering input at higher speeds.", "From InfamousSabre's Custom Steering." });
    g_menu.FloatOption("Steering multiplier", g_settings.CustomSteering.SteeringMult, 0.01f, 2.0f, 0.01f,
        { "Increase/decrease steering lock.", "From InfamousSabre's Custom Steering." });
    g_menu.FloatOption("Steering gamma", g_settings.CustomSteering.Gamma, 0.01f, 2.0f, 0.01f,
        { "Change linearity of steering input." });
    g_menu.FloatOptionCb("Steering time", g_settings.CustomSteering.SteerTime, 0.000001f, 0.90f, 0.000001f,
        getKbEntry,
        { "The lower the value, the faster the steering is.",
          "Press Enter to enter a value manually." });
    g_menu.FloatOptionCb("Centering time", g_settings.CustomSteering.CenterTime, 0.000001f, 0.99f, 0.000001f,
        getKbEntry,
        { "The lower the value, the faster the wheels return to center after letting go.",
          "Press Enter to enter a value manually." });

    g_menu.BoolOption("Custom wheel rotation", g_settings.CustomSteering.CustomRotation, 
        { "Override GTA's default 180 degree steering with whatever you want.",
          "This is purely cosmetic and does not change handling." });
    g_menu.FloatOption("Wheel rotation", g_settings.CustomSteering.CustomRotationDegrees, 180.0f, 1440.0f, 30.0f, 
        { "Rotation in degrees." });

    g_menu.BoolOption("Enable mouse steering", g_settings.CustomSteering.MouseSteering,
        { "When enabled, hold the mouse steering override button to use mouse steering." });

    g_menu.MenuOption("Mouse steering options", "mousesteeringoptionsmenu");
}

void update_mousesteeringoptionsmenu() {
    g_menu.Title("Mouse steering");
    g_menu.Subtitle("");

    g_menu.FloatOption("Mouse sensitivity", g_settings.CustomSteering.MouseSensitivity, 0.05f, 2.0f, 0.05f,
        { "Sensitivity for mouse steering." });
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
              "FPV camera angle is limited, consider enabling the custom FPV camera.",
              "Check animations.yml for more details!" });
    }

    g_menu.BoolOption("Enable custom FPV camera", g_settings.Misc.Camera.Enable,
        { "Camera mounted to the player head." });

    g_menu.MenuOption("Camera options", "cameraoptionsmenu",
        { "Adjust the custom FPV camera" });

    if (g_menu.BoolOption("Hide player in FPV", g_settings.Misc.HidePlayerInFPV,
        { "Hides the player in first person view.",
          "If you use another script that does the same, "
            "enable [Disable Player Hiding] in dev settings."})) {
        functionHidePlayerInFPV(true);
    }

    g_menu.BoolOption("Enable dashboard extensions", g_settings.Misc.DashExtensions,
        { "If DashHook is installed, the script controls some dashboard lights such as the ABS light." });

    if (g_menu.BoolOption("Enable UDP telemetry", g_settings.Misc.UDPTelemetry,
        { "Allows programs like SimHub to use data from this script."
            "This script uses DIRT 4 format for telemetry data." })) {
        StartUDPTelemetry();
    }
}

void update_cameraoptionsmenu() {
    g_menu.Title("Camera options");
    g_menu.Subtitle("");

    std::string camInfo;

    switch(g_settings.Misc.Camera.AttachId) {
        case 0:
            camInfo = "Camera moves with character while steering.";
            break;
        case 1:
            camInfo = "Camera is static with vehicle. You need to be stopped and not steering, "
                        "for the camera to get centered properly.";
            break;
        case 2:
            camInfo = "Camera is static with vehicle and uses vanilla FPV camera offsets.";
            break;
        default:
            camInfo = "Invalid selection";
            break;
    }

    if (g_menu.StringArray("Attach to", camAttachPoints, g_settings.Misc.Camera.AttachId, 
        { camInfo })) {
        FPVCam::CancelCam(); // it'll re-acquire next tick with the correct position.
    }

    g_menu.MenuOption("Motorcycle camera options", "bikecameraoptionsmenu",
        { "The FPV camera for 2-wheelers, quads and every between behaves differently,"
            "so these have their own options." });

    if (Dismemberment::Available()) {
        if (g_menu.BoolOption("Hide head", g_settings.Misc.Camera.RemoveHead,
            { "Using DismembermentASI by CamxxCore from Jedijosh' dismemberment mod, "
              "the player head can be hidden. This also turns on better near clipping." })) {
            FPVCam::HideHead(g_settings.Misc.Camera.RemoveHead);
        }
    }
    else {
        if (g_menu.Option("Hide head (download needed)", NativeMenu::solidRed,
            { "Press Select/Enter to open Dismemberment on GTA5-Mods.com.",
              "DismembermentASI.asi by CamxxCore from Jedijosh' dismemberment mod is "
              "needed to hide the player head." })) {
            WAIT(20);
            PAD::_SET_CONTROL_NORMAL(0, ControlFrontendPause, 1.0f);
            ShellExecuteA(0, 0, "https://www.gta5-mods.com/scripts/dismemberment", 0, 0, SW_SHOW);
        }
    }

    g_menu.BoolOption("Follow movement", g_settings.Misc.Camera.FollowMovement,
        { "Camera moves with motion and rotation, somewhat like NFS Shift." });

    g_menu.FloatOption("Motion multiplier", g_settings.Misc.Camera.MovementMultVel, 0.0f, 4.0f, 0.01f,
        { "How much the direction of travel affects the camera." });

    g_menu.FloatOption("Rotation movement", g_settings.Misc.Camera.MovementMultRot, 0.0f, 4.0f, 0.01f,
        { "How much the rotation speed affects the camera." });

    g_menu.FloatOption("Movement cap", g_settings.Misc.Camera.MovementCap, 0.0f, 90.0f, 1.0f,
        { "To how many degrees camera movement is capped." });

    g_menu.FloatOptionCb("Field of view", g_settings.Misc.Camera.FOV, 1.0f, 120.0f, 0.5f, getKbEntry, 
        { "In degrees." });

    g_menu.FloatOptionCb("Offset height", g_settings.Misc.Camera.OffsetHeight, -2.0f, 2.0f, 0.01f, getKbEntry,
        { "Distance in meters." });

    g_menu.FloatOptionCb("Offset forward", g_settings.Misc.Camera.OffsetForward, -2.0f, 2.0f, 0.01f, getKbEntry,
        { "Distance in meters." });

    g_menu.FloatOptionCb("Offset side", g_settings.Misc.Camera.OffsetSide, -2.0f, 2.0f, 0.01f, getKbEntry,
        { "Distance in meters." });

    g_menu.FloatOption("Pitch", g_settings.Misc.Camera.Pitch, -20.0f, 20.0f, 0.1f,
        { "In degrees." });

    g_menu.FloatOptionCb("Controller smoothing", g_settings.Misc.Camera.LookTime, 0.0f, 0.5f, 0.000001f, getKbEntry,
        { "How smooth the camera moves.", "Press enter to enter a value manually. Range: 0.0 to 0.5." });

    g_menu.FloatOption("Mouse sensitivity", g_settings.Misc.Camera.MouseSensitivity, 0.05f, 2.0f, 0.05f);

    g_menu.FloatOptionCb("Mouse smoothing", g_settings.Misc.Camera.MouseLookTime, 0.0f, 0.5f, 0.000001f, getKbEntry,
        { "How smooth the camera moves.", "Press enter to enter a value manually. Range: 0.0 to 0.5." });

    g_menu.IntOption("Mouse center timeout", g_settings.Misc.Camera.MouseCenterTimeout, 0, 2000, 25,
        { "Milliseconds before centering the camera after looking with the mouse." });
}

void update_bikecameraoptionsmenu() {
    g_menu.Title("Camera options");
    g_menu.Subtitle("2-wheelers - Quads");

    g_menu.BoolOption("Disable for 2-ish wheelers", g_settings.Misc.Camera.Bike.Disable,
        { "Use the the vanilla FPV camera on these vehicles." });

    if (g_menu.StringArray("Attach to", camAttachPoints, g_settings.Misc.Camera.Bike.AttachId)) {
        FPVCam::CancelCam();
    }

    g_menu.FloatOptionCb("Field of view", g_settings.Misc.Camera.Bike.FOV, 1.0f, 120.0f, 0.5f, getKbEntry,
        { "In degrees." });

    g_menu.FloatOptionCb("Offset height", g_settings.Misc.Camera.Bike.OffsetHeight, -2.0f, 2.0f, 0.01f, getKbEntry,
        { "Distance in meters." });

    g_menu.FloatOptionCb("Offset forward", g_settings.Misc.Camera.Bike.OffsetForward, -2.0f, 2.0f, 0.01f, getKbEntry,
        { "Distance in meters." });

    g_menu.FloatOptionCb("Offset side", g_settings.Misc.Camera.Bike.OffsetSide, -2.0f, 2.0f, 0.01f, getKbEntry,
        { "Distance in meters." });

    g_menu.FloatOption("Pitch", g_settings.Misc.Camera.Bike.Pitch, -20.0f, 20.0f, 0.1f,
        { "In degrees." });
}

void update_devoptionsmenu() {
    g_menu.Title("Developer options");
    g_menu.Subtitle("");

    g_menu.MenuOption("Debug settings", "debugmenu");

    g_menu.MenuOption("Metrics settings", "metricsmenu", 
        { "Show the G-Force graph and speed timers." });

    g_menu.MenuOption("Performance settings", "perfmenu",
        { "Every tick AI is also updated, which might impact performance." });

    g_menu.BoolOption("Disable input detection", g_settings.Debug.DisableInputDetect,
        { "Allows for manual input selection." });

    g_menu.BoolOption("Disable player hiding", g_settings.Debug.DisablePlayerHide,
        { "Disables toggling player visibility by script.",
            "Use this when some other script controls visibility." });

    g_menu.BoolOption("Enable update check", g_settings.Update.EnableUpdate,
        { "Check for mod updates."});

    if (g_menu.Option("Check for updates", { "Manually check for updates. "
        "Re-enables update checking and clears ignored update." })) {
        g_settings.Update.EnableUpdate = true;
        g_settings.Update.IgnoredVersion = "v0.0.0";

        threadCheckUpdate(0);
    }
}

void update_debugmenu() {
    g_menu.Title("Debug settings");
    g_menu.Subtitle("");

    g_menu.BoolOption("Display debug info", g_settings.Debug.DisplayInfo,
        { "Show all detailed technical info of the gearbox and inputs calculations." });
    g_menu.BoolOption("Display car wheel info", g_settings.Debug.DisplayWheelInfo,
        { "Show per-wheel debug info with off-ground detection, lockup detection and suspension info." });
    g_menu.BoolOption("Display gearing info", g_settings.Debug.DisplayGearingInfo,
        { "Show gear ratios and shift points from auto mode." });
    g_menu.BoolOption("Display force feedback lines", g_settings.Debug.DisplayFFBInfo,
        { "Show lines detailing force feedback direction and force.",
            "Green: Vehicle velocity","Red: Vehicle rotation","Purple: Steering direction" });
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

            LPDIRECTINPUT lpDi = nullptr;
            HRESULT result = DirectInput8Create(GetModuleHandle(nullptr),
                DIRECTINPUT_VERSION,
                IID_IDirectInput8,
                reinterpret_cast<void**>(&lpDi),
                nullptr);

            if (FAILED(result)) {
                logger.Write(DEBUG, "Failed to DirectInput8Create, HRESULT: %d", result);
                diDevicesInfo_.push_back(fmt::format("Failed to get DI, HRESULT: {}", result));
            }

            DIDeviceFactory::Get().Enumerate(lpDi);

            diDevicesInfo_.push_back(fmt::format("Devices: {}", DIDeviceFactory::Get().GetEntryCount()));
            diDevicesInfo_.push_back("");

            for (int i = 0; i < DIDeviceFactory::Get().GetEntryCount(); i++) {
                const auto* device = DIDeviceFactory::Get().GetEntry(i);
                std::wstring wDevName = device->diDeviceInstance.tszInstanceName;
                GUID guid = device->diDeviceInstance.guidInstance;

                // Name
                diDevicesInfo_.push_back(fmt::format("{}", StrUtil::utf8_encode(wDevName)));
                diDevicesInfo_.push_back(fmt::format("    GUID: {}", GUID2String(guid)));
                diDevicesInfo_.push_back(fmt::format("    Type: 0x{:X}", device->diDevCaps.dwDevType));
                diDevicesInfo_.push_back(fmt::format("    FFB: {}", device->diDevCaps.dwFlags & DIDC_FORCEFEEDBACK));
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
        saveChanges();
        g_settings.Read(&g_controls);
        initTimers();
    }
}

void update_perfmenu() {
    g_menu.Title("Performance settings");
    g_menu.Subtitle("");

    g_menu.BoolOption("Disable NPC gearbox", g_settings.Debug.DisableNPCGearbox,
        { "While MT is enabled, NPC uses custom script-driven gearbox logic."
            "Disabling makes NPCs drive unpredictable and cars never shift up." });

    g_menu.BoolOption("Disable NPC brakes", g_settings.Debug.DisableNPCBrake,
        { "While ABS, TCS or ESC are active, NPC braking is replaced by script.",
            "Disabling hampers AI braking." });
}

void update_menu() {
    g_menu.CheckKeys();

    /* mainmenu */
    if (g_menu.CurrentMenu("mainmenu")) { update_mainmenu(); }

    /* mainmenu -> settingsmenu */
    if (g_menu.CurrentMenu("settingsmenu")) { update_settingsmenu(); }

    /* mainmenu -> settingsmenu -> featuresmenu */
    if (g_menu.CurrentMenu("featuresmenu")) { update_featuresmenu(); }

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

    /* mainmenu -> controlsmenu -> wheelmenu -> buttonsmenu */
    if (g_menu.CurrentMenu("buttonsmenu")) { update_buttonsmenu(); }

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

    /* mainmenu -> hudmenu -> mousehudmenu*/
    if (g_menu.CurrentMenu("mousehudmenu")) { update_mousehudmenu(); }

    /* mainmenu -> driveassistmenu */
    if (g_menu.CurrentMenu("driveassistmenu")) { update_driveassistmenu(); }

    /* mainmenu -> driveassistmenu -> espsettingsmenu*/
    if (g_menu.CurrentMenu("espsettingsmenu")) { update_espsettingsmenu(); }

    /* mainmenu -> gameassistmenu */
    if (g_menu.CurrentMenu("gameassistmenu")) { update_gameassistmenu(); }

    /* mainmenu -> miscoptionsmenu */
    if (g_menu.CurrentMenu("miscoptionsmenu")) { update_miscoptionsmenu(); }

    /* mainmenu -> miscoptionsmenu -> cameraoptionsmenu */
    if (g_menu.CurrentMenu("cameraoptionsmenu")) { update_cameraoptionsmenu(); }

    /* mainmenu -> miscoptionsmenu -> cameraoptionsmenu -> bike*/
    if (g_menu.CurrentMenu("bikecameraoptionsmenu")) { update_bikecameraoptionsmenu(); }

    /* mainmenu -> devoptionsmenu */
    if (g_menu.CurrentMenu("devoptionsmenu")) { update_devoptionsmenu(); }

    /* mainmenu -> devoptionsmenu -> debugmenu */
    if (g_menu.CurrentMenu("debugmenu")) { update_debugmenu(); }

    /* mainmenu -> devoptionsmenu -> metricsmenu */
    if (g_menu.CurrentMenu("metricsmenu")) { update_metricsmenu(); }

    /* mainmenu -> devoptionsmenu -> perfmenu */
    if (g_menu.CurrentMenu("perfmenu")) { update_perfmenu(); }

    g_menu.EndMenu();
}


///////////////////////////////////////////////////////////////////////////////
//                              Config helpers/util
///////////////////////////////////////////////////////////////////////////////
// Wheel section

void saveAxis(const std::string &confTag, GUID devGUID, const std::string& axis, int min, int max) {
    saveChanges();
    std::wstring wDevName = g_controls.GetWheel().FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
    std::string devName = StrUtil::utf8_encode(wDevName);
    auto index = g_settings.SteeringAppendDevice(devGUID, devName);
    g_settings.SteeringSaveAxis(confTag, index, axis, min, max);
    g_settings.Read(&g_controls);
}

void saveButton(const std::string &confTag, GUID devGUID, int button) {
    saveChanges();
    std::wstring wDevName = g_controls.GetWheel().FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
    std::string devName = StrUtil::utf8_encode(wDevName);
    auto index = g_settings.SteeringAppendDevice(devGUID, devName);
    g_settings.SteeringSaveButton(confTag, index, button);
    g_settings.Read(&g_controls);
}

void addWheelToKey(const std::string &confTag, GUID devGUID, int button, const std::string& keyName) {
    saveChanges();
    std::wstring wDevName = g_controls.GetWheel().FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
    std::string devName = StrUtil::utf8_encode(wDevName);
    auto index = g_settings.SteeringAppendDevice(devGUID, devName);
    g_settings.SteeringAddWheelToKey(confTag, index, button, keyName);
    g_settings.Read(&g_controls);
}

void saveHShifter(const std::string &confTag, GUID devGUID, const std::vector<int>& buttonArray) {
    saveChanges();
    std::wstring wDevName = g_controls.GetWheel().FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
    std::string devName = StrUtil::utf8_encode(wDevName);
    auto index = g_settings.SteeringAppendDevice(devGUID, devName);

    for (uint8_t i = 0; i < buttonArray.size(); ++i) {
        g_settings.SteeringSaveButton(fmt::format("HPATTERN_{}", i), index, buttonArray[i]);
    }

    g_settings.Read(&g_controls);
}

void clearAxis(const std::string& confTag) {
    saveChanges();
    g_settings.SteeringSaveAxis(confTag, -1, "", 0, 0);
    g_settings.Read(&g_controls);
    UI::Notify(WARN, fmt::format("Cleared axis {}", confTag));
    initWheel();
}

void clearWheelToKey() {
    MISC::DISPLAY_ONSCREEN_KEYBOARD(1, "VEUI_ENTER_TEXT", "", "", "", "", "", 30);
    while (MISC::UPDATE_ONSCREEN_KEYBOARD() == 0) {
        PAD::DISABLE_ALL_CONTROL_ACTIONS(0);
        WAIT(0);
    }
    if (!MISC::GET_ONSCREEN_KEYBOARD_RESULT()) return;
    std::string result = MISC::GET_ONSCREEN_KEYBOARD_RESULT();

    int button;
    if (str2int(button, result.c_str(), 10) != STR2INT_SUCCESS) {
        UI::Notify(WARN, fmt::format("Invalid input: {} is not a valid number!", result));
        return;
    }
    bool found = g_settings.SteeringClearWheelToKey(button);
    if (found) {
        saveChanges();
        UI::Notify(WARN, fmt::format("Removed button {}", result));
        g_settings.Read(&g_controls);
    }
    else {
        UI::Notify(WARN, fmt::format("Button {} not found.", result));
    }
}

void clearButton(const std::string& confTag) {
    saveChanges();
    g_settings.SteeringSaveButton(confTag, -1, -1);
    g_settings.Read(&g_controls);
    UI::Notify(WARN, fmt::format("Cleared button {}", confTag));
}

void clearHShifter() {
    saveChanges();
    for (uint8_t i = 0; i < VExt::GearsAvailable(); ++i) {
        g_settings.SteeringSaveButton(fmt::format("HPATTERN_{}", i), -1, -1);
    }
    g_settings.Read(&g_controls);
    UI::Notify(WARN, "Cleared H-pattern shifter");
}

void clearASelect() {
    saveChanges();
    g_settings.SteeringSaveButton("AUTO_P", -1, -1);
    g_settings.SteeringSaveButton("AUTO_R", -1, -1);
    g_settings.SteeringSaveButton("AUTO_N", -1, -1);
    g_settings.SteeringSaveButton("AUTO_D", -1, -1);
    g_settings.Read(&g_controls);
    UI::Notify(WARN, "Cleared H-pattern shifter (auto)");
}

// Controller and keyboard
void saveKeyboardKey(const std::string& confTag, const std::string& key) {
    saveChanges();
    g_settings.KeyboardSaveKey(confTag, key);
    g_settings.Read(&g_controls);
    UI::Notify(WARN, fmt::format("Saved key {}: {}.", confTag, key));
}

void saveControllerButton(const std::string& confTag, const std::string& button) {
    saveChanges();
    g_settings.ControllerSaveButton(confTag, button);
    g_settings.Read(&g_controls);
    UI::Notify(WARN, fmt::format("Saved button {}: {}.", confTag, button));
}

void saveLControllerButton(const std::string& confTag, int button) {
    saveChanges();
    g_settings.LControllerSaveButton(confTag, button);
    g_settings.Read(&g_controls);
    UI::Notify(WARN, fmt::format("Saved button {}: {}", confTag, button));
}

void clearKeyboardKey(const std::string& confTag) {
    saveChanges();
    g_settings.KeyboardSaveKey(confTag, "UNKNOWN");
    g_settings.Read(&g_controls);
    UI::Notify(WARN, fmt::format("Cleared key {}", confTag));
}

void clearControllerButton(const std::string& confTag) {
    saveChanges();
    g_settings.ControllerSaveButton(confTag, "UNKNOWN");
    g_settings.Read(&g_controls);
    UI::Notify(WARN, fmt::format("Cleared button {}", confTag));
}

void clearLControllerButton(const std::string& confTag) {
    saveChanges();
    g_settings.LControllerSaveButton(confTag, -1);
    g_settings.Read(&g_controls);
    UI::Notify(WARN, fmt::format("Cleared button {}", confTag));
}

///////////////////////////////////////////////////////////////////////////////
//                              Config inputs
///////////////////////////////////////////////////////////////////////////////
// Wheel
bool configAxis(const std::string& confTag) {
    struct SAxisState {
        GUID Guid;
        WheelDirectInput::DIAxis Axis;
        int ValueStart;
        int ValueEnd;
    };

    std::string additionalInfo = fmt::format("Press {} to exit.", escapeKey);
    bool confSteer = confTag == "STEER";
    if (confTag == "STEER") {
        additionalInfo += " Steer right to register axis.";
    }
    else if (confTag == "HANDBRAKE_ANALOG") {
        additionalInfo += " Fully pull and set back handbrake to register axis.";
    }
    else {
        additionalInfo += fmt::format(" Fully press and release the {} pedal to register axis.", confTag);
    }

    g_controls.UpdateValues(CarControls::InputDevices::Wheel, true);
    // Save current state
    std::vector<SAxisState> axisStates;
    for (auto guid : g_controls.GetWheel().GetGuids()) {
        for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
            auto axis = static_cast<WheelDirectInput::DIAxis>(i);
            int axisValue = g_controls.GetWheel().GetAxisValue(axis, guid);
            axisStates.push_back({ guid, axis, axisValue, axisValue });
        }
    }

    // Ignore inputs before selection if they moved less than hyst
    int hyst = 65536 / 8;

    bool found = false;
    while (!found) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        g_controls.UpdateValues(CarControls::InputDevices::Wheel, true);

        for (auto guid : g_controls.GetWheel().GetGuids()) {
            for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
                auto axis = static_cast<WheelDirectInput::DIAxis>(i);
                for (auto& axisState : axisStates) {
                    if (axisState.Guid != guid)
                        continue;

                    if (axisState.Axis != axis)
                        continue;

                    int axisValue = g_controls.GetWheel().GetAxisValue(axis, guid);

                    if (axisValue == -1)
                        continue;

                    // 0 (min) - 65535 (max) -> Positive
                    if (axisValue > axisState.ValueStart + hyst && axisValue > axisState.ValueEnd) {
                        axisState.ValueEnd = axisValue;
                        if (confSteer) // Early return for steering axis.
                            found = true;
                    }

                    // 65535 (min) - 0 (max) -> Negative
                    if (axisValue < axisState.ValueStart - hyst && axisValue < axisState.ValueEnd) {
                        axisState.ValueEnd = axisValue;
                        if (confSteer) // Early return for steering axis.
                            found = true;
                    }

                    // 0 (min) - 65535 (max) -> Positive
                    if (axisValue < axisState.ValueEnd - hyst) {
                        found = true;
                    }

                    // 65535 (min) - 0 (max) -> Negative
                    if (axisValue > axisState.ValueEnd + hyst) {
                        found = true;
                    }
                }
            }
        }

        showSubtitle(additionalInfo);
        WAIT(0);
    }

    SAxisState foundAxis{};
    int maxDiff = 0;
    for (const auto& axisState : axisStates) {
        int diff = abs(axisState.ValueStart - axisState.ValueEnd);
        if (diff > maxDiff) {
            maxDiff = diff;
            foundAxis = axisState;
        }
    }
    std::string axisName = g_controls.GetWheel().DIAxisHelper[foundAxis.Axis];

    // Use full range instead of registered values, since steering is full range.
    if (confSteer) {
        int min = foundAxis.ValueEnd > foundAxis.ValueStart ? 0 : 65535;
        int max = foundAxis.ValueEnd > foundAxis.ValueStart ? 65535 : 0;
        saveAxis(confTag, foundAxis.Guid, axisName, min, max);
        return true;
    }

    int min = foundAxis.ValueStart;
    int max = foundAxis.ValueEnd;
    saveAxis(confTag, foundAxis.Guid, axisName, min, max);
    return true;
}

bool configWheelToKey() {
    std::string additionalInfo = fmt::format("Press a button to configure. Press {} to exit", escapeKey);

    g_controls.UpdateValues(CarControls::InputDevices::Wheel, true);

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
        g_controls.UpdateValues(CarControls::InputDevices::Wheel, true);

        if (progress == 0) {
            for (auto guid : g_controls.GetWheel().GetGuids()) {
                for (int i = 0; i < MAX_RGBBUTTONS; i++) {
                    if (g_controls.GetWheel().IsButtonJustReleased(i, guid)) {
                        selectedGuid = guid;
                        button = i;
                        progress++;
                    }
                }
                //POV hat
                for (auto d : g_controls.GetWheel().POVDirections) {
                    if (g_controls.GetWheel().IsButtonJustReleased(d, guid)) {
                        selectedGuid = guid;
                        button = d;
                        progress++;
                    }
                }
            }
            if (progress == 1) {
                additionalInfo = fmt::format("Press a keyboard key to configure. Press {} to exit", escapeKey);
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
        if (progress >= 2) {
            addWheelToKey("TO_KEYBOARD", selectedGuid, button, keyName);
            return true;
        }
        showSubtitle(additionalInfo);
        WAIT(0);
    }
}

bool configButton(const std::string& confTag) {
    std::string additionalInfo = fmt::format("Press {} to exit. Press a button to set {}.", escapeKey, confTag);

    g_controls.UpdateValues(CarControls::InputDevices::Wheel, true);

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        g_controls.UpdateValues(CarControls::InputDevices::Wheel, true);

        for (auto guid : g_controls.GetWheel().GetGuids()) {
            for (int i = 0; i < MAX_RGBBUTTONS; i++) {
                if (g_controls.GetWheel().IsButtonJustReleased(i, guid)) {
                    saveButton(confTag, guid, i);
                    return true;
                }
            }

            for (auto d : g_controls.GetWheel().POVDirections) {
                if (g_controls.GetWheel().IsButtonJustReleased(d, guid)) {
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
    std::string additionalInfo = fmt::format("Press {} to exit. Press {} to skip gear.", escapeKey, skipKey);

    GUID devGUID = {};
    std::vector<int> buttonArray(VExt::GearsAvailable());
    std::fill(buttonArray.begin(), buttonArray.end(), -1);

    int progress = 0;

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        if (IsKeyJustUp(str2key(skipKey))) {
            progress++;
        }

        g_controls.UpdateValues(CarControls::InputDevices::Wheel, true);

        for (auto guid : g_controls.GetWheel().GetGuids()) {
            for (int i = 0; i < MAX_RGBBUTTONS; i++) {
                // only find unregistered buttons
                if (g_controls.GetWheel().IsButtonJustPressed(i, guid) &&
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

        if (progress > VExt::GearsAvailable() - 1) {
            break;
        }
        std::string gearDisplay;
        switch (progress) {
        case 0: gearDisplay = "Reverse"; break;
        case 1: gearDisplay = "1st gear"; break;
        case 2: gearDisplay = "2nd gear"; break;
        case 3: gearDisplay = "3rd gear"; break;
        default: gearDisplay = fmt::format("{}th gear", progress); break;
        }
        showSubtitle(fmt::format("Shift into {}. {}", gearDisplay, additionalInfo));
        WAIT(0);
    }
    saveHShifter(confTag, devGUID, buttonArray);
    return true;
}

bool configASelect() {
    std::string additionalInfo = fmt::format("Press {} to exit.", escapeKey);
    GUID devGUID = {};
    std::array<int, 4> buttonArray{ -1, -1, -1, -1 };
    int progress = 0;

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }

        g_controls.UpdateValues(CarControls::InputDevices::Wheel, true);

        for (auto guid : g_controls.GetWheel().GetGuids()) {
            for (int i = 0; i < MAX_RGBBUTTONS; i++) {
                // only find unregistered buttons
                if (g_controls.GetWheel().IsButtonJustPressed(i, guid) &&
                    find(begin(buttonArray), end(buttonArray), i) == end(buttonArray)) {
                    if (progress == 0) { // also save device info when just started
                        devGUID = guid;
                        saveButton("AUTO_P", devGUID, i);
                        progress++;
                    }
                    else if (guid == devGUID) { // only accept same device onwards
                        std::string autoPos;
                        switch (progress) {
                        case 0: autoPos = "P"; break;
                        case 1: autoPos = "R"; break;
                        case 2: autoPos = "N"; break;
                        case 3: autoPos = "D"; break;
                        default: autoPos = "?"; break;
                        }
                        saveButton(fmt::format("AUTO_{}", autoPos), devGUID, i);
                        progress++;
                    }
                }
            }
        }
        if (progress > 3) {
            break;
        }
        std::string gearDisplay;
        switch (progress) {
        case 0: gearDisplay = "Park"; break;
        case 1: gearDisplay = "Reverse"; break;
        case 2: gearDisplay = "Neutral"; break;
        case 3: gearDisplay = "Drive"; break;
        default: gearDisplay = "?"; break;
        }

        if (progress == 2) {
            std::string additionalInfoN = fmt::format("Press {} to skip Neutral assignment. No input becomes Neutral. {}", skipKey, additionalInfo);
            if (IsKeyJustUp(str2key(skipKey))) {
                progress++;
                saveButton("AUTO_N", devGUID, -1);
            }
            showSubtitle(fmt::format("Shift into {}. {}", gearDisplay, additionalInfoN));
        }
        else {
            showSubtitle(fmt::format("Shift into {}. {}", gearDisplay, additionalInfo));
        }
        WAIT(0);
    }
    return true;
}

// Keyboard
bool isMenuControl(int control) {
    return control == g_menu.GetControls().ControlKeys[NativeMenu::MenuControls::ControlType::MenuKey] ||
        control == g_menu.GetControls().ControlKeys[NativeMenu::MenuControls::ControlType::MenuSelect] ||
        control == g_menu.GetControls().ControlKeys[NativeMenu::MenuControls::ControlType::MenuCancel];
}

bool configKeyboardKey(const std::string &confTag) {
    std::string additionalInfo = fmt::format("Press {} to exit", escapeKey);
    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        for (auto k : NativeMenu::KeyMap) {
            if (IsKeyJustUp(k.second)) {
                if (isMenuControl(k.second)) {
                    UI::Notify(WARN, "Can't use menu controls!");
                    continue;
                }
                saveKeyboardKey(confTag, k.first);
                return true;
            }
        }
        for (char letter = 0x30; letter <= 0x5A; letter++) {
            if (letter > 0x39 && letter < 0x41)
                continue;
            std::string letter_ = std::string(1, letter);
            if (IsKeyJustUp(str2key(letter_))) {
                if (isMenuControl(str2key(letter_))) {
                    UI::Notify(WARN, "Can't use menu controls!");
                    continue;
                }
                saveKeyboardKey(confTag, letter_);
                return true;
            }
        }

        showSubtitle(fmt::format("Press {}. Menu keys can't be chosen. {}", confTag, additionalInfo));
        WAIT(0);
    }
}

// Controller
bool configControllerButton(const std::string &confTag) {
    std::string additionalInfo = fmt::format("Press {} to exit", escapeKey);
    XInputController& controller = g_controls.GetController();

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        g_controls.UpdateValues(CarControls::InputDevices::Controller, true);
        for (const std::string& buttonHelper : controller.XboxButtonsHelper) {
            auto button = controller.StringToButton(buttonHelper);
            if (controller.IsButtonJustPressed(button)) {
                saveControllerButton(confTag, buttonHelper);
                return true;
            }
        }
        showSubtitle(fmt::format("Press {}. {}", confTag, additionalInfo));
        WAIT(0);
    }
}

bool configLControllerButton(const std::string &confTag) {
    std::string additionalInfo = fmt::format("Press {} to exit", escapeKey);

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        g_controls.UpdateValues(CarControls::InputDevices::Controller, true);
        for (const auto& input : NativeController::NativeGamepadInputs) {
            if (PAD::IS_DISABLED_CONTROL_JUST_PRESSED(0, input.first)) {
                saveLControllerButton(confTag, input.first);
                return true;
            }
        }

        showSubtitle(fmt::format("Press {}. {}", confTag, additionalInfo));
        WAIT(0);
    }
}
