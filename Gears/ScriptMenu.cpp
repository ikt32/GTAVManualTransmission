#include "script.h"

#include "ScriptSettings.hpp"
#include "Input/CarControls.hpp"
#include "Input/keyboard.h"
#include "Util/UIUtils.h"
#include "Util/Util.hpp"
#include "Constants.h"
#include "UpdateChecker.h"
#include "Memory/MemoryPatcher.hpp"
#include "Memory/VehicleExtensions.hpp"
#include "Util/MathExt.h"
#include "ScriptUtils.h"

#include <menu.h>
#include <menukeyboard.h>
#include <inc/main.h>
#include <inc/natives.h>

#include <windows.h>
#include <shellapi.h>
#include <fmt/format.h>
#include <string>
#include <mutex>

extern ReleaseInfo g_releaseInfo;
extern std::mutex g_releaseInfoMutex;

extern bool g_notifyUpdate;
extern std::mutex g_notifyUpdateMutex;

extern bool g_checkUpdateDone;
extern std::mutex g_checkUpdateDoneMutex;

extern NativeMenu::Menu menu;
extern CarControls carControls;
extern ScriptSettings settings;

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

    const std::vector<SControlText<CarControls::WheelControlType>> wheelMenuButtons {
        { CarControls::WheelControlType::HR             , "[Gear R]" },
        { CarControls::WheelControlType::H1             , "[Gear 1]" },
        { CarControls::WheelControlType::H2             , "[Gear 2]" },
        { CarControls::WheelControlType::H3             , "[Gear 3]" },
        { CarControls::WheelControlType::H4             , "[Gear 4]" },
        { CarControls::WheelControlType::H5             , "[Gear 5]" },
        { CarControls::WheelControlType::H6             , "[Gear 6]" },
        { CarControls::WheelControlType::H7             , "[Gear 7]" },
        { CarControls::WheelControlType::H8             , "[Gear 8]" },
        { CarControls::WheelControlType::H9             , "[Gear 9]" },
        { CarControls::WheelControlType::H10            , "[Gear 10]" },
        { CarControls::WheelControlType::APark          , "[Auto Park]" },
        { CarControls::WheelControlType::AReverse       , "[Auto Reverse]" },
        { CarControls::WheelControlType::ANeutral       , "[Auto Reverse]" },
        { CarControls::WheelControlType::ADrive         , "[Auto Drive]" },
        { CarControls::WheelControlType::ShiftUp        , "[ShiftUp]" },
        { CarControls::WheelControlType::ShiftDown      , "[ShiftDown]" },
        { CarControls::WheelControlType::Clutch         , "[ClutchButton]" },
        { CarControls::WheelControlType::Engine         , "[Engine]" },
        { CarControls::WheelControlType::Handbrake      , "[Handbrake]" },
        { CarControls::WheelControlType::Horn           , "[Horn]" },
        { CarControls::WheelControlType::Lights         , "[Lights]" },
        { CarControls::WheelControlType::LookBack       , "[LookBack]" },
        { CarControls::WheelControlType::LookLeft       , "[LookLeft]" },
        { CarControls::WheelControlType::LookRight      , "[LookRight]" },
        { CarControls::WheelControlType::Camera         , "[Camera]" },
        { CarControls::WheelControlType::RadioNext      , "[RadioNext]" },
        { CarControls::WheelControlType::RadioPrev      , "[RadioPrev]" },
        { CarControls::WheelControlType::IndicatorLeft  , "[IndicatorLeft]" },
        { CarControls::WheelControlType::IndicatorRight , "[IndicatorRight]" },
        { CarControls::WheelControlType::IndicatorHazard, "[IndicatorHazard]" },
        { CarControls::WheelControlType::Toggle         , "[ToggleMod]" },
        { CarControls::WheelControlType::ToggleH        , "[ChangeShiftMode]" },
    };

    const std::vector<SFont> fonts {
        { 0, "Chalet London" },
        { 1, "Sign Painter" },
        { 2, "Slab Serif" },
        { 4, "Chalet Cologne" },
        { 7, "Pricedown" },
    };

    const std::vector<std::string> buttonConfTags {
        { "TOGGLE_MOD" },
        { "CHANGE_SHIFTMODE" },
        { "THROTTLE_BUTTON" },
        { "BRAKE_BUTTON" },
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

    const std::vector<STagInfo> keyboardConfTags {
        { "Toggle"   , "Toggle mod on/off"      },
        { "ToggleH"  , "Switch shift mode"      },
        { "ShiftUp"  , "Shift up"               },
        { "ShiftDown", "Shift down"             },
        { "Clutch"   , "Hold for clutch"        },
        { "Engine"   , "Toggle engine on/off"   },
        { "Throttle" , "Key used for throttle"  },
        { "Brake"    , "Key used for brake"     },
        { "HR"       , "H-pattern gear R press" },
        { "H1"       , "H-pattern gear 1 press" },
        { "H2"       , "H-pattern gear 2 press" },
        { "H3"       , "H-pattern gear 3 press" },
        { "H4"       , "H-pattern gear 4 press" },
        { "H5"       , "H-pattern gear 5 press" },
        { "H6"       , "H-pattern gear 6 press" },
        { "H7"       , "H-pattern gear 7 press" },
        { "H8"       , "H-pattern gear 8 press" },
        { "H9"       , "H-pattern gear 9 press" },
        { "H10"      , "H-pattern gear 10 press"},
        { "HN"       , "H-pattern Neutral"      },
    };


    const std::vector<STagInfo> controllerConfTags {
        { "Toggle"     , "Toggle mod usage: hold"      },
        { "ToggleShift", "Toggle shift usage: hold"    },
        { "ShiftUp"    , "Shift up usage: press"       },
        { "ShiftDown"  , "Shift down usage: press"     },
        { "Clutch"     , "Clutch usage: axis or button"},
        { "Engine"     , "Engine usage: press"         },
        { "Throttle"   , "Throttle: axis or button"    },
        { "Brake"      , "Brake: axis or button"       }
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
        "Disabled", "Brakes", "Throttle"
    };

    const std::vector<std::string> notifyLevelStrings{
        "Debug",
        "Info",
        "UI",
        "None"
    };
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
    menu.ReadSettings();
}

void saveChanges() {
    settings.SaveGeneral();
    settings.SaveController(&carControls);
    settings.SaveWheel(&carControls);
}

void onMenuClose() {
    saveChanges();
}

