#include "script.h"

#include <string>

#include <shellapi.h>

#include <fmt/format.h>

#include <inc/main.h>
#include <inc/natives.h>

#include <menu.h>
#include <menukeyboard.h>

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

extern bool g_notifyUpdate;
extern ReleaseInfo g_releaseInfo;

extern NativeMenu::Menu menu;
extern CarControls carControls;
extern ScriptSettings settings;

extern int prevNotification;
extern int textureWheelId;

const std::string escapeKey = "BACKSPACE";
const std::string skipKey = "RIGHT";

const std::string modUrl = "https://www.gta5-mods.com/scripts/manual-transmission-ikt";

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

struct STagInfo {
    std::string Tag;
    std::string Info;
};

const std::vector<STagInfo> keyboardConfTags = {
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


const std::vector<STagInfo> controllerConfTags = {
    { "Toggle"     , "Toggle mod usage: hold"      },
    { "ToggleShift", "Toggle shift usage: hold"    },
    { "ShiftUp"    , "Shift up usage: press"       },
    { "ShiftDown"  , "Shift down usage: press"     },
    { "Clutch"     , "Clutch usage: axis or button"},
    { "Engine"     , "Engine usage: press"         },
    { "Throttle"   , "Throttle: axis or button"    },
    { "Brake"      , "Brake: axis or button"       }
};

const std::vector<std::string> speedoTypes = {
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
    ControlVehicleExit
};

std::vector<std::string> blockableControlsHelp = {
    "None",
    "Aim",
    "Handbrake",
    "Attack",
    "Duck",
    "NextWeapon",
    "Cinematic Cam",
    "Exit Car"
};

int getBlockableControlIndex(int control) {
    for (size_t i = 0; i < blockableControls.size(); i++) {
        if (control == blockableControls[i])
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
    menu.Subtitle(fmt::format("~b~{}", DISPLAY_VERSION));

    if (MemoryPatcher::Error) {
        menu.Option("Patch test error", NativeMenu::solidRed, 
            { "One or more components can't be patched. Mod behavior is uncertain."
              "Usually caused by a game update or using an incompatible version.", 
              "Check Gears.log for more details." });
    }

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
            settings.IgnoredVersion = g_releaseInfo.Version;
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

    bool tempEnableRead = settings.EnableManual;
    if (menu.BoolOption("Enable manual transmission", tempEnableRead,
        { "Enable or disable the manual transmission. Steering wheel stays active." })) {
        toggleManual(!settings.EnableManual);
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

    if (settings.DisableInputDetect) {
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
            carControls.CheckGUIDs(settings.RegisteredGUIDs);
        }
    }
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

    menu.BoolOption("Clutch Creep", settings.ClutchCatching,
        { "Simulate clutch creep when stopped with clutch engaged." });

    menu.BoolOption("Hard rev limiter", settings.HardLimiter,
        { "Enforce rev limiter for reverse and top speed. No more infinite speed!" });

    menu.BoolOption("Default Neutral gear", settings.DefaultNeutral,
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

    menu.FloatOption("Clutch bite threshold", settings.ClutchThreshold, 0.0f, 1.0f, 0.05f,
        { "How far the clutch has to be lifted to start biting. This value should be lower than \"Stalling threshold\"." });
    menu.FloatOption("Stalling threshold", settings.StallingThreshold, 0.0f, 1.0f, 0.05f,
        { "How far the clutch has to be lifted to start stalling. This value should be higher than \"Clutch bite point\"." });
    menu.FloatOption("Stalling RPM", settings.StallingRPM, 0.0f, 0.2f, 0.01f,
        { "Consider stalling when the expected RPM drops below this number.",
          "The range is 0.0 to 0.2. The engine idles at 0.2, so 0.1 is a decent value." });

    menu.FloatOption("RPM Damage", settings.RPMDamage, 0.0f, 10.0f, 0.05f,
        { "Damage from redlining too long." });
    menu.IntOption("Misshift Damage", settings.MisshiftDamage, 0, 100, 5,
        { "Damage from being over the rev range." });
    menu.FloatOption("Engine braking threshold", settings.EngBrakeThreshold, 0.0f, 1.0f, 0.05f,
        { "RPM where engine braking starts being effective." });
    menu.FloatOption("Engine braking power", settings.EngBrakePower, 0.0f, 5.0f, 0.05f,
        { "Decrease this value if your wheels lock up when engine braking." });
}

void update_shiftingoptionsmenu() {
    menu.Title("Shifting options");
    menu.Subtitle("Auto & sequential shift options");

    menu.BoolOption("Cut throttle on upshift", settings.DownshiftBlip,
        { "Helps rev matching." });
    menu.BoolOption("Blip throttle on downshift", settings.UpshiftCut,
        { "Helps rev matching." });
    menu.FloatOption("Clutch rate multiplier", settings.ClutchRateMult, 0.05f, 20.0f, 0.05f,
        { "Change how fast clutching is. Below 1 is slower, higher than 1 is faster." });
}

void update_finetuneautooptionsmenu() {
    menu.Title("Automatic transmission finetuning");
    menu.Subtitle("Script-driven automatic transmission");

    menu.FloatOption("Upshift engine load", settings.UpshiftLoad, 0.01f, 0.20f, 0.01f,
        { "Upshift when the engine load drops below this value. "
          "Raise this value if the car can't upshift."});
    menu.FloatOption("Downshift engine load", settings.DownshiftLoad, 0.30f, 1.00f, 0.01f,
        { "Downshift when the engine load rises over this value. "
          "Raise this value if the car downshifts right after upshifting." });
    menu.FloatOption("Downshift timeout multiplier", settings.DownshiftTimeoutMult, 0.05f, 4.00f, 0.05f,
        { "Don't downshift while car has just shifted up. "
          "Timeout based on clutch change rate.",
          "Raise for longer timeout, lower to allow earlier downshifting after an upshift." });
    menu.FloatOption("Next gear min RPM",    settings.NextGearMinRPM, 0.20f, 0.50f, 0.01f, 
        { "Don't upshift until next gears' RPM is over this value." });
    menu.FloatOption("Current gear min RPM", settings.CurrGearMinRPM, 0.20f, 0.50f, 0.01f, 
        { "Downshift when RPM drops below this value." });
    menu.FloatOption("Economy rate", settings.EcoRate, 0.01f, 0.50f, 0.01f,
        { "On releasing throttle, high values cause earlier upshifts.",
          "Set this low to stay in gear longer when releasing throttle." });
}

void update_controlsmenu() {
    menu.Title("Controls");
    menu.Subtitle("Controls options");

    menu.MenuOption("Controller", "controllermenu",
        { "Change controller control assignments." });

    menu.MenuOption("Keyboard", "keyboardmenu",
        { "Change keyboard control assignments." });

    menu.BoolOption("Non-Xinput controller", carControls.UseLegacyController,
        { "If you needed to set up your controller in the pause menu, you should enable this." });

    menu.MenuOption("Non-Xinput controller", "legacycontrollermenu",
        { "Set up the non-Xinput controller with native controls." });

    if (menu.BoolOption("Patch steering for all inputs", settings.PatchSteeringAlways,
        { "Also patch steering reduction and automatic countersteer for keyboard and controller inputs.",
            "Only active if this patch is also enabled for steering wheels.",
            fmt::format("PatchSteering (Wheel) is {}.", settings.PatchSteering ? "enabled" : "disabled") })) {
        settings.SaveWheel(&carControls);
    }

    if (menu.FloatOption("Steering reduction (kb/controller)", settings.SteeringReductionOther, 0.0f, 1.0f, 0.01f,
        { "Reduce steering input at higher speeds.","From InfamousSabre's Custom Steering." })) {
        settings.SaveWheel(&carControls);
    }

    if (menu.FloatOption("Steering multiplier (kb/controller)", settings.GameSteerMultOther, 0.01f, 2.0f, 0.01f,
        { "Increase/decrease steering lock.","From InfamousSabre's Custom Steering." })) {
        settings.SaveWheel(&carControls);
    }

    menu.BoolOption("Block H-pattern on controller", settings.BlockHShift,
        { "Block H-pattern mode when using controller input." });
}

void update_legacycontrollermenu() {
    menu.Title("Non-Xinput controls");
    menu.Subtitle("Non-Xinput options");

    int oldIndexUp = getBlockableControlIndex(carControls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)]);
    if (menu.StringArray("Shift Up blocks", blockableControlsHelp, oldIndexUp)) {
        carControls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)] = blockableControls[oldIndexUp];
    }

    int oldIndexDown = getBlockableControlIndex(carControls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)]);
    if (menu.StringArray("Shift Down blocks", blockableControlsHelp, oldIndexDown)) {
        carControls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)] = blockableControls[oldIndexDown];
    }

    int oldIndexClutch = getBlockableControlIndex(carControls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)]);
    if (menu.StringArray("Clutch blocks", blockableControlsHelp, oldIndexClutch)) {
        carControls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)] = blockableControls[oldIndexClutch];
    }

    std::vector<std::string> controllerInfo = {
        "Press RIGHT to clear key",
        "Press RETURN to configure button",
        "",
    };

    auto it = 0;
    for (const auto& confTag : controllerConfTags) {
        controllerInfo.back() = confTag.Info;
        int nativeControl = carControls.ConfTagLController2Value(confTag.Tag);
        controllerInfo.push_back(fmt::format("Assigned to {} ({})", carControls.NativeControl2Text(nativeControl), nativeControl));

        if (menu.OptionPlus(fmt::format("Assign {}", confTag.Tag), controllerInfo, nullptr, std::bind(clearLControllerButton, confTag.Tag), nullptr, "Current setting")) {
            WAIT(500);
            bool result = configLControllerButton(confTag.Tag);
            if (!result) showNotification(fmt::format("Cancelled {} assignment", confTag.Tag), &prevNotification);
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

    menu.IntOption("Long press time (ms)", carControls.CToggleTime, 100, 5000, 50,
        { "Timeout for long press buttons to activate." });

    menu.IntOption("Max tap time (ms)", carControls.MaxTapTime, 50, 1000, 10,
        { "Buttons pressed and released within this time are regarded as a tap. Shift up, shift down are tap controls." });

    float currTriggerValue = carControls.GetControllerTrigger();
    float prevTriggerValue = currTriggerValue;
    menu.FloatOption("Trigger value", currTriggerValue, 0.25, 1.0, 0.05,
        { "Threshold for an analog input to be detected as button press." });

    if (currTriggerValue != prevTriggerValue) {
        carControls.SetControllerTriggerLevel(currTriggerValue);
    }

    menu.BoolOption("Block car controls", settings.BlockCarControls,
        { "Blocks car action controls. Holding activates the original button again.",
            "Experimental!" });

    menu.BoolOption("Ignore shifts in UI", settings.IgnoreShiftsUI,
        { "Ignore shift up/shift down while using the phone or when the menu is open" });

    int oldIndexUp = getBlockableControlIndex(carControls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftUp)]);
    if (menu.StringArray("Shift Up blocks", blockableControlsHelp, oldIndexUp)) {
        carControls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftUp)] = blockableControls[oldIndexUp];
    }

    int oldIndexDown = getBlockableControlIndex(carControls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftDown)]);
    if (menu.StringArray("Shift Down blocks", blockableControlsHelp, oldIndexDown)) {
        carControls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftDown)] = blockableControls[oldIndexDown];
    }

    int oldIndexClutch = getBlockableControlIndex(carControls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::Clutch)]);
    if (menu.StringArray("Clutch blocks", blockableControlsHelp, oldIndexClutch)) {
        carControls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::Clutch)] = blockableControls[oldIndexClutch];
    }

    std::vector<std::string> controllerInfo = {
        "Press RIGHT to clear key",
        "Press RETURN to configure button",
        "",
    };

    auto it = 0;
    for (const auto& confTag : controllerConfTags) {
        controllerInfo.back() = confTag.Info;
        controllerInfo.push_back(fmt::format("Assigned to {}", carControls.ConfTagController2Value(confTag.Tag)));
        if (menu.OptionPlus(fmt::format("Assign {}", confTag.Tag), controllerInfo, nullptr, std::bind(clearControllerButton, confTag.Tag), nullptr, "Current setting")) {
            WAIT(500);
            bool result = configControllerButton(confTag.Tag);
            if (!result) showNotification(fmt::format("Cancelled {} assignment", confTag.Tag), &prevNotification);
            WAIT(500);
        }
        it++;
        controllerInfo.pop_back();
    }
}

