#include "InputConfiguration.h"

#include "script.h"

#include "ScriptSettings.hpp"

#include "Input/WheelDirectInput.hpp"
#include "Input/XInputController.hpp"
#include "Input/NativeController.h"
#include "Input/keyboard.h"

#include "Util/UIUtils.h"
#include "Util/Strings.hpp"

#include "Memory/VehicleExtensions.hpp"

#include <menu.h>
#include <menukeyboard.h>
#include <inc/natives.h>

#include <fmt/format.h>

#include <string>

using VExt = VehicleExtensions;

extern NativeMenu::Menu g_menu;
extern CarControls g_controls;
extern ScriptSettings g_settings;

namespace {
    const std::string escapeKey = "BACKSPACE";
    const std::string skipKey = "RIGHT";
}

///////////////////////////////////////////////////////////////////////////////
//                              Config helpers/util
///////////////////////////////////////////////////////////////////////////////
// Wheel section

void saveAxis(const std::string& confTag, GUID devGUID, const std::string& axis, int min, int max) {
    saveAllSettings();
    std::string devName = g_controls.GetWheel().GetDeviceInfo(devGUID)->DeviceInstance.tszInstanceName;
    auto index = g_settings.SteeringAppendDevice(devGUID, devName);
    g_settings.SteeringSaveAxis(confTag, index, axis, min, max);
    g_settings.Read(&g_controls);
}

void saveButton(const std::string& confTag, GUID devGUID, int button) {
    saveAllSettings();
    std::string devName = g_controls.GetWheel().GetDeviceInfo(devGUID)->DeviceInstance.tszInstanceName;
    auto index = g_settings.SteeringAppendDevice(devGUID, devName);
    g_settings.SteeringSaveButton(confTag, index, button);
    g_settings.Read(&g_controls);
}

void addWheelToKey(const std::string& confTag, GUID devGUID, int button, const std::string& keyName) {
    saveAllSettings();
    std::string devName = g_controls.GetWheel().GetDeviceInfo(devGUID)->DeviceInstance.tszInstanceName;
    auto index = g_settings.SteeringAppendDevice(devGUID, devName);
    g_settings.SteeringAddWheelToKey(confTag, index, button, keyName);
    g_settings.Read(&g_controls);
}

void saveHShifter(const std::string& confTag, GUID devGUID, const std::vector<int>& buttonArray) {
    saveAllSettings();
    std::string devName = g_controls.GetWheel().GetDeviceInfo(devGUID)->DeviceInstance.tszInstanceName;
    auto index = g_settings.SteeringAppendDevice(devGUID, devName);

    for (uint8_t i = 0; i < buttonArray.size(); ++i) {
        g_settings.SteeringSaveButton(fmt::format("HPATTERN_{}", i), index, buttonArray[i]);
    }

    g_settings.Read(&g_controls);
}

void clearAxis(const std::string& confTag) {
    saveAllSettings();
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
        saveAllSettings();
        UI::Notify(WARN, fmt::format("Removed button {}", result));
        g_settings.Read(&g_controls);
    }
    else {
        UI::Notify(WARN, fmt::format("Button {} not found.", result));
    }
}

void clearButton(const std::string& confTag) {
    saveAllSettings();
    g_settings.SteeringSaveButton(confTag, -1, -1);
    g_settings.Read(&g_controls);
    UI::Notify(WARN, fmt::format("Cleared button {}", confTag));
}

void clearHShifter() {
    saveAllSettings();
    for (uint8_t i = 0; i < VExt::GearsAvailable(); ++i) {
        g_settings.SteeringSaveButton(fmt::format("HPATTERN_{}", i), -1, -1);
    }
    g_settings.Read(&g_controls);
    UI::Notify(WARN, "Cleared H-pattern shifter");
}

void clearASelect() {
    saveAllSettings();
    g_settings.SteeringSaveButton("AUTO_P", -1, -1);
    g_settings.SteeringSaveButton("AUTO_R", -1, -1);
    g_settings.SteeringSaveButton("AUTO_N", -1, -1);
    g_settings.SteeringSaveButton("AUTO_D", -1, -1);
    g_settings.Read(&g_controls);
    UI::Notify(WARN, "Cleared H-pattern shifter (auto)");
}

// Controller and keyboard
void saveKeyboardKey(const std::string& confTag, const std::string& key) {
    saveAllSettings();
    g_settings.KeyboardSaveKey(confTag, key);
    g_settings.Read(&g_controls);
    UI::Notify(WARN, fmt::format("Saved key {}: {}.", confTag, key));
}

void saveControllerButton(const std::string& confTag, const std::string& button) {
    saveAllSettings();
    g_settings.ControllerSaveButton(confTag, button);
    g_settings.Read(&g_controls);
    UI::Notify(WARN, fmt::format("Saved button {}: {}.", confTag, button));
}