void update_mainmenu() {
    menu.Title("Manual Transmission", 0.90f);
    menu.Subtitle(fmt::format("~b~{}", Constants::DisplayVersion));

    if (MemoryPatcher::Error) {
        menu.Option("Patch test error", NativeMenu::solidRed, 
            { "One or more components can't be patched. Mod behavior is uncertain."
              "Usually caused by a game update or using an incompatible version.", 
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

                if (menu.OptionPlus("New update available!", extra, nullptr, [] {
                    settings.Update.IgnoredVersion = g_releaseInfo.Version;
                    g_notifyUpdate = false;
                    saveChanges();
                    }, nullptr, "Update info",
                    { "Press accept to check GTA5-Mods.com.",
                        "Press right to ignore current update." })) {
                    WAIT(20);
                    CONTROLS::_SET_CONTROL_NORMAL(0, ControlFrontendPause, 1.0f);
                    ShellExecuteA(0, 0, modUrl.c_str(), 0, 0, SW_SHOW);
                }
            }
        }
    }

    bool tempEnableRead = settings.MTOptions.Enable;
    if (menu.BoolOption("Enable manual transmission", tempEnableRead,
        { "Enable or disable the manual transmission. Steering wheel stays active." })) {
        toggleManual(!settings.MTOptions.Enable);
    }

    int tempShiftMode = EToInt(settings.MTOptions.ShiftMode);
    std::vector<std::string> gearboxModes = {
        "Sequential",
        "H-pattern",
        "Automatic"
    };

    menu.StringArray("Gearbox", gearboxModes, tempShiftMode,
        { "Choose your gearbox! Options are Sequential, H-pattern and Automatic." });

    if (tempShiftMode != EToInt(settings.MTOptions.ShiftMode)) {
        setShiftMode(static_cast<EShiftMode>(tempShiftMode));
    }

    menu.MenuOption("Mod options", "optionsmenu", 
        { "You can tweak and fine-tune gearbox simulation here." });
    menu.MenuOption("Keyboard/Controller", "controlsmenu", 
        { "Configure the keyboard and controller inputs." });
    menu.MenuOption("Steering Wheel", "wheelmenu", 
        { "Set up your steering wheel." });
    menu.MenuOption("HUD Options", "hudmenu", 
        { "Toggle and move HUD elements. Choose between imperial or metric speeds." });
    menu.MenuOption("Driving assists", "miscassistmenu",
        { "Assist to make driving a bit easier." });
    menu.MenuOption("Debug options", "debugmenu", 
        { "Show technical details and options." });

    if (settings.Debug.DisableInputDetect) {
        int activeIndex = carControls.PrevInput;
        std::vector<std::string> inputNames {
            "Keyboard", "Controller", "Wheel"
        };
        if (menu.StringArray("Active input", inputNames, activeIndex, { "Active input is set manually." })) {
            carControls.PrevInput = static_cast<CarControls::InputDevices>(activeIndex);
        }
    }
    else {
        int activeIndex = 0;
        std::string activeInputName;
        switch (carControls.PrevInput) {
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
        menu.StringArray("Active input", { activeInputName }, activeIndex, { "Active input is automatically detected and can't be changed." });
    }

    for (const auto& device : carControls.FreeDevices) {
        if (menu.Option(device.name, NativeMenu::solidRed,
            { "~r~<b>This device needs to be configured!</b>~w~",
              "Please assign axes and buttons in ~r~<b>Steering Wheel</b>~w~->"
              "~r~<b>Analog Input Setup</b>~w~, ~r~<b>Button Input Setup</b>~w~ before using this device.",
              "Pressing Select discards this warning.",
              fmt::format("Full name: {}", device.name) })) {
            saveChanges();
            settings.SteeringAppendDevice(device.guid, device.name);
            settings.Read(&carControls);
            carControls.CheckGUIDs(settings.Wheel.InputDevices.RegisteredGUIDs);
        }
    }
}

void update_optionsmenu() {
    menu.Title("Mod options");
    menu.Subtitle("Gearbox simulation options");

    menu.BoolOption("Engine Damage", settings.MTOptions.EngDamage,
        { "Damage the engine when over-revving and when mis-shifting." });

    menu.BoolOption("Engine Stalling (H)", settings.MTOptions.EngStallH,
        { "Stall the engine when the wheel speed gets too low. Applies to H-pattern shift mode." });

    menu.BoolOption("Engine Stalling (S)", settings.MTOptions.EngStallS,
        { "Stall the engine when the wheel speed gets too low. Applies to sequential shift mode." });

    menu.BoolOption("Clutch Shift (H)", settings.MTOptions.ClutchShiftH,
        { "Require holding the clutch to shift in H-pattern shift mode." });

    menu.BoolOption("Clutch Shift (S)", settings.MTOptions.ClutchShiftS,
        { "Require holding the clutch to shift in sequential mode." });

    menu.BoolOption("Engine Braking", settings.MTOptions.EngBrake,
        { "Help the car braking by slowing down more at high RPMs." });

    menu.BoolOption("Gear/RPM Lockup", settings.MTOptions.EngLock,
        { "Simulate wheel lock-up when mis-shifting to a too low gear for the RPM range." });

    menu.BoolOption("Clutch Creep", settings.MTOptions.ClutchCreep,
        { "Simulate clutch creep when stopped with clutch engaged." });

    menu.BoolOption("Hard rev limiter", settings.MTOptions.HardLimiter,
        { "Enforce rev limiter for reverse and top speed. No more infinite speed!" });

    menu.BoolOption("Default Neutral gear", settings.GameAssists.DefaultNeutral,
        { "The car will be in neutral when you get in." });

    menu.MenuOption("Finetuning", "finetuneoptionsmenu",
        { "Fine-tune the parameters above." });

    menu.MenuOption("Shifting options", "shiftingoptionsmenu",
        { "Change the shifting behavior for sequential and automatic modes." } );

    menu.MenuOption("Automatic finetuning", "finetuneautooptionsmenu",
        { "Fine-tune script-provided automatic transmission parameters." });
}

void update_finetuneoptionsmenu() {
    menu.Title("Fine-tuning");
    menu.Subtitle("Gearbox fine-tuning options");

    menu.FloatOption("Clutch bite threshold", settings.MTParams.ClutchThreshold, 0.0f, 1.0f, 0.05f,
        { "How far the clutch has to be lifted to start biting. This value should be lower than \"Stalling threshold\"." });
    menu.FloatOption("Stalling threshold", settings.MTParams.StallingThreshold, 0.0f, 1.0f, 0.05f,
        { "How far the clutch has to be lifted to start stalling. This value should be higher than \"Clutch bite point\"." });
    menu.FloatOption("Stalling RPM", settings.MTParams.StallingRPM, 0.0f, 0.2f, 0.01f,
        { "Consider stalling when the expected RPM drops below this number.",
          "The range is 0.0 to 0.2. The engine idles at 0.2, so 0.1 is a decent value." });

    menu.FloatOption("RPM Damage", settings.MTParams.RPMDamage, 0.0f, 10.0f, 0.05f,
        { "Damage from redlining too long." });
    menu.FloatOption("Misshift Damage", settings.MTParams.MisshiftDamage, 0, 100, 5,
        { "Damage from being over the rev range." });
    menu.FloatOption("Engine braking threshold", settings.MTParams.EngBrakeThreshold, 0.0f, 1.0f, 0.05f,
        { "RPM where engine braking starts being effective." });
    menu.FloatOption("Engine braking power", settings.MTParams.EngBrakePower, 0.0f, 5.0f, 0.05f,
        { "Decrease this value if your wheels lock up when engine braking." });
}

void update_shiftingoptionsmenu() {
    menu.Title("Shifting options");
    menu.Subtitle("Auto & sequential shift options");

    menu.BoolOption("Cut throttle on upshift", settings.ShiftOptions.DownshiftBlip,
        { "Helps rev matching." });
    menu.BoolOption("Blip throttle on downshift", settings.ShiftOptions.UpshiftCut,
        { "Helps rev matching." });
    menu.FloatOption("Clutch rate multiplier", settings.ShiftOptions.ClutchRateMult, 0.05f, 20.0f, 0.05f,
        { "Change how fast clutching is. Below 1 is slower, higher than 1 is faster." });
    menu.FloatOption("Shifting RPM tolerance", settings.ShiftOptions.RPMTolerance, 0.0f, 1.0f, 0.05f,
        { "RPM mismatch tolerance on shifts",
            "Only applies to H-pattern with \"Clutch Shift\" enabled.",
            fmt::format("Clutch Shift (H) is {}abled.", settings.MTOptions.ClutchShiftH ? "~g~en" : "~r~dis")
        });
}

void update_finetuneautooptionsmenu() {
    menu.Title("Automatic transmission finetuning");
    menu.Subtitle("Script-driven automatic transmission");

    menu.FloatOption("Upshift engine load", settings.AutoParams.UpshiftLoad, 0.01f, 0.20f, 0.01f,
        { "Upshift when the engine load drops below this value. "
          "Raise this value if the car can't upshift."});
    menu.FloatOption("Downshift engine load", settings.AutoParams.DownshiftLoad, 0.30f, 1.00f, 0.01f,
        { "Downshift when the engine load rises over this value. "
          "Raise this value if the car downshifts right after upshifting." });
    menu.FloatOption("Downshift timeout multiplier", settings.AutoParams.DownshiftTimeoutMult, 0.05f, 4.00f, 0.05f,
        { "Don't downshift while car has just shifted up. "
          "Timeout based on clutch change rate.",
          "Raise for longer timeout, lower to allow earlier downshifting after an upshift." });
    menu.FloatOption("Next gear min RPM",    settings.AutoParams.NextGearMinRPM, 0.20f, 0.50f, 0.01f, 
        { "Don't upshift until next gears' RPM is over this value." });
    menu.FloatOption("Current gear min RPM", settings.AutoParams.CurrGearMinRPM, 0.20f, 0.50f, 0.01f, 
        { "Downshift when RPM drops below this value." });
    menu.FloatOption("Economy rate", settings.AutoParams.EcoRate, 0.01f, 0.50f, 0.01f,
        { "On releasing throttle, high values cause earlier upshifts.",
          "Set this low to stay in gear longer when releasing throttle." });
}

void update_controlsmenu() {
    menu.Title("Controller and keyboard options");
    menu.Subtitle("Options and assignments");

    menu.MenuOption("Controller options", "controlleroptionsmenu");

    if (settings.Controller.Native.Enable) {
        menu.MenuOption("Controller bindings (Native)", "legacycontrollermenu",
            { "Set up controller bindings." });
    }
    else {
        menu.MenuOption("Controller bindings (XInput)", "controllermenu",
            { "Set up controller bindings." });
    }

    menu.MenuOption("Keyboard bindings", "keyboardmenu",
        { "Change keyboard control bindings." });

    menu.MenuOption("Steering assists", "steeringassistmenu");
}

void update_controlleroptionsmenu() {
    menu.Title("Controller options");
    menu.Subtitle("Controller options");

    menu.BoolOption("Engine button toggles", settings.Controller.ToggleEngine,
        { "Checked: the engine button turns the engine on AND off.",
            "Not checked: the button only turns the engine on when it's off." });

    menu.IntOption("Long press time (ms)", settings.Controller.HoldTimeMs, 100, 5000, 50,
        { "Timeout for long press buttons to activate." });

    menu.IntOption("Max tap time (ms)", settings.Controller.MaxTapTimeMs, 50, 1000, 10,
        { "Buttons pressed and released within this time are regarded as a tap. Shift up, shift down are tap controls." });

    menu.FloatOption("Trigger value", settings.Controller.TriggerValue, 0.25, 1.0, 0.05,
        { "Threshold for an analog input to be detected as button press." });

    menu.BoolOption("Block car controls", settings.Controller.BlockCarControls,
        { "Blocks car action controls. Holding activates the original button again.",
            "Experimental!" });

    menu.BoolOption("Block H-pattern on controller", settings.Controller.BlockHShift,
        { "Block H-pattern mode when using controller input." });

    menu.BoolOption("Ignore shifts in UI", settings.Controller.IgnoreShiftsUI,
        { "Ignore shift up/shift down while using the phone or when the menu is open" });

    menu.BoolOption("Native controller input", settings.Controller.Native.Enable,
        { "Using a controller not supported by XInput? Enable this option and re-bind your controls." });
}

void update_legacycontrollermenu() {
    menu.Title("Controller bindings");
    menu.Subtitle("Native controls");

    std::vector<std::string> blockableControlsHelp;
    blockableControlsHelp.reserve(blockableControls.size());
    for (const auto& control : blockableControls) {
        blockableControlsHelp.emplace_back(control.Text);
    }

    int oldIndexUp = getBlockableControlIndex(carControls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)]);
    if (menu.StringArray("Shift Up blocks", blockableControlsHelp, oldIndexUp)) {
        carControls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)] = blockableControls[oldIndexUp].Control;
    }

    int oldIndexDown = getBlockableControlIndex(carControls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)]);
    if (menu.StringArray("Shift Down blocks", blockableControlsHelp, oldIndexDown)) {
        carControls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)] = blockableControls[oldIndexDown].Control;
    }

    int oldIndexClutch = getBlockableControlIndex(carControls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)]);
    if (menu.StringArray("Clutch blocks", blockableControlsHelp, oldIndexClutch)) {
        carControls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)] = blockableControls[oldIndexClutch].Control;
    }

    std::vector<std::string> controllerInfo = {
        "Press RIGHT to clear key",
        "Press RETURN to configure button",
        "",
    };

    for (const auto& confTag : controllerConfTags) {
        controllerInfo.back() = confTag.Info;
        int nativeControl = carControls.ConfTagLController2Value(confTag.Tag);
        controllerInfo.push_back(fmt::format("Assigned to {} ({})", carControls.NativeControl2Text(nativeControl), nativeControl));

        if (menu.OptionPlus(fmt::format("Assign {}", confTag.Tag), controllerInfo, nullptr, std::bind(clearLControllerButton, confTag.Tag), nullptr, "Current setting")) {
            WAIT(500);
            bool result = configLControllerButton(confTag.Tag);
            if (!result)
                UI::Notify(WARN, fmt::format("Cancelled {} assignment", confTag.Tag));
            WAIT(500);
        }
        controllerInfo.pop_back();
    }
}