void update_keyboardmenu() {
    menu.Title("Keyboard controls");
    menu.Subtitle("Keyboard assignments");

    std::vector<std::string> keyboardInfo = {
        "Press RIGHT to clear key",
        "Press RETURN to configure button",
        "",
    };

    int it = 0;
    for (const auto& confTag : keyboardConfTags) {
        keyboardInfo.back() = confTag.Info;
        keyboardInfo.push_back(fmt::format("Assigned to {}", key2str(carControls.ConfTagKB2key(confTag.Tag))));
        if (menu.OptionPlus(fmt::format("Assign {}", confTag.Tag), keyboardInfo, nullptr, std::bind(clearKeyboardKey, confTag.Tag), nullptr, "Current setting")) {
            WAIT(500);
            bool result = configKeyboardKey(confTag.Tag);
            if (!result) showNotification(fmt::format("Cancelled {} assignment", confTag.Tag), &prevNotification);
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
        saveChanges();
        settings.Read(&carControls);
        initWheel();
    }

    if (menu.BoolOption("Always enable wheel", settings.WheelWithoutManual,
        { "Enable your wheel even when the Manual Transmission part of this mod is off." })) {
        settings.SaveWheel(&carControls);
    }

    if (menu.BoolOption("Patch steering", settings.PatchSteering,
        { "Patches steering reduction and automatic countersteer for more direct control.",
          "Recommended to keep this on." })) {
        settings.SaveWheel(&carControls);
    }

    if (menu.FloatOption("Steering reduction (wheel)", settings.SteeringReductionWheel, 0.0f, 1.0f, 0.01f,
        { "Reduce steering input at higher speeds. Best to leave it at 0 for steering wheels." })) {
        settings.SaveWheel(&carControls);
    }

    if (menu.FloatOption("Steering multiplier (wheel)", settings.GameSteerMultWheel, 0.1f, 2.0f, 0.01f,
        { "Increase steering lock for all cars. You might want to increase it for faster steering and more steering lock." })) {
    }

    if (menu.BoolOption("Disable non-wheel steering", settings.PatchSteeringControl,
        { "Disable keyboard and controller inputs for steering. This fixes the jerky animation.",
          "Recommended to keep this on." })) {
        settings.SaveWheel(&carControls);
    }

    if (menu.BoolOption("Logitech RPM LEDs", settings.LogiLEDs,
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
        showNotification(result ? "H-pattern shifter saved" : "Cancelled H-pattern shifter setup", &prevNotification);
    }

    std::vector<std::string> hAutoInfo = {
        "Press RIGHT to clear H-pattern auto",
        "Active gear:"
    };
    if (carControls.ButtonIn(CarControls::WheelControlType::AR))        hAutoInfo.emplace_back("Reverse");
    else if (carControls.ButtonIn(CarControls::WheelControlType::AD))   hAutoInfo.emplace_back("Drive");
    else                                                                hAutoInfo.emplace_back("Neutral");

    if (menu.OptionPlus("H-pattern shifter setup (auto)", hAutoInfo, nullptr, [] { clearASelect(); }, nullptr, "Input values",
        { "Select this option to start H-pattern shifter (auto) setup. Follow the on-screen instructions." })) {
        bool result = configASelect();
        showNotification(result ? "H-pattern shifter (auto) saved" : "Cancelled H-pattern shifter (auto) setup", &prevNotification);
    }

    menu.BoolOption("Keyboard H-pattern", settings.HPatternKeyboard,
        { "This allows you to use the keyboard for H-pattern shifting. Configure the controls in the keyboard section." });

    menu.BoolOption("Use shifter for automatic", settings.UseShifterForAuto,
        { "Use the H-pattern shifter to select the Drive/Reverse gears." });
}

void update_anglemenu() {
    menu.Title("Soft lock");
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

    menu.FloatOption("Steering deadzone", carControls.DZSteer, 0.0f, 0.5f, 0.01f,
        { "Deadzone size, from the center of the wheel." });

    menu.FloatOption("Steering deadzone offset", carControls.DZSteerOffset, -0.5f, 0.5f, 0.01f,
        { "Put the deadzone with an offset from the center." });

    if (!settings.PatchSteeringControl) {
        menu.FloatOption("Steering anti-deadzone", carControls.ADZSteer, 0.0f, 1.0f, 0.01f,
            { "GTA V ignores 25% input for analog controls by default." });
    }

    menu.FloatOption("Throttle anti-deadzone", carControls.ADZThrottle, 0.0f, 1.0f, 0.01f,
        { "GTA V ignores 25% input for analog controls by default." });

    menu.FloatOption("Brake anti-deadzone", carControls.ADZBrake, 0.0f, 1.0f, 0.01f,
        { "GTA V ignores 25% input for analog controls by default." });

    bool showBrakeGammaBox = false;
    std::vector<std::string> extras = {};
    menu.OptionPlus("Brake gamma", extras, &showBrakeGammaBox,
        [=] { return incGamma(settings.BrakeGamma, 5.0f, 0.01f); },
        [=] { return decGamma(settings.BrakeGamma, 0.1f, 0.01f); },
        "Brake gamma",
        { "Linearity of the brake pedal. Values over 1.0 feel more real if you have progressive springs." });

    if (showBrakeGammaBox) {
        extras = showGammaCurve("Brake", carControls.BrakeValAvg, settings.BrakeGamma);
        menu.OptionPlusPlus(extras, "Brake gamma");
    }

    bool showThrottleGammaBox = false;
    extras = {};
    menu.OptionPlus("Throttle gamma", extras, &showThrottleGammaBox,
        [=] { return incGamma(settings.ThrottleGamma, 5.0f, 0.01f); },
        [=] { return decGamma(settings.ThrottleGamma, 0.1f, 0.01f); },
        "Throttle gamma",
        { "Linearity of the throttle pedal." });

    if (showThrottleGammaBox) {
        extras = showGammaCurve("Throttle", carControls.ThrottleVal, settings.ThrottleGamma);
        menu.OptionPlusPlus(extras, "Throttle gamma");
    }

    bool showSteeringGammaBox = false;
    extras = {};
    menu.OptionPlus("Steering gamma", extras, &showSteeringGammaBox,
        [=] { return incGamma(settings.SteerGamma, 5.0f, 0.01f); },
        [=] { return decGamma(settings.SteerGamma, 0.1f, 0.01f); },
        "Steering gamma",
        { "Linearity of the steering wheel." });

    if (showSteeringGammaBox) {
        float steerValL = map(carControls.SteerVal, 0.0f, 0.5f, 1.0f, 0.0f);
        float steerValR = map(carControls.SteerVal, 0.5f, 1.0f, 0.0f, 1.0f);
        float steerVal = carControls.SteerVal < 0.5f ? steerValL : steerValR;
        extras = showGammaCurve("Steering", steerVal, settings.SteerGamma);
        menu.OptionPlusPlus(extras, "Steering gamma");
    }

    //menu.BoolOption("Invert steer", carControls.InvertSteer);
    //menu.BoolOption("Invert throttle", carControls.InvertThrottle);
    //menu.BoolOption("Invert brake", carControls.InvertBrake);
    //menu.BoolOption("Invert clutch", carControls.InvertClutch);
}

void update_forcefeedbackmenu() {
    menu.Title("Force feedback");
    menu.Subtitle("Force feedback setup");

    menu.BoolOption("Enable", settings.EnableFFB,
        { "Enable or disable force feedback entirely." });

    menu.BoolOption("Scale forces", settings.ScaleFFB,
        { "Scale forces to degree of rotation." });

    menu.FloatOption("Self aligning torque multiplier", settings.SATAmpMult, 0.1f, 10.0f, 0.05f,
        { "Force feedback strength for steering. Increase for weak wheels, decrease for strong/fast wheels.",
        "Putting this too high clips force feedback. Too low and the car doesn't feel responsive." });

    menu.FloatOption("Detail effect multiplier", settings.DetailMult, 0.0f, 10.0f, 0.1f,
        { "Force feedback effects caused by the suspension." });

    menu.FloatOption("Collision effect multiplier", settings.CollisionMult, 0.0f, 10.0f, 0.1f,
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
        showNotification(result ? "Entry added" : "Cancelled entry addition", &prevNotification);
    }

    std::vector<std::string> buttonInfo;
    buttonInfo.push_back("Press RIGHT to clear this button");
    buttonInfo.push_back("Active buttons:");
    if (carControls.ButtonIn(CarControls::WheelControlType::HR)) buttonInfo.push_back("Gear R");
    if (carControls.ButtonIn(CarControls::WheelControlType::H1)) buttonInfo.push_back("Gear 1");
    if (carControls.ButtonIn(CarControls::WheelControlType::H2)) buttonInfo.push_back("Gear 2");
    if (carControls.ButtonIn(CarControls::WheelControlType::H3)) buttonInfo.push_back("Gear 3");
    if (carControls.ButtonIn(CarControls::WheelControlType::H4)) buttonInfo.push_back("Gear 4");
    if (carControls.ButtonIn(CarControls::WheelControlType::H5)) buttonInfo.push_back("Gear 5");
    if (carControls.ButtonIn(CarControls::WheelControlType::H6)) buttonInfo.push_back("Gear 6");
    if (carControls.ButtonIn(CarControls::WheelControlType::H7)) buttonInfo.push_back("Gear 7");
    if (carControls.ButtonIn(CarControls::WheelControlType::H8)) buttonInfo.push_back("Gear 8");
    if (carControls.ButtonIn(CarControls::WheelControlType::H9)) buttonInfo.push_back("Gear 9");
    if (carControls.ButtonIn(CarControls::WheelControlType::H10)) buttonInfo.push_back("Gear 10");
    if (carControls.ButtonIn(CarControls::WheelControlType::AR)) buttonInfo.push_back("Auto R");
    if (carControls.ButtonIn(CarControls::WheelControlType::AD)) buttonInfo.push_back("Auto 1");
    if (carControls.ButtonIn(CarControls::WheelControlType::ShiftUp))   buttonInfo.push_back("ShiftUp");
    if (carControls.ButtonIn(CarControls::WheelControlType::ShiftDown)) buttonInfo.push_back("ShiftDown");
    if (carControls.ButtonIn(CarControls::WheelControlType::Clutch))    buttonInfo.push_back("ClutchButton");
    if (carControls.ButtonIn(CarControls::WheelControlType::Engine))    buttonInfo.push_back("Engine");
    if (carControls.ButtonIn(CarControls::WheelControlType::Handbrake)) buttonInfo.push_back("Handbrake");
    if (carControls.ButtonIn(CarControls::WheelControlType::Horn))      buttonInfo.push_back("Horn");
    if (carControls.ButtonIn(CarControls::WheelControlType::Lights))    buttonInfo.push_back("Lights");
    if (carControls.ButtonIn(CarControls::WheelControlType::LookBack))  buttonInfo.push_back("LookBack");
    if (carControls.ButtonIn(CarControls::WheelControlType::LookLeft))  buttonInfo.push_back("LookLeft");
    if (carControls.ButtonIn(CarControls::WheelControlType::LookRight)) buttonInfo.push_back("LookRight");
    if (carControls.ButtonIn(CarControls::WheelControlType::Camera))    buttonInfo.push_back("Camera");
    if (carControls.ButtonIn(CarControls::WheelControlType::RadioNext)) buttonInfo.push_back("RadioNext");
    if (carControls.ButtonIn(CarControls::WheelControlType::RadioPrev)) buttonInfo.push_back("RadioPrev");
    if (carControls.ButtonIn(CarControls::WheelControlType::IndicatorLeft))     buttonInfo.push_back("IndicatorLeft");
    if (carControls.ButtonIn(CarControls::WheelControlType::IndicatorRight))    buttonInfo.push_back("IndicatorRight");
    if (carControls.ButtonIn(CarControls::WheelControlType::IndicatorHazard))   buttonInfo.push_back("IndicatorHazard");
    if (carControls.ButtonIn(CarControls::WheelControlType::Toggle))    buttonInfo.push_back("ToggleMod");
    if (carControls.ButtonIn(CarControls::WheelControlType::ToggleH))   buttonInfo.push_back("ChangeShiftMode");
    if (buttonInfo.size() == 2) buttonInfo.push_back("None");

    for (auto confTag : buttonConfTags) {
        buttonInfo.push_back(fmt::format("Assigned to {}", carControls.ConfTagWheel2Value(confTag)));
        if (menu.OptionPlus(fmt::format("Assign {}", confTag), buttonInfo, nullptr, std::bind(clearButton, confTag), nullptr, "Current inputs")) {
            bool result = configButton(confTag);
            showNotification(fmt::format("[{}] {}", confTag, result ? "saved" : "assignment cancelled."), &prevNotification);
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
        wheelTexturePresent.emplace_back("Wheel texture not found!");
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
    int newPos = static_cast<int>(oldPos);
    menu.StringArray("Speedometer", speedoTypes, newPos);
    if (newPos != oldPos) {
        settings.Speedo = speedoTypes.at(newPos);
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

    menu.IntOption("RPM Background Red  ", settings.RPMIndicatorBackgroundR, 0, 255);
    menu.IntOption("RPM Background Green", settings.RPMIndicatorBackgroundG, 0, 255);
    menu.IntOption("RPM Background Blue ", settings.RPMIndicatorBackgroundB, 0, 255);
    menu.IntOption("RPM Background Alpha", settings.RPMIndicatorBackgroundA, 0, 255);

    menu.IntOption("RPM Foreground Red  ", settings.RPMIndicatorForegroundR, 0, 255);
    menu.IntOption("RPM Foreground Green", settings.RPMIndicatorForegroundG, 0, 255);
    menu.IntOption("RPM Foreground Blue ", settings.RPMIndicatorForegroundB, 0, 255);
    menu.IntOption("RPM Foreground Alpha", settings.RPMIndicatorForegroundA, 0, 255);

    menu.IntOption("RPM Redline Red     ", settings.RPMIndicatorRedlineR, 0, 255);
    menu.IntOption("RPM Redline Green   ", settings.RPMIndicatorRedlineG, 0, 255);
    menu.IntOption("RPM Redline Blue    ", settings.RPMIndicatorRedlineB, 0, 255);
    menu.IntOption("RPM Redline Alpha   ", settings.RPMIndicatorRedlineA, 0, 255);

    menu.IntOption("RPM Revlimit Red    ", settings.RPMIndicatorRevlimitR, 0, 255);
    menu.IntOption("RPM Revlimit Green  ", settings.RPMIndicatorRevlimitG, 0, 255);
    menu.IntOption("RPM Revlimit Blue   ", settings.RPMIndicatorRevlimitB, 0, 255);
    menu.IntOption("RPM Revlimit Alpha  ", settings.RPMIndicatorRevlimitA, 0, 255);
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
    menu.FloatOption("Pedals Height", settings.PedalInfoH, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Pedals Width", settings.PedalInfoW, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Pedals Pad X", settings.PedalInfoPadX, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Pedals Pad Y", settings.PedalInfoPadY, 0.0f, 1.0f, 0.01f);
    menu.IntOption("Pedals Background Alpha", settings.PedalBackgroundA, 0, 255);
    menu.IntOption("Throttle Bar Red    ", settings.PedalInfoThrottleR, 0, 255);
    menu.IntOption("Throttle Bar Green  ", settings.PedalInfoThrottleG, 0, 255);
    menu.IntOption("Throttle Bar Blue   ", settings.PedalInfoThrottleB, 0, 255);
    menu.IntOption("Throttle Bar Alpha  ", settings.PedalInfoThrottleA, 0, 255);
    menu.IntOption("Brake Bar Red    ", settings.PedalInfoBrakeR, 0, 255);
    menu.IntOption("Brake Bar Green  ", settings.PedalInfoBrakeG, 0, 255);
    menu.IntOption("Brake Bar Blue   ", settings.PedalInfoBrakeB, 0, 255);
    menu.IntOption("Brake Bar Alpha  ", settings.PedalInfoBrakeA   , 0, 255);
    menu.IntOption("Clutch Bar Red    ", settings.PedalInfoClutchR, 0, 255);
    menu.IntOption("Clutch Bar Green  ", settings.PedalInfoClutchG, 0, 255);
    menu.IntOption("Clutch Bar Blue   ", settings.PedalInfoClutchB, 0, 255);
    menu.IntOption("Clutch Bar Alpha  ", settings.PedalInfoClutchA  , 0, 255);
}

void update_miscassistmenu() {
    menu.Title("Drving assists");
    menu.Subtitle("Assists to make driving easier");

    menu.BoolOption("Enable ABS", settings.CustomABS,
        { "Experimental script-driven ABS." });
    menu.BoolOption("Only enable ABS if not present", settings.ABSFilter,
        { "Only enables script-driven ABS on vehicles without the ABS flag." });

    menu.BoolOption("Simple Bike", settings.SimpleBike,
        { "Disables bike engine stalling and the clutch bite simulation." });

    menu.BoolOption("Hill gravity workaround", settings.HillBrakeWorkaround,
        { "Gives the car a push to overcome the games' default brakes at a stop." });

    menu.BoolOption("Auto gear 1", settings.AutoGear1,
        { "Automatically switch to first gear when the car reaches a standstill." });

    menu.BoolOption("Auto look back", settings.AutoLookBack,
        { "Automatically look back whenever in reverse gear." });

    menu.BoolOption("Clutch + throttle start", settings.ThrottleStart,
        { "Allow to start the engine by pressing clutch and throttle." });

    menu.BoolOption("Hide player in FPV", settings.HidePlayerInFPV,
        { "Hides the player in first person view." });
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
            "Green: Vehicle velocity","Red: Vehicle rotation","Purple: Steering direction" });
    menu.BoolOption("Show NPC info", settings.ShowNPCInfo,
        { "Show vehicle info of NPC vehicles near you." });
    menu.BoolOption("Disable input detection", settings.DisableInputDetect);
    menu.BoolOption("Enable update check", settings.EnableUpdate,
        { "Check for mod updates."});

    if (menu.Option("Check for updates", { "Manually check for updates. "
        "Re-enables update checking and clears ignored update." })) {
        settings.EnableUpdate = true;
        settings.IgnoredVersion = "v0.0.0";
        if (CheckUpdate(g_releaseInfo)) {
            g_notifyUpdate = true;
            showNotification(fmt::format("Manual Transmission: Update available, new version: {}.", 
                g_releaseInfo.Version));
        }
        else {
            showNotification("Manual Transmission: No update available.");
        }
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

    /* mainmenu -> controlsmenu -> legacycontrollermenu */
    if (menu.CurrentMenu("legacycontrollermenu")) { update_legacycontrollermenu(); }

    /* mainmenu -> controlsmenu -> controllermenu */
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

    /* mainmenu -> debugmenu */
    if (menu.CurrentMenu("debugmenu")) { update_debugmenu(); }

    menu.EndMenu();
}


///////////////////////////////////////////////////////////////////////////////
//                              Config helpers/util
///////////////////////////////////////////////////////////////////////////////

// Wheel section
// Look at that argument list! :D
bool getConfigAxisWithValues(std::vector<std::tuple<GUID, std::string, int>> startStates, GUID &selectedGUID, std::string &selectedAxis, int hyst, bool &positive, int &startValue_) {
    for (auto guid : carControls.GetWheel().GetGuids()) {
        for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
            for (auto startState : startStates) {
                std::string axisName = carControls.GetWheel().DIAxisHelper[i];
                int axisValue = carControls.GetWheel().GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), guid);
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
    showNotification(fmt::format("Cleared axis {}", confTag), &prevNotification);
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
        showNotification(fmt::format("Invalid input: {} is not a valid number!", result), &prevNotification);
        return;
    }
    bool found = settings.SteeringClearWheelToKey(button);
    if (found) {
        saveChanges();
        showNotification(fmt::format("Removed button {}", result), &prevNotification);
        settings.Read(&carControls);
    }
    else {
        showNotification(fmt::format("Button {} not found.", result), &prevNotification);
    }
}

void clearButton(const std::string& confTag) {
    saveChanges();
    settings.SteeringSaveButton(confTag, -1, -1);
    settings.Read(&carControls);
    showNotification(fmt::format("Cleared button {}", confTag), &prevNotification);
}

void clearHShifter() {
    saveChanges();
    std::vector<int> empty(g_numGears);
    std::fill(empty.begin(), empty.end(), -1);

    settings.SteeringSaveHShifter("SHIFTER", -1, empty);
    settings.Read(&carControls);
    showNotification("Cleared H-pattern shifter", &prevNotification);
}

void clearASelect() {
    saveChanges();
    settings.SteeringSaveButton("AUTO_R", -1, -1);
    settings.SteeringSaveButton("AUTO_D", -1, -1);
    settings.Read(&carControls);
    showNotification("Cleared H-pattern shifter (auto)", &prevNotification);
}

// Controller and keyboard
void saveKeyboardKey(const std::string& confTag, const std::string& key) {
    saveChanges();
    settings.KeyboardSaveKey(confTag, key);
    settings.Read(&carControls);
    showNotification(fmt::format("Saved key {}: {}.", confTag, key), &prevNotification);
}

void saveControllerButton(const std::string& confTag, const std::string& button) {
    saveChanges();
    settings.ControllerSaveButton(confTag, button);
    settings.Read(&carControls);
    showNotification(fmt::format("Saved button {}: {}.", confTag, button), &prevNotification);
}

void saveLControllerButton(const std::string& confTag, int button) {
    saveChanges();
    settings.LControllerSaveButton(confTag, button);
    settings.Read(&carControls);
    showNotification(fmt::format("Saved button {}: {}", confTag, button), &prevNotification);
}

void clearKeyboardKey(const std::string& confTag) {
    saveChanges();
    settings.KeyboardSaveKey(confTag, "UNKNOWN");
    settings.Read(&carControls);
    showNotification(fmt::format("Cleared key {}", confTag), &prevNotification);
}

void clearControllerButton(const std::string& confTag) {
    saveChanges();
    settings.ControllerSaveButton(confTag, "UNKNOWN");
    settings.Read(&carControls);
    showNotification(fmt::format("Cleared button {}", confTag), &prevNotification);
}

void clearLControllerButton(const std::string& confTag) {
    saveChanges();
    settings.LControllerSaveButton(confTag, -1);
    settings.Read(&carControls);
    showNotification(fmt::format("Cleared button {}", confTag), &prevNotification);
}

///////////////////////////////////////////////////////////////////////////////
//                              Config inputs
///////////////////////////////////////////////////////////////////////////////
// Wheel
bool configAxis(const std::string& confTag) {
    std::string additionalInfo = fmt::format("Press {} to exit.", escapeKey);

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
    // Save current state TODO
    std::vector<std::tuple<GUID, std::string, int>> startStates;
    for (auto guid : carControls.GetWheel().GetGuids()) {
        for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
            std::string axisName = carControls.GetWheel().DIAxisHelper[i];
            int axisValue = carControls.GetWheel().GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), guid);
            startStates.emplace_back(guid, axisName, axisValue);
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
        carControls.UpdateValues(CarControls::InputDevices::Wheel, true);
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

    int prevAxisValue = carControls.GetWheel().GetAxisValue(carControls.GetWheel().StringToAxis(selectedAxis), selectedGUID);

    // and up again!
    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        carControls.UpdateValues(CarControls::InputDevices::Wheel, true);

        int axisValue = carControls.GetWheel().GetAxisValue(carControls.GetWheel().StringToAxis(selectedAxis), selectedGUID);

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

// I hate myself.
// TODO: Fix strings
bool configASelect() {
    std::string additionalInfo = fmt::format("Press {} to exit", escapeKey);
    GUID devGUID = {};
    std::array<int, 2> buttonArray{ -1, -1 };
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
                        saveButton("AUTO_R", devGUID, i);
                        progress++;
                    }
                    else if (guid == devGUID) { // only accept same device onwards
                        saveButton("AUTO_D", devGUID, i);
                        progress++;
                    }
                }
            }
        }

        if (progress > 1) {
            break;
        }
        std::string gearDisplay;
        switch (progress) {
        case 0: gearDisplay = "Reverse"; break;
        case 1: gearDisplay = "Drive"; break;
        default: gearDisplay = "?"; break;
        }
        showSubtitle(fmt::format("Shift into {}. {}", gearDisplay, additionalInfo));
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
                    showNotification("Can't use menu controls!", &prevNotification);
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
                    showNotification("Can't use menu controls!", &prevNotification);
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
    XInputController* rawController = carControls.GetRawController();
    if (rawController == nullptr)
        return false;

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        carControls.UpdateValues(CarControls::InputDevices::Controller, true);
        for (const std::string& buttonHelper : rawController->XboxButtonsHelper) {
            auto button = rawController->StringToButton(buttonHelper);
            if (rawController->IsButtonJustPressed(button)) {
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