void saveLControllerButton(const std::string& confTag, int button) {
    saveAllSettings();
    g_settings.LControllerSaveButton(confTag, button);
    g_settings.Read(&g_controls);
    UI::Notify(WARN, fmt::format("Saved button {}: {}", confTag, button));
}

void clearKeyboardKey(const std::string& confTag) {
    saveAllSettings();
    g_settings.KeyboardSaveKey(confTag, "UNKNOWN");
    g_settings.Read(&g_controls);
    UI::Notify(WARN, fmt::format("Cleared key {}", confTag));
}

void clearControllerButton(const std::string& confTag) {
    saveAllSettings();
    g_settings.ControllerSaveButton(confTag, "UNKNOWN");
    g_settings.Read(&g_controls);
    UI::Notify(WARN, fmt::format("Cleared button {}", confTag));
}

void clearLControllerButton(const std::string& confTag) {
    saveAllSettings();
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

    std::string additionalInfo;
    bool confSteer = confTag == "STEER";
    if (confTag == "STEER") {
        additionalInfo = "Steer right to register axis.";
    }
    else if (confTag == "HANDBRAKE_ANALOG") {
        additionalInfo = "Fully pull and set back handbrake to register axis.";
    }
    else {
        additionalInfo = fmt::format("Fully press and release the {} pedal to register axis.", confTag);
    }
    additionalInfo += fmt::format(" Press [{}] to exit.", escapeKey);

    g_controls.UpdateValues(CarControls::InputDevices::Wheel, true);
    // Save current state
    std::vector<SAxisState> axisStates;
    for (const auto& [guid, device] : g_controls.GetWheel().GetDevices()) {
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

        for (const auto& [guid, device] : g_controls.GetWheel().GetDevices()) {
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

        UI::ShowHelpText(additionalInfo);
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
    std::string additionalInfo = fmt::format("Press a button to configure. Press [{}] to exit", escapeKey);

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
            for (const auto& [guid, device] : g_controls.GetWheel().GetDevices()) {
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
                additionalInfo = fmt::format("Press a keyboard key to configure. Press [{}] to exit", escapeKey);
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
        UI::ShowHelpText(additionalInfo);
        WAIT(0);
    }
}

bool configButton(const std::string& confTag) {
    std::string additionalInfo = fmt::format("Press a button to set {}. Press [{}] to exit.", confTag, escapeKey);

    g_controls.UpdateValues(CarControls::InputDevices::Wheel, true);

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }
        g_controls.UpdateValues(CarControls::InputDevices::Wheel, true);

        for (const auto& [guid, device] : g_controls.GetWheel().GetDevices()) {
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
        UI::ShowHelpText(additionalInfo);
        WAIT(0);
    }
}

bool configHPattern() {
    std::string confTag = "SHIFTER";
    std::string additionalInfo = fmt::format("Press [{}] to skip gear. Press [{}] to exit.", skipKey, escapeKey);

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

        for (const auto& [guid, device] : g_controls.GetWheel().GetDevices()) {
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
        UI::ShowHelpText(fmt::format("Shift into {}. {}", gearDisplay, additionalInfo));
        WAIT(0);
    }
    saveHShifter(confTag, devGUID, buttonArray);
    return true;
}

bool configASelect() {
    std::string additionalInfo = fmt::format("Press [{}] to exit.", escapeKey);
    GUID devGUID = {};
    std::array<int, 4> buttonArray{ -1, -1, -1, -1 };
    int progress = 0;

    while (true) {
        if (IsKeyJustUp(str2key(escapeKey))) {
            return false;
        }

        g_controls.UpdateValues(CarControls::InputDevices::Wheel, true);

        for (const auto& [guid, device] : g_controls.GetWheel().GetDevices()) {
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
            UI::ShowHelpText(fmt::format("Shift into {}. {}", gearDisplay, additionalInfoN));
        }
        else {
            UI::ShowHelpText(fmt::format("Shift into {}. {}", gearDisplay, additionalInfo));
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

bool configKeyboardKey(const std::string& confTag) {
    std::string additionalInfo = fmt::format("Press [{}] to exit.", escapeKey);
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

        UI::ShowHelpText(fmt::format("Press {}. Menu keys can't be chosen. {}", confTag, additionalInfo));
        WAIT(0);
    }
}

// Controller
bool configControllerButton(const std::string& confTag) {
    std::string additionalInfo = fmt::format("Press [{}] to exit.", escapeKey);
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
        UI::ShowHelpText(fmt::format("Press {}. {}", confTag, additionalInfo));
        WAIT(0);
    }
}

bool configLControllerButton(const std::string& confTag) {
    std::string additionalInfo = fmt::format("Press [{}] to exit", escapeKey);

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

        UI::ShowHelpText(fmt::format("Press {}. {}", confTag, additionalInfo));
        WAIT(0);
    }
}