void update_controllermenu() {
    menu.Title("Controller bindings");
    menu.Subtitle("XInput controls");

    std::vector<std::string> blockableControlsHelp;
    blockableControlsHelp.reserve(blockableControls.size());
    for (const auto& control : blockableControls) {
        blockableControlsHelp.emplace_back(control.Text);
    }

    int oldIndexUp = getBlockableControlIndex(carControls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftUp)]);
    if (menu.StringArray("Shift Up blocks", blockableControlsHelp, oldIndexUp)) {
        carControls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftUp)] = blockableControls[oldIndexUp].Control;
    }

    int oldIndexDown = getBlockableControlIndex(carControls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftDown)]);
    if (menu.StringArray("Shift Down blocks", blockableControlsHelp, oldIndexDown)) {
        carControls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftDown)] = blockableControls[oldIndexDown].Control;
    }

    int oldIndexClutch = getBlockableControlIndex(carControls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::Clutch)]);
    if (menu.StringArray("Clutch blocks", blockableControlsHelp, oldIndexClutch)) {
        carControls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::Clutch)] = blockableControls[oldIndexClutch].Control;
    }

    std::vector<std::string> controllerInfo = {
        "Press RIGHT to clear key",
        "Press RETURN to configure button",
        "",
    };

    for (const auto& confTag : controllerConfTags) {
        controllerInfo.back() = confTag.Info;
        controllerInfo.push_back(fmt::format("Assigned to {}", carControls.ConfTagController2Value(confTag.Tag)));
        if (menu.OptionPlus(fmt::format("Assign {}", confTag.Tag), controllerInfo, nullptr, std::bind(clearControllerButton, confTag.Tag), nullptr, "Current setting")) {
            WAIT(500);
            bool result = configControllerButton(confTag.Tag);
            if (!result)
                UI::Notify(WARN, fmt::format("Cancelled {} assignment", confTag.Tag));
            WAIT(500);
        }
        controllerInfo.pop_back();
    }
}

void update_keyboardmenu() {
    menu.Title("Keyboard bindings");
    menu.Subtitle("Keyboard bindings");

    std::vector<std::string> keyboardInfo = {
        "Press RIGHT to clear key",
        "Press RETURN to configure button",
        "",
    };

    for (const auto& confTag : keyboardConfTags) {
        keyboardInfo.back() = confTag.Info;
        keyboardInfo.push_back(fmt::format("Assigned to {}", key2str(carControls.ConfTagKB2key(confTag.Tag))));
        if (menu.OptionPlus(fmt::format("Assign {}", confTag.Tag), keyboardInfo, nullptr, std::bind(clearKeyboardKey, confTag.Tag), nullptr, "Current setting")) {
            WAIT(500);
            bool result = configKeyboardKey(confTag.Tag);
            if (!result)
                UI::Notify(WARN, fmt::format("Cancelled {} assignment", confTag.Tag));
            WAIT(500);
        }
        keyboardInfo.pop_back();
    }
}

void update_wheelmenu() {
    menu.Title("Steering wheel");
    menu.Subtitle("Steering wheel options");

    if (menu.BoolOption("Enable wheel", settings.Wheel.Options.Enable,
        { "Enable usage of a steering wheel." })) {
        saveChanges();
        settings.Read(&carControls);
        initWheel();
    }

    if (menu.FloatOption("Steering multiplier (wheel)", settings.Wheel.Steering.SteerMult, 0.1f, 2.0f, 0.01f,
        { "Increase steering lock for all cars. You might want to increase it for faster steering and more steering lock." })) {
    }

    if (menu.BoolOption("Logitech RPM LEDs", settings.Wheel.Options.LogiLEDs,
        { "Show the RPM LEDs on Logitech steering wheels. If the wheel doesn't have compatible RPM LEDs, this might crash." })) {
        settings.SaveWheel(&carControls);
    }

    menu.MenuOption("Analog input setup", "axesmenu",
        { "Configure the analog inputs." });

    menu.MenuOption("Button input setup", "buttonsmenu",
        { "Set up your buttons on your steering wheel." });

    menu.MenuOption("Force feedback options", "forcefeedbackmenu",
        { "Fine-tune your force feedback parameters." });

    menu.MenuOption("Soft lock options", "anglemenu",
        { "Set up soft lock options here." });

    std::vector<std::string> hpatInfo = {
        "Press RIGHT to clear H-pattern shifter",
        "Active gear:"
    };
    if (carControls.ButtonIn(CarControls::WheelControlType::HR)) hpatInfo.emplace_back("Reverse");
    for (uint8_t gear = 1; gear < g_numGears; ++gear) {
        // H1 == 1
        if (carControls.ButtonIn(static_cast<CarControls::WheelControlType>(gear))) 
            hpatInfo.emplace_back(fmt::format("Gear {}", gear));
    }

    if (menu.OptionPlus("H-pattern shifter setup", hpatInfo, nullptr, std::bind(clearHShifter), nullptr, "Input values",
        { "Select this option to start H-pattern shifter setup. Follow the on-screen instructions." })) {
        bool result = configHPattern();
        UI::Notify(WARN, result ? "H-pattern shifter saved" : "Cancelled H-pattern shifter setup");
    }

    std::vector<std::string> hAutoInfo = {
        "Press RIGHT to clear H-pattern auto",
        "Active gear:"
    };
    if (carControls.ButtonIn(CarControls::WheelControlType::APark)) hAutoInfo.emplace_back("Auto Park");
    if (carControls.ButtonIn(CarControls::WheelControlType::AReverse)) hAutoInfo.emplace_back("Auto Reverse");
    if (carControls.ButtonIn(CarControls::WheelControlType::ANeutral)) hAutoInfo.emplace_back("Auto Neutral");
    if (carControls.ButtonIn(CarControls::WheelControlType::ADrive)) hAutoInfo.emplace_back("Auto Drive");

    if (menu.OptionPlus("Shifter setup for automatic", hAutoInfo, nullptr, [] { clearASelect(); }, nullptr, "Input values",
        { "Set up H-pattern shifter for automatic gearbox. Follow the on-screen instructions." })) {
        bool result = configASelect();
        UI::Notify(WARN, result ? "H-pattern shifter (auto) saved" : "Cancelled H-pattern shifter (auto) setup");
    }

    menu.BoolOption("Keyboard H-pattern", settings.Wheel.Options.HPatternKeyboard,
        { "This allows you to use the keyboard for H-pattern shifting. Configure the controls in the keyboard section." });

    menu.BoolOption("Use shifter for automatic", settings.Wheel.Options.UseShifterForAuto,
        { "Use the H-pattern shifter to select the Drive/Reverse gears." });
}

void update_anglemenu() {
    menu.Title("Soft lock");
    menu.Subtitle("Soft lock & angle setup");
    float minLock = 180.0f;
    if (menu.FloatOption("Physical degrees", settings.Wheel.Steering.AngleMax, minLock, 1080.0, 30.0,
        { "How many degrees your wheel can physically turn." })) {
        if (settings.Wheel.Steering.AngleCar > settings.Wheel.Steering.AngleMax) { settings.Wheel.Steering.AngleCar = settings.Wheel.Steering.AngleMax; }
        if (settings.Wheel.Steering.AngleBike > settings.Wheel.Steering.AngleMax) { settings.Wheel.Steering.AngleBike = settings.Wheel.Steering.AngleMax; }
        if (settings.Wheel.Steering.AngleBoat > settings.Wheel.Steering.AngleMax) { settings.Wheel.Steering.AngleBoat = settings.Wheel.Steering.AngleMax; }
    }
    menu.FloatOption("Car soft lock", settings.Wheel.Steering.AngleCar, minLock, settings.Wheel.Steering.AngleMax, 30.0,
        { "Soft lock for cars and trucks. (degrees)" });

    menu.FloatOption("Bike soft lock", settings.Wheel.Steering.AngleBike, minLock, settings.Wheel.Steering.AngleMax, 30.0,
        { "Soft lock for bikes and quads. (degrees)" });

    menu.FloatOption("Boat soft lock", settings.Wheel.Steering.AngleBoat, minLock, settings.Wheel.Steering.AngleMax, 30.0,
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

    std::pair<float, float> currentPoint = { input, pow(input, gamma) };
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
    carControls.UpdateValues(CarControls::Wheel, true);
    std::vector<std::string> info = {
        "Press RIGHT to clear this axis" ,
        fmt::format("Steer:\t\t{:.3f}", carControls.SteerVal),
        fmt::format("Throttle:\t\t{:.3f}", carControls.ThrottleVal),
        fmt::format("Brake:\t\t{:.3f}", carControls.BrakeVal),
        fmt::format("Clutch:\t\t{:.3f}", carControls.ClutchVal),
        fmt::format("Handbrake:\t{:.3f}", carControls.HandbrakeVal),
    };

    if (menu.OptionPlus("Configure steering", info, nullptr, std::bind(clearAxis, "STEER"), nullptr, "Input values")) {
        bool result = configAxis("STEER");
        UI::Notify(WARN, result ? "Steering axis saved" : "Cancelled steering axis configuration");
        if (result) 
            initWheel();
    }
    if (menu.OptionPlus("Configure throttle", info, nullptr, std::bind(clearAxis, "THROTTLE"), nullptr, "Input values")) {
        bool result = configAxis("THROTTLE");
        UI::Notify(WARN, result ? "Throttle axis saved" : "Cancelled throttle axis configuration");
        if (result) 
            initWheel();
    }
    if (menu.OptionPlus("Configure brake", info, nullptr, std::bind(clearAxis, "BRAKE"), nullptr, "Input values")) {
        bool result = configAxis("BRAKE");
        UI::Notify(WARN, result ? "Brake axis saved" : "Cancelled brake axis configuration");
        if (result) 
            initWheel();
    }
    if (menu.OptionPlus("Configure clutch", info, nullptr, std::bind(clearAxis, "CLUTCH"), nullptr, "Input values")) {
        bool result = configAxis("CLUTCH");
        UI::Notify(WARN, result ? "Clutch axis saved" : "Cancelled clutch axis configuration");
        if (result) 
            initWheel();
    }
    if (menu.OptionPlus("Configure handbrake", info, nullptr, std::bind(clearAxis, "HANDBRAKE_ANALOG"), nullptr, "Input values")) {
        bool result = configAxis("HANDBRAKE_ANALOG");
        UI::Notify(WARN, result ? "Handbrake axis saved" : "Cancelled handbrake axis configuration");
        if (result) 
            initWheel();
    }

    menu.FloatOption("Steering deadzone", settings.Wheel.Steering.DeadZone, 0.0f, 0.5f, 0.01f,
        { "Deadzone size, from the center of the wheel." });

    menu.FloatOption("Steering deadzone offset", settings.Wheel.Steering.DeadZoneOffset, -0.5f, 0.5f, 0.01f,
        { "Put the deadzone with an offset from the center." });

    menu.FloatOption("Throttle anti-deadzone", settings.Wheel.Throttle.AntiDeadZone, 0.0f, 1.0f, 0.01f,
        { "GTA V ignores 25% input for analog controls by default." });

    menu.FloatOption("Brake anti-deadzone", settings.Wheel.Brake.AntiDeadZone, 0.0f, 1.0f, 0.01f,
        { "GTA V ignores 25% input for analog controls by default." });

    bool showBrakeGammaBox = false;
    std::vector<std::string> extras = {};
    menu.OptionPlus("Brake gamma", extras, &showBrakeGammaBox,
        [=] { return incGamma(settings.Wheel.Brake.Gamma, 5.0f, 0.01f); },
        [=] { return decGamma(settings.Wheel.Brake.Gamma, 0.1f, 0.01f); },
        "Brake gamma",
        { "Linearity of the brake pedal. Values over 1.0 feel more real if you have progressive springs." });

    if (showBrakeGammaBox) {
        extras = showGammaCurve("Brake", carControls.BrakeVal, settings.Wheel.Brake.Gamma);
        menu.OptionPlusPlus(extras, "Brake gamma");
    }

    bool showThrottleGammaBox = false;
    extras = {};
    menu.OptionPlus("Throttle gamma", extras, &showThrottleGammaBox,
        [=] { return incGamma(settings.Wheel.Throttle.Gamma, 5.0f, 0.01f); },
        [=] { return decGamma(settings.Wheel.Throttle.Gamma, 0.1f, 0.01f); },
        "Throttle gamma",
        { "Linearity of the throttle pedal." });

    if (showThrottleGammaBox) {
        extras = showGammaCurve("Throttle", carControls.ThrottleVal, settings.Wheel.Throttle.Gamma);
        menu.OptionPlusPlus(extras, "Throttle gamma");
    }

    bool showSteeringGammaBox = false;
    extras = {};
    menu.OptionPlus("Steering gamma", extras, &showSteeringGammaBox,
        [=] { return incGamma(settings.Wheel.Steering.Gamma, 5.0f, 0.01f); },
        [=] { return decGamma(settings.Wheel.Steering.Gamma, 0.1f, 0.01f); },
        "Steering gamma",
        { "Linearity of the steering wheel." });

    if (showSteeringGammaBox) {
        float steerValL = map(carControls.SteerVal, 0.0f, 0.5f, 1.0f, 0.0f);
        float steerValR = map(carControls.SteerVal, 0.5f, 1.0f, 0.0f, 1.0f);
        float steerVal = carControls.SteerVal < 0.5f ? steerValL : steerValR;
        extras = showGammaCurve("Steering", steerVal, settings.Wheel.Steering.Gamma);
        menu.OptionPlusPlus(extras, "Steering gamma");
    }
}

void update_forcefeedbackmenu() {
    menu.Title("Force feedback");
    menu.Subtitle("Force feedback setup");

    menu.BoolOption("Enable", settings.Wheel.FFB.Enable,
        { "Enable or disable force feedback entirely." });

    menu.BoolOption("Scale forces", settings.Wheel.FFB.Scale,
        { "Scale forces to degree of rotation." });

    menu.FloatOption("Self aligning torque multiplier", settings.Wheel.FFB.SATAmpMult, 0.1f, 10.0f, 0.05f,
        { "Force feedback strength for steering. Increase for weak wheels, decrease for strong/fast wheels.",
        "Putting this too high clips force feedback. Too low and the car doesn't feel responsive." });

    menu.IntOption("Self aligning torque limit", settings.Wheel.FFB.SATMax, 0, 10000, 100,
        { "Clamp effect amplitude to this value. 10000 is the technical limit." });

    menu.FloatOption("Self aligning torque factor", settings.Wheel.FFB.SATFactor, 0.0f, 1.0f, 0.01f,
        { "Reactive force offset/multiplier, when not going straight.",
          "Depending on wheel strength, a larger value helps reaching countersteer faster or reduce overcorrection."});

    menu.FloatOption("Detail effect multiplier", settings.Wheel.FFB.DetailMult, 0.0f, 10.0f, 0.1f,
        { "Force feedback effects caused by the suspension. This effect is muxed with the main effect." });

    menu.IntOption("Detail effect limit", settings.Wheel.FFB.DetailLim, 0, 20000, 100, 
        { "Clamp effect amplitude to this value. 20000 allows muxing with the main effect." });

    menu.IntOption("Detail effect averaging", settings.Wheel.FFB.DetailMAW, 1, 100, 1,
        { "Averages the detail effect to prevent force feedback spikes.",
        "Recommended to keep as low as possible, as higher values delay more."});

    menu.FloatOption("Collision effect multiplier", settings.Wheel.FFB.CollisionMult, 0.0f, 10.0f, 0.1f,
        { "Force feedback effect caused by frontal/rear collisions." });

    menu.IntOption("Damper max (low speed)", settings.Wheel.FFB.DamperMax, 0, 200, 1,
        { "Wheel friction at low speed." });

    menu.IntOption("Damper min (high speed)", settings.Wheel.FFB.DamperMin, 0, 200, 1,
        { "Wheel friction at high speed." });

    menu.FloatOption("Damper min speed", settings.Wheel.FFB.DamperMinSpeed, 0.0f, 40.0f, 0.2f,
        { "Speed where the damper strength should be minimal.", "In m/s." });

    if (menu.Option("Tune FFB Anti-Deadzone")) {
        carControls.PlayFFBCollision(0);
        carControls.PlayFFBDynamics(0, 0);
        while (true) {
            if (IsKeyJustUp(str2key(escapeKey))) {
                break;
            }

            if (IsKeyJustUp(str2key("LEFT"))) {
                settings.Wheel.FFB.AntiDeadForce -= 100;
                if (settings.Wheel.FFB.AntiDeadForce < 100) {
                    settings.Wheel.FFB.AntiDeadForce = 0;
                }
            }

            if (IsKeyJustUp(str2key("RIGHT"))) {
                settings.Wheel.FFB.AntiDeadForce += 100;
                if (settings.Wheel.FFB.AntiDeadForce > 10000 - 100) {
                    settings.Wheel.FFB.AntiDeadForce = 10000;
                }
            }
            carControls.UpdateValues(CarControls::InputDevices::Wheel, true);

            carControls.PlayFFBDynamics(settings.Wheel.FFB.AntiDeadForce, 0);
            
            showSubtitle(fmt::format("Press LEFT and RIGHT to decrease and increase force feedback anti-deadzone. "
                "Use the highest value before your wheel starts moving. Currently [{}]. Press {} to exit.", settings.Wheel.FFB.AntiDeadForce, escapeKey));
            WAIT(0);
        }
    }
}

void update_buttonsmenu() {
    menu.Title("Configure buttons");
    menu.Subtitle("Button setup and review");

    std::vector<std::string> wheelToKeyInfo = {
        "Active wheel-to-key options:",
        "Press RIGHT to clear a button",
        fmt::format("Device: {}", settings.GUIDToDeviceIndex(carControls.WheelToKeyGUID))
    };

    for (int i = 0; i < MAX_RGBBUTTONS; i++) {
        if (carControls.WheelToKey[i] != -1) {
            wheelToKeyInfo.push_back(fmt::format("{} = {}", i, key2str(carControls.WheelToKey[i])));
            if (carControls.GetWheel().IsButtonPressed(i, carControls.WheelToKeyGUID)) {
                wheelToKeyInfo.back() = fmt::format("{} (Pressed)", wheelToKeyInfo.back());
            }
        }
    }

    if (menu.OptionPlus("Set up WheelToKey", wheelToKeyInfo, nullptr, clearWheelToKey, nullptr, "Info",
        { "Set up wheel buttons that press a keyboard key. Only one device can be used for this." })) {
        bool result = configWheelToKey();
        UI::Notify(WARN, result ? "Entry added" : "Cancelled entry addition");
    }

    std::vector<std::string> buttonInfo;
    buttonInfo.emplace_back("Press RIGHT to clear this button");
    buttonInfo.emplace_back("Active buttons:");
    for (const auto& wheelButton : wheelMenuButtons) {
        if (carControls.ButtonIn(wheelButton.Control))
            buttonInfo.emplace_back(wheelButton.Text);
    }
    if (buttonInfo.size() == 2)
        buttonInfo.emplace_back("None");

    for (auto confTag : buttonConfTags) {
        buttonInfo.push_back(fmt::format("Assigned to {}", carControls.ConfTagWheel2Value(confTag)));
        if (menu.OptionPlus(fmt::format("Assign {}", confTag), buttonInfo, nullptr, std::bind(clearButton, confTag), nullptr, "Current inputs")) {
            bool result = configButton(confTag);
            UI::Notify(WARN, fmt::format("[{}] {}", confTag, result ? "saved" : "assignment cancelled."));
        }
        buttonInfo.pop_back();
    }
}

void update_hudmenu() {
    menu.Title("HUD Options");
    menu.Subtitle("");

    menu.BoolOption("Enable", settings.HUD.Enable,
        { "Display HUD elements." });

    menu.BoolOption("Always enable", settings.HUD.Always,
        { "Display HUD even if manual transmission is off." });

    auto fontIt = std::find_if(fonts.begin(), fonts.end(), [](const SFont& font) { return font.ID == settings.HUD.Font; });
    if (fontIt != fonts.end()) {
        std::vector<std::string> strFonts;
        strFonts.reserve(fonts.size());
        for (const auto& font : fonts)
            strFonts.push_back(font.Name);
        int fontIndex = static_cast<int>(fontIt - fonts.begin());
        if (menu.StringArray("Font", strFonts, fontIndex, { "Select the font for speed, gearbox mode, current gear." })) {
            settings.HUD.Font = fonts.at(fontIndex).ID;
        }
    }
    else {
        menu.Option("Invalid font ID in settings");
    }

    menu.StringArray("Notification level", notifyLevelStrings, settings.HUD.NotifyLevel,
        { "What kind of notifications to display.",
        "Debug: All",
        "Info: Mode switching",
        "UI: Menu actions and setup",
        "None: Hide all notifications" });

    menu.MenuOption("Gear and shift mode", "geardisplaymenu");
    menu.MenuOption("Speedometer", "speedodisplaymenu");
    menu.MenuOption("RPM Gauge", "rpmdisplaymenu");
    menu.MenuOption("Wheel & Pedal Info", "wheelinfomenu");
}

void update_geardisplaymenu() {
    menu.Title("Gear options");
    menu.Subtitle("");

    menu.BoolOption("Gear", settings.HUD.Gear.Enable);
    menu.BoolOption("Shift Mode", settings.HUD.ShiftMode.Enable);

    menu.FloatOption("Gear X", settings.HUD.Gear.XPos, 0.0f, 1.0f, 0.005f);
    menu.FloatOption("Gear Y", settings.HUD.Gear.YPos, 0.0f, 1.0f, 0.005f);
    menu.FloatOption("Gear Size", settings.HUD.Gear.Size, 0.0f, 3.0f, 0.05f);
    menu.IntOption("Gear Top Color Red", settings.HUD.Gear.TopColorR, 0, 255);
    menu.IntOption("Gear Top Color Green", settings.HUD.Gear.TopColorG, 0, 255);
    menu.IntOption("Gear Top Color Blue", settings.HUD.Gear.TopColorB, 0, 255);

    menu.FloatOption("Shift Mode X", settings.HUD.ShiftMode.XPos, 0.0f, 1.0f, 0.005f);
    menu.FloatOption("Shift Mode Y", settings.HUD.ShiftMode.YPos, 0.0f, 1.0f, 0.005f);
    menu.FloatOption("Shift Mode Size", settings.HUD.ShiftMode.Size, 0.0f, 3.0f, 0.05f);
}

void update_speedodisplaymenu() {
    menu.Title("Speedometer options");
    menu.Subtitle("");

    ptrdiff_t oldPos = std::find(speedoTypes.begin(), speedoTypes.end(), settings.HUD.Speedo.Speedo) - speedoTypes.begin();
    int newPos = static_cast<int>(oldPos);
    menu.StringArray("Speedometer", speedoTypes, newPos);
    if (newPos != oldPos) {
        settings.HUD.Speedo.Speedo = speedoTypes.at(newPos);
    }
    menu.BoolOption("Show units", settings.HUD.Speedo.ShowUnit);

    menu.FloatOption("Speedometer X", settings.HUD.Speedo.XPos, 0.0f, 1.0f, 0.005f);
    menu.FloatOption("Speedometer Y", settings.HUD.Speedo.YPos, 0.0f, 1.0f, 0.005f);
    menu.FloatOption("Speedometer Size", settings.HUD.Speedo.Size, 0.0f, 3.0f, 0.05f);
}

void update_rpmdisplaymenu() {
    menu.Title("RPM Gauge options");
    menu.Subtitle("");

    menu.BoolOption("RPM Gauge", settings.HUD.RPMBar.Enable);
    menu.FloatOption("RPM Redline", settings.HUD.RPMBar.Redline, 0.0f, 1.0f, 0.01f);

    menu.FloatOption("RPM X", settings.HUD.RPMBar.XPos, 0.0f, 1.0f, 0.0025f);
    menu.FloatOption("RPM Y", settings.HUD.RPMBar.YPos, 0.0f, 1.0f, 0.0025f);
    menu.FloatOption("RPM Width", settings.HUD.RPMBar.XSz, 0.0f, 1.0f, 0.0025f);
    menu.FloatOption("RPM Height", settings.HUD.RPMBar.YSz, 0.0f, 1.0f, 0.0025f);

    menu.IntOption("RPM Background Red  ", settings.HUD.RPMBar.BgR, 0, 255);
    menu.IntOption("RPM Background Green", settings.HUD.RPMBar.BgG, 0, 255);
    menu.IntOption("RPM Background Blue ", settings.HUD.RPMBar.BgB, 0, 255);
    menu.IntOption("RPM Background Alpha", settings.HUD.RPMBar.BgA, 0, 255);

    menu.IntOption("RPM Foreground Red  ", settings.HUD.RPMBar.FgR, 0, 255);
    menu.IntOption("RPM Foreground Green", settings.HUD.RPMBar.FgG, 0, 255);
    menu.IntOption("RPM Foreground Blue ", settings.HUD.RPMBar.FgB, 0, 255);
    menu.IntOption("RPM Foreground Alpha", settings.HUD.RPMBar.FgA, 0, 255);

    menu.IntOption("RPM Redline Red     ", settings.HUD.RPMBar.RedlineR, 0, 255);
    menu.IntOption("RPM Redline Green   ", settings.HUD.RPMBar.RedlineG, 0, 255);
    menu.IntOption("RPM Redline Blue    ", settings.HUD.RPMBar.RedlineB, 0, 255);
    menu.IntOption("RPM Redline Alpha   ", settings.HUD.RPMBar.RedlineA, 0, 255);

    menu.IntOption("RPM Revlimit Red    ", settings.HUD.RPMBar.RevLimitR, 0, 255);
    menu.IntOption("RPM Revlimit Green  ", settings.HUD.RPMBar.RevLimitG, 0, 255);
    menu.IntOption("RPM Revlimit Blue   ", settings.HUD.RPMBar.RevLimitB, 0, 255);
    menu.IntOption("RPM Revlimit Alpha  ", settings.HUD.RPMBar.RevLimitA, 0, 255);
}

void update_wheelinfomenu() {
    menu.Title("Wheel & Pedal Info");
    menu.Subtitle("");

    menu.BoolOption("Display steering wheel info", settings.HUD.Wheel.Enable, { "Show input info graphically." });
    menu.BoolOption("Always display info", settings.HUD.Wheel.Always, { "Display the info even without a steering wheel." });
    menu.FloatOption("Wheel image X", settings.HUD.Wheel.ImgXPos, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Wheel image Y", settings.HUD.Wheel.ImgYPos, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Wheel image size", settings.HUD.Wheel.ImgSize, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Pedals X", settings.HUD.Wheel.PedalXPos, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Pedals Y", settings.HUD.Wheel.PedalYPos, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Pedals Height", settings.HUD.Wheel.PedalYSz, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Pedals Width", settings.HUD.Wheel.PedalXSz, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Pedals Pad X", settings.HUD.Wheel.PedalXPad, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Pedals Pad Y", settings.HUD.Wheel.PedalYPad, 0.0f, 1.0f, 0.01f);
    menu.IntOption("Pedals Background Alpha", settings.HUD.Wheel.PedalBgA, 0, 255);
    menu.IntOption("Throttle Bar Red    ", settings.HUD.Wheel.PedalThrottleR, 0, 255);
    menu.IntOption("Throttle Bar Green  ", settings.HUD.Wheel.PedalThrottleG, 0, 255);
    menu.IntOption("Throttle Bar Blue   ", settings.HUD.Wheel.PedalThrottleB, 0, 255);
    menu.IntOption("Throttle Bar Alpha  ", settings.HUD.Wheel.PedalThrottleA, 0, 255);
    menu.IntOption("Brake Bar Red    ", settings.HUD.Wheel.PedalBrakeR, 0, 255);
    menu.IntOption("Brake Bar Green  ", settings.HUD.Wheel.PedalBrakeG, 0, 255);
    menu.IntOption("Brake Bar Blue   ", settings.HUD.Wheel.PedalBrakeB, 0, 255);
    menu.IntOption("Brake Bar Alpha  ", settings.HUD.Wheel.PedalBrakeA   , 0, 255);
    menu.IntOption("Clutch Bar Red    ", settings.HUD.Wheel.PedalClutchR, 0, 255);
    menu.IntOption("Clutch Bar Green  ", settings.HUD.Wheel.PedalClutchG, 0, 255);
    menu.IntOption("Clutch Bar Blue   ", settings.HUD.Wheel.PedalClutchB, 0, 255);
    menu.IntOption("Clutch Bar Alpha  ", settings.HUD.Wheel.PedalClutchA  , 0, 255);
}

void update_miscassistmenu() {
    menu.Title("Driving assists");
    menu.Subtitle("Assists to make driving easier");

    menu.BoolOption("Enable ABS", settings.DriveAssists.CustomABS,
        { "Experimental script-driven ABS." });
    menu.BoolOption("Only enable ABS if not present", settings.DriveAssists.ABSFilter,
        { "Only enables script-driven ABS on vehicles without the ABS flag." });

    menu.StringArray("Traction Control mode", tcsStrings, settings.DriveAssists.TCMode,
        { "Disabled", "Brakes", "Throttle" });

    menu.BoolOption("Simple Bike", settings.GameAssists.SimpleBike,
        { "Disables bike engine stalling and the clutch bite simulation." });

    menu.BoolOption("Hill gravity workaround", settings.GameAssists.HillGravity,
        { "Gives the car a push to overcome the games' default brakes at a stop." });

    menu.BoolOption("Auto gear 1", settings.GameAssists.AutoGear1,
        { "Automatically switch to first gear when the car reaches a standstill." });

    menu.BoolOption("Auto look back", settings.GameAssists.AutoLookBack,
        { "Automatically look back whenever in reverse gear." });

    menu.BoolOption("Clutch + throttle start", settings.GameAssists.ThrottleStart,
        { "Allow to start the engine by pressing clutch and throttle." });

    if (menu.BoolOption("Hide player in FPV", settings.GameAssists.HidePlayerInFPV,
        { "Hides the player in first person view." })) {
        functionHidePlayerInFPV(true);
    }
}

void update_steeringassistmenu() {
    menu.Title("Steering assists");
    menu.Subtitle("Custom steering assist options");

    std::vector<std::string> steeringModeDescription;
    switch(settings.CustomSteering.Mode) {
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
    menu.StringArray("Steering Mode", { "Default", "Enhanced" }, settings.CustomSteering.Mode, 
        steeringModeDescription);
    menu.FloatOption("Countersteer multiplier", settings.CustomSteering.CountersteerMult, 0.0f, 2.0f, 0.05f,
        { "How much countersteer should be given." });
    menu.FloatOption("Countersteer limit", settings.CustomSteering.CountersteerLimit, 0.0f, 360.0f, 1.0f, 
        { "Maximum angle in degrees for automatic countersteering. Game default is 15 degrees." });
    menu.FloatOption("Steering Reduction", settings.CustomSteering.SteeringReduction, 0.0f, 1.0f, 0.01f,
        { "Reduce steering input at higher speeds.", "From InfamousSabre's Custom Steering." });
    menu.FloatOption("Steering Multiplier", settings.CustomSteering.SteeringMult, 0.01f, 2.0f, 0.01f,
        { "Increase/decrease steering lock.", "From InfamousSabre's Custom Steering." });
    menu.FloatOption("Steering Gamma", settings.CustomSteering.Gamma, 0.01f, 2.0f, 0.01f,
        { "Change linearity of steering input." });
}

void update_debugmenu() {
    menu.Title("Debug settings");
    menu.Subtitle("Extra mod info");

    menu.BoolOption("Display info", settings.Debug.DisplayInfo,
        { "Show all detailed technical info of the gearbox and inputs calculations." });
    menu.BoolOption("Display car wheel info", settings.Debug.DisplayWheelInfo,
        { "Show per-wheel debug info with off-ground detection, lockup detection and suspension info." });
    menu.BoolOption("Display gearing info", settings.Debug.DisplayGearingInfo,
        { "Show gear ratios and shift points from auto mode." });
    menu.BoolOption("Display force feedback lines", settings.Debug.DisplayFFBInfo,
        { "Show lines detailing force feedback direction and force.",
            "Green: Vehicle velocity","Red: Vehicle rotation","Purple: Steering direction" });
    menu.BoolOption("Show NPC info", settings.Debug.DisplayNPCInfo,
        { "Show vehicle info of NPC vehicles near you." });
    menu.BoolOption("Disable input detection", settings.Debug.DisableInputDetect,
        { "Allows for manual input selection." });
    menu.BoolOption("Disable player hiding", settings.Debug.DisablePlayerHide, 
        { "Disables toggling player visibility by script.",
            "Use this when some other script controls visibility." });

    menu.BoolOption("Enable update check", settings.Update.EnableUpdate,
        { "Check for mod updates."});

    if (menu.Option("Check for updates", { "Manually check for updates. "
        "Re-enables update checking and clears ignored update." })) {
        settings.Update.EnableUpdate = true;
        settings.Update.IgnoredVersion = "v0.0.0";

        threadCheckUpdate();
    }
}

void update_menu() {
    menu.CheckKeys();

    /* mainmenu */
    if (menu.CurrentMenu("mainmenu")) { update_mainmenu(); }

    /* mainmenu -> optionsmenu */
    if (menu.CurrentMenu("optionsmenu")) { update_optionsmenu(); }

    /* mainmenu -> optionsmenu -> finetuneoptionsmenu */
    if (menu.CurrentMenu("finetuneoptionsmenu")) { update_finetuneoptionsmenu(); }

    /* mainmenu -> optionsmenu -> shiftingoptionsmenu */
    if (menu.CurrentMenu("shiftingoptionsmenu")) { update_shiftingoptionsmenu(); }

    /* mainmenu -> optionsmenu -> finetuneautooptionsmenu */
    if (menu.CurrentMenu("finetuneautooptionsmenu")) { update_finetuneautooptionsmenu(); }

    /* mainmenu -> controlsmenu */
    if (menu.CurrentMenu("controlsmenu")) { update_controlsmenu(); }

    /* mainmenu -> controlsmenu -> controlleroptionsmenu */
    if (menu.CurrentMenu("controlleroptionsmenu")) { update_controlleroptionsmenu(); }

    /* mainmenu -> controlsmenu -> legacycontrollermenu */
    if (menu.CurrentMenu("legacycontrollermenu")) { update_legacycontrollermenu(); }

    /* mainmenu -> controlsmenu -> controllerbindingsmenu */
    if (menu.CurrentMenu("controllermenu")) { update_controllermenu(); }

    /* mainmenu -> controlsmenu -> keyboardmenu */
    if (menu.CurrentMenu("keyboardmenu")) { update_keyboardmenu(); }

    /* mainmenu -> wheelmenu */
    if (menu.CurrentMenu("wheelmenu")) { update_wheelmenu(); }

    /* mainmenu -> wheelmenu -> anglemenu */
    if (menu.CurrentMenu("anglemenu")) { update_anglemenu(); }

    /* mainmenu -> wheelmenu -> axesmenu */
    if (menu.CurrentMenu("axesmenu")) { update_axesmenu(); }

    /* mainmenu -> wheelmenu -> forcefeedbackmenu */
    if (menu.CurrentMenu("forcefeedbackmenu")) { update_forcefeedbackmenu(); }

    /* mainmenu -> wheelmenu -> buttonsmenu */
    if (menu.CurrentMenu("buttonsmenu")) { update_buttonsmenu(); }

    /* mainmenu -> hudmenu */
    if (menu.CurrentMenu("hudmenu")) { update_hudmenu(); }

    /* mainmenu -> hudmenu -> geardisplaymenu*/
    if (menu.CurrentMenu("geardisplaymenu")) { update_geardisplaymenu(); }

    /* mainmenu -> hudmenu -> speedodisplaymenu*/
    if (menu.CurrentMenu("speedodisplaymenu")) { update_speedodisplaymenu(); }

    /* mainmenu -> hudmenu -> rpmdisplaymenu*/
    if (menu.CurrentMenu("rpmdisplaymenu")) { update_rpmdisplaymenu(); }

    /* mainmenu -> hudmenu -> wheelinfomenu*/
    if (menu.CurrentMenu("wheelinfomenu")) { update_wheelinfomenu(); }

    /* mainmenu -> miscassistmenu */
    if (menu.CurrentMenu("miscassistmenu")) { update_miscassistmenu(); }

    /* mainmenu -> miscassistmenu -> steeringassistmenu */
    if (menu.CurrentMenu("steeringassistmenu")) { update_steeringassistmenu(); }

    /* mainmenu -> debugmenu */
    if (menu.CurrentMenu("debugmenu")) { update_debugmenu(); }

    menu.EndMenu();
}


///////////////////////////////////////////////////////////////////////////////
//                              Config helpers/util
///////////////////////////////////////////////////////////////////////////////
// Wheel section

void saveAxis(const std::string &confTag, GUID devGUID, const std::string& axis, int min, int max) {
    saveChanges();
    std::wstring wDevName = carControls.GetWheel().FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
    std::string devName = std::string(wDevName.begin(), wDevName.end());
    auto index = settings.SteeringAppendDevice(devGUID, devName);
    settings.SteeringSaveAxis(confTag, index, axis, min, max);
    if (confTag == "STEER") {
        settings.SteeringSaveFFBAxis(confTag, index, axis);
    }
    settings.Read(&carControls);
}

void saveButton(const std::string &confTag, GUID devGUID, int button) {
    saveChanges();
    std::wstring wDevName = carControls.GetWheel().FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
    std::string devName = std::string(wDevName.begin(), wDevName.end());
    auto index = settings.SteeringAppendDevice(devGUID, devName);
    settings.SteeringSaveButton(confTag, index, button);
    settings.Read(&carControls);
}

void addWheelToKey(const std::string &confTag, GUID devGUID, int button, const std::string& keyName) {
    saveChanges();
    std::wstring wDevName = carControls.GetWheel().FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
    std::string devName = std::string(wDevName.begin(), wDevName.end());
    auto index = settings.SteeringAppendDevice(devGUID, devName);
    settings.SteeringAddWheelToKey(confTag, index, button, keyName);
    settings.Read(&carControls);
}

void saveHShifter(const std::string &confTag, GUID devGUID, const std::vector<int>& buttonArray) {
    saveChanges();
    std::wstring wDevName = carControls.GetWheel().FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
    std::string devName = std::string(wDevName.begin(), wDevName.end());
    auto index = settings.SteeringAppendDevice(devGUID, devName);
    settings.SteeringSaveHShifter(confTag, index, buttonArray);
    settings.Read(&carControls);
}

void clearAxis(const std::string& confTag) {
    saveChanges();
    settings.SteeringSaveAxis(confTag, -1, "", 0, 0);
    settings.Read(&carControls);
    UI::Notify(WARN, fmt::format("Cleared axis {}", confTag));
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
        UI::Notify(WARN, fmt::format("Invalid input: {} is not a valid number!", result));
        return;
    }
    bool found = settings.SteeringClearWheelToKey(button);
    if (found) {
        saveChanges();
        UI::Notify(WARN, fmt::format("Removed button {}", result));
        settings.Read(&carControls);
    }
    else {
        UI::Notify(WARN, fmt::format("Button {} not found.", result));
    }
}

void clearButton(const std::string& confTag) {
    saveChanges();
    settings.SteeringSaveButton(confTag, -1, -1);
    settings.Read(&carControls);
    UI::Notify(WARN, fmt::format("Cleared button {}", confTag));
}

void clearHShifter() {
    saveChanges();
    std::vector<int> empty(g_numGears);
    std::fill(empty.begin(), empty.end(), -1);

    settings.SteeringSaveHShifter("SHIFTER", -1, empty);
    settings.Read(&carControls);
    UI::Notify(WARN, "Cleared H-pattern shifter");
}

void clearASelect() {
    saveChanges();
    settings.SteeringSaveButton("AUTO_R", -1, -1);
    settings.SteeringSaveButton("AUTO_D", -1, -1);
    settings.Read(&carControls);
    UI::Notify(WARN, "Cleared H-pattern shifter (auto)");
}

// Controller and keyboard
void saveKeyboardKey(const std::string& confTag, const std::string& key) {
    saveChanges();
    settings.KeyboardSaveKey(confTag, key);
    settings.Read(&carControls);
    UI::Notify(WARN, fmt::format("Saved key {}: {}.", confTag, key));
}

void saveControllerButton(const std::string& confTag, const std::string& button) {
    saveChanges();
    settings.ControllerSaveButton(confTag, button);
    settings.Read(&carControls);
    UI::Notify(WARN, fmt::format("Saved button {}: {}.", confTag, button));
}

void saveLControllerButton(const std::string& confTag, int button) {
    saveChanges();
    settings.LControllerSaveButton(confTag, button);
    settings.Read(&carControls);
    UI::Notify(WARN, fmt::format("Saved button {}: {}", confTag, button));
}

void clearKeyboardKey(const std::string& confTag) {
    saveChanges();
    settings.KeyboardSaveKey(confTag, "UNKNOWN");
    settings.Read(&carControls);
    UI::Notify(WARN, fmt::format("Cleared key {}", confTag));
}

void clearControllerButton(const std::string& confTag) {
    saveChanges();
    settings.ControllerSaveButton(confTag, "UNKNOWN");
    settings.Read(&carControls);
    UI::Notify(WARN, fmt::format("Cleared button {}", confTag));
}

void clearLControllerButton(const std::string& confTag) {
    saveChanges();
    settings.LControllerSaveButton(confTag, -1);
    settings.Read(&carControls);
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

    carControls.UpdateValues(CarControls::InputDevices::Wheel, true);
    // Save current state
    std::vector<SAxisState> axisStates;
    for (auto guid : carControls.GetWheel().GetGuids()) {
        for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
            auto axis = static_cast<WheelDirectInput::DIAxis>(i);
            int axisValue = carControls.GetWheel().GetAxisValue(axis, guid);
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
        carControls.UpdateValues(CarControls::InputDevices::Wheel, true);

        for (auto guid : carControls.GetWheel().GetGuids()) {
            for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
                auto axis = static_cast<WheelDirectInput::DIAxis>(i);
                for (auto& axisState : axisStates) {
                    if (axisState.Guid != guid)
                        continue;

                    if (axisState.Axis != axis)
                        continue;

                    int axisValue = carControls.GetWheel().GetAxisValue(axis, guid);

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
    std::string axisName = carControls.GetWheel().DIAxisHelper[foundAxis.Axis];

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

    carControls.UpdateValues(CarControls::InputDevices::Wheel, true);

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
        carControls.UpdateValues(CarControls::InputDevices::Wheel, true);

        if (progress == 0) {
            for (auto guid : carControls.GetWheel().GetGuids()) {
                for (int i = 0; i < MAX_RGBBUTTONS; i++) {
                    if (carControls.GetWheel().IsButtonJustReleased(i, guid)) {
                        selectedGuid = guid;
                        button = i;
                        progress++;
                    }
                }
                //POV hat
                for (auto d : carControls.GetWheel().POVDirections) {
                    if (carControls.GetWheel().IsButtonJustReleased(d, guid)) {
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

    carControls.UpdateValues(CarControls::InputDevices::Wheel, true);

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        carControls.UpdateValues(CarControls::InputDevices::Wheel, true);

        for (auto guid : carControls.GetWheel().GetGuids()) {
            for (int i = 0; i < MAX_RGBBUTTONS; i++) {
                if (carControls.GetWheel().IsButtonJustReleased(i, guid)) {
                    saveButton(confTag, guid, i);
                    return true;
                }
            }

            for (auto d : carControls.GetWheel().POVDirections) {
                if (carControls.GetWheel().IsButtonJustReleased(d, guid)) {
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
    std::vector<int> buttonArray(g_numGears);
    std::fill(buttonArray.begin(), buttonArray.end(), -1);

    int progress = 0;

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        if (IsKeyJustUp(str2key(skipKey))) {
            progress++;
        }

        carControls.UpdateValues(CarControls::InputDevices::Wheel, true);

        for (auto guid : carControls.GetWheel().GetGuids()) {
            for (int i = 0; i < MAX_RGBBUTTONS; i++) {
                // only find unregistered buttons
                if (carControls.GetWheel().IsButtonJustPressed(i, guid) &&
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

        if (progress > g_numGears - 1) {
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

        carControls.UpdateValues(CarControls::InputDevices::Wheel, true);

        for (auto guid : carControls.GetWheel().GetGuids()) {
            for (int i = 0; i < MAX_RGBBUTTONS; i++) {
                // only find unregistered buttons
                if (carControls.GetWheel().IsButtonJustPressed(i, guid) &&
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
    return control == menu.GetControls().ControlKeys[NativeMenu::MenuControls::ControlType::MenuKey] ||
        control == menu.GetControls().ControlKeys[NativeMenu::MenuControls::ControlType::MenuSelect] ||
        control == menu.GetControls().ControlKeys[NativeMenu::MenuControls::ControlType::MenuCancel];
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
    XInputController controller = carControls.GetController();

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        carControls.UpdateValues(CarControls::InputDevices::Controller, true);
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
        carControls.UpdateValues(CarControls::InputDevices::Controller, true);

        for (auto mapItem : carControls.LegacyControlsMap) {
            if (CONTROLS::IS_DISABLED_CONTROL_JUST_PRESSED(0, mapItem.second)) {
                saveLControllerButton(confTag, mapItem.second);
                return true;
            }
        }

        showSubtitle(fmt::format("Press {}. {}", confTag, additionalInfo));
        WAIT(0);
    }
}
