#include <conio.h>  
#include <cstdio>
#include <iostream>
#include <thread>
#include <fmt/format.h>
#include "../Gears/Input/WheelDirectInput.hpp"
#include "../Gears/ScriptSettings.hpp"
#include "../Gears/Input/CarControls.hpp"
#include "../Gears/Util/Paths.h"
#include "../Gears/Util/Util.hpp"
#include "../Gears/Constants.h"

#define ESC 0x1B
#define TAB 0x09

static const int devWidth = 36;

HANDLE hConsole;
CONSOLE_CURSOR_INFO cursorInfo;
RECT r;
HWND consoleWindow;
CONSOLE_SCREEN_BUFFER_INFO csbi;

CarControls controls;

std::string settingsGeneralFile = Paths::GetRunningExecutableFolder() + "\\settings_general.ini";
std::string settingsControlsFile = Paths::GetRunningExecutableFolder() + "\\settings_controls.ini";
std::string settingsWheelFile = Paths::GetRunningExecutableFolder() + "\\settings_wheel.ini";
ScriptSettings g_settings;

uint8_t g_numGears = 11;

// acceptedKeys, gameButton, confTag
std::vector<std::tuple<char, std::string, std::string>> buttonInfos = {
	std::make_tuple('=', "shift up",          "SHIFT_UP"),
	std::make_tuple('-', "shift down",        "SHIFT_DOWN"),
	std::make_tuple('x', "engine",            "ENGINE"),
	std::make_tuple(' ', "handbrake",         "HANDBRAKE"),
	std::make_tuple('e', "horn",              "HORN"),
	std::make_tuple('h', "lights",            "LIGHTS"),
	std::make_tuple('c', "look back",         "LOOK_BACK"),
	std::make_tuple('v', "camera",            "CHANGE_CAMERA"),
	std::make_tuple('q', "radio next",        "RADIO_NEXT"),
	std::make_tuple('r', "radio previous",    "RADIO_PREVIOUS"),
	std::make_tuple(',', "indicator left",    "INDICATOR_LEFT"),
	std::make_tuple('.', "indicator right",   "INDICATOR_RIGHT"),
	std::make_tuple('/', "indicator hazard",  "INDICATOR_HAZARD"),
	std::make_tuple('\\', "toggle mod",       "TOGGLE_MOD"),
	std::make_tuple(']', "change shift mode", "CHANGE_SHIFTMODE"),
};

std::vector<std::tuple<char, std::string, std::string>> axisInfos = {
	std::make_tuple('w', "throttle",          "THROTTLE"),
	std::make_tuple('s', "brake",             "BRAKE"),
	std::make_tuple('a', "clutch",            "CLUTCH"),
	std::make_tuple('d', "steering",          "STEER"),
	std::make_tuple('f', "handbrake",         "HANDBRAKE_ANALOG"),
};

/*
 * Console stuff
 */
void cls()
{
	// Get the Win32 handle representing standard output.
	// This generally only has to be done once, so we make it static.
	static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD topLeft = { 0, 0 };

	// std::cout uses a buffer to batch writes to the underlying console.
	// We need to flush that to the console because we're circumventing
	// std::cout entirely; after we clear the console, we don't want
	// stale buffered text to randomly be written out.
	std::cout.flush();

	// Figure out the current width and height of the console window
	if (!GetConsoleScreenBufferInfo(hOut, &csbi)) {
		// TO/DO Handle failure!
		abort();
	}
	DWORD length = csbi.dwSize.X * csbi.dwSize.Y;

	DWORD written;

	// Flood-fill the console with spaces to clear it
	FillConsoleOutputCharacter(hOut, TEXT(' '), length, topLeft, &written);

	// Reset the attributes of every character to the default.
	// This clears all background colour formatting, if any.
	FillConsoleOutputAttribute(hOut, csbi.wAttributes, length, topLeft, &written);

	// Move the cursor back to the top left for the next sequence of writes
	SetConsoleCursorPosition(hOut, topLeft);
}

void setCursorPosition(int x, int y)
{
	static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	std::cout.flush();
	COORD coord = { static_cast<SHORT>(x), static_cast<SHORT>(y) };
	SetConsoleCursorPosition(hOut, coord);
}

void blankLines(int y, int lines) {
	setCursorPosition(0, y);
	for (int i = 0; i < csbi.srWindow.Right * lines; i++) {
		printf(" ");
	}
}

void blankBlock(int x, int y, int lines, int rows) {
	for (int l = 0; l < lines; l++) {
		setCursorPosition(x, y + l);
		for (int r = 0; r < rows; r++) {
			printf(" ");
		}
	}
}

/*
 * Similar to reInit() in Gears but with console stuff
 */
void init() {	
	g_settings.SetFiles(settingsGeneralFile, settingsControlsFile,settingsWheelFile);
	g_settings.Read(&controls);
	logger.Write(INFO, "Settings read");

	controls.InitWheel();
	controls.CheckGUIDs(g_settings.Wheel.InputDevices.RegisteredGUIDs);
	
	int totalWidth = 0;
	for (auto guid : controls.GetWheel().GetGuids()) {
		//std::wstring wDevName = controls.GetWheel().FindEntryFromGUID(guid)->diDeviceInstance.tszInstanceName;
		totalWidth += devWidth;//static_cast<int>(wDevName.length()) + 4;
	}
	
	if (totalWidth < 80) {
		totalWidth = 80;
	}

	std::string modeStr = "MODE " + std::to_string(totalWidth) + ",40";
	system(modeStr.c_str());
}

/*
 * Simplified playWheelEffects() from in Gears
 */
void playWheelEffects(float effSteer) {
	if (g_settings.Wheel.Options.LogiLEDs) {
		controls.GetWheel().PlayLedsDInput(
			controls.WheelAxes[static_cast<int>(CarControls::WheelAxisType::Steer)].Guid,
			controls.ThrottleVal,
			0.5f, 0.95f);
	}

	auto totalForce = static_cast<int>(controls.ThrottleVal * 20000.0f * 2.0f * (controls.SteerVal - 0.5f));


	// steerSpeed is to dampen the steering wheel

	if (effSteer > 1.0f) {
		totalForce = static_cast<int>((effSteer - 1.0f) * 100000) + totalForce;
		if (effSteer > 1.1f) {
			totalForce = 10000;
		}
	}
	else if (effSteer < -1.0f) {
		totalForce = static_cast<int>((-effSteer - 1.0f) * -100000) + totalForce;
		if (effSteer < -1.1f) {
			totalForce = -10000;
		}
	}
	controls.PlayFFBDynamics(totalForce, 0);
}

/*
 * Fun starts here!
 */
bool getConfigAxisWithValues(
	const std::vector<std::tuple<GUID, std::string, int>>& startStates,
	std::tuple<GUID, std::string> &selectedDevice,
	int hyst,
	bool &positive, int &startValue_) {
	for (auto guid : controls.GetWheel().GetGuids()) {
		for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
			for (auto startState : startStates) {
				std::string axisName = controls.GetWheel().DIAxisHelper[i];
				int axisValue = controls.GetWheel().GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), guid);
				int startValue = std::get<2>(startState);
				if (std::get<0>(startState) == guid &&
					std::get<1>(startState) == axisName) {
					startValue_ = startValue;
					if (axisValue > startValue + hyst) { // 0 (min) - 65535 (max)
						selectedDevice = std::tuple<GUID, std::string>(guid, axisName);
						positive = true;
						return true;
					}
					if (axisValue < startValue - hyst) { // 65535 (min) - 0 (max)
						selectedDevice = std::tuple<GUID, std::string>(guid, axisName);
						positive = false;
						return true;
					}
				}
			}
		}
	}
	return false;
}

void saveAxis(const std::string& confTag, GUID devGUID, const std::string& axis, int min, int max) {
	std::wstring wDevName = controls.GetWheel().FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
	std::string devName = StrUtil::utf8_encode(wDevName);
	auto index = g_settings.SteeringAppendDevice(devGUID, devName);
	g_settings.SteeringSaveAxis(confTag, index, axis, min, max);
	g_settings.Read(&controls);
}

void saveButton(int button, const std::string& confTag, GUID devGUID, const std::string& devName) {
	auto index = g_settings.SteeringAppendDevice(devGUID, devName);
	g_settings.SteeringSaveButton(confTag, index, button);
}

void clearHShifter() {
	for (uint8_t i = 0; i < g_numGears; ++i) {
		g_settings.SteeringSaveButton(fmt::format("HPATTERN_{}", i), -1, -1);
	}
	g_settings.Read(&controls);
}

void saveHShifter(const std::string& confTag, GUID devGUID, const std::vector<int>& buttonArray) {
	std::wstring wDevName = controls.GetWheel().FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
	std::string devName = StrUtil::utf8_encode(wDevName);
	auto index = g_settings.SteeringAppendDevice(devGUID, devName);
	for (uint8_t i = 0; i < buttonArray.size(); ++i) {
		g_settings.SteeringSaveButton(fmt::format("HPATTERN_{}", i), index, buttonArray[i]);
	}
	g_settings.Read(&controls);
}

std::tuple<char, std::string, std::string> isAcceptedAxisChar(char c) {
	for (auto t : axisInfos) {
		if (c == std::get<0>(t))
			return t;
	}
	return{};
}

/*
 * 1. User selects input to be configured
 *		- Show "Press w/a/s/d"
 * 2. User presses input to be tied to game axis
 *		- Show "AWAITING INPUT"
 * 3. Save result
 * 0. User can exit at any time
 * 0. Pressing TAB erases the config.
 */
void configDynamicAxes(char c) {
	struct SAxisState {
		GUID Guid;
		WheelDirectInput::DIAxis Axis;
		int ValueStart;
		int ValueEnd;
	};
	
	auto axisInfo = isAcceptedAxisChar(c);
	if (std::get<1>(axisInfo).empty()) {
		return;
	}

	cls();

	std::string gameAxis = std::get<1>(axisInfo);
	std::string confTag = std::get<2>(axisInfo);

	std::string additionalInfo;

	bool confSteer = confTag == "STEER";
	auto axisInfosSteerIt = std::find_if(std::begin(axisInfos), std::end(axisInfos), [](auto&& e) {
		return std::get<1>(e) == "steering";
	});
	auto steerTupleIndex = axisInfosSteerIt - axisInfos.begin();
	if (c == std::get<0>(axisInfos.at(steerTupleIndex))) {
		additionalInfo = "Steer right to register axis";
	}

	auto axisInfosHBrkIt = std::find_if(std::begin(axisInfos), std::end(axisInfos), [](auto&& e) {
		return std::get<1>(e) == "handbrake";
	});
	auto hBrkTupleIndex = axisInfosHBrkIt - axisInfos.begin();
	if (c == std::get<0>(axisInfos.at(hBrkTupleIndex))) {
		additionalInfo = "Pull handbrake to register axis";
	}

	controls.UpdateValues(CarControls::InputDevices::Wheel, true);
	
	// Save current state
	std::vector<SAxisState> axisStates;
	for (auto guid : controls.GetWheel().GetGuids()) {
		for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
			auto axis = static_cast<WheelDirectInput::DIAxis>(i);
			int axisValue = controls.GetWheel().GetAxisValue(axis, guid);
			axisStates.push_back({ guid, axis, axisValue, axisValue });
		}
	}

	// Ignore inputs before selection if they moved less than hyst
	int hyst = 65536/8;

	// To track direction of physical <-> digital value.
	bool found = false;

	while (!found) {
		if (_kbhit()) {
			cls();
			char c = _getch();
			if (c == ESC) {
				return;
			}
			if (c == TAB) {
				g_settings.SteeringSaveAxis(confTag, -1, "", 0, 0);
				cls();
				setCursorPosition(0, csbi.srWindow.Bottom);
				printf("Cleared axis settings");
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				cls();
				init();
				return;
			}
		}
		controls.UpdateValues(CarControls::InputDevices::Wheel, true);

		for (auto guid : controls.GetWheel().GetGuids()) {
			for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
				auto axis = static_cast<WheelDirectInput::DIAxis>(i);
				for (auto& axisState : axisStates) {
					if (axisState.Guid != guid)
						continue;

					if (axisState.Axis != axis)
						continue;

					int axisValue = controls.GetWheel().GetAxisValue(axis, guid);

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

		std::string probableInput = "awaiting input...";
		for (SAxisState& axisState : axisStates) {
			if (abs(axisState.ValueStart - axisState.ValueEnd) > hyst) {
				std::string axisName = controls.GetWheel().DIAxisHelper[axisState.Axis];
				std::wstring wDevName = controls.GetWheel().FindEntryFromGUID(axisState.Guid)->diDeviceInstance.tszInstanceName;
				std::string devName = StrUtil::utf8_encode(wDevName);
				probableInput = "using " + axisName + " on " + devName;
			}
		}

		setCursorPosition(0, 0);
		printf("Configuring %s", gameAxis.c_str());
		setCursorPosition(0, 1);
		printf("Input detected on: %s", probableInput.c_str());
		
		setCursorPosition(0, csbi.srWindow.Bottom - 3);
		printf("If nothing seems to happen, press ESC and try again");
		setCursorPosition(0, csbi.srWindow.Bottom - 2);
		if (additionalInfo.empty()) {
			printf("Fully press and release the %s pedal to register axis", gameAxis.c_str());
		}
		else {
			printf("%s", additionalInfo.c_str());
		}
		
		setCursorPosition(0, csbi.srWindow.Bottom);
		std::cout << "ESC: Cancel config - TAB: Clear setting";
		std::cout.flush();
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
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
	std::string axisName = controls.GetWheel().DIAxisHelper[foundAxis.Axis];

	// Use full range instead of registered values, since steering is full range.
	if (confSteer) {
		int min = foundAxis.ValueEnd > foundAxis.ValueStart ? 0 : 65535;
		int max = foundAxis.ValueEnd > foundAxis.ValueStart ? 65535 : 0;
		saveAxis(confTag, foundAxis.Guid, axisName, min, max);
		return;
	}

	int min = foundAxis.ValueStart;
	int max = foundAxis.ValueEnd;
	saveAxis(confTag, foundAxis.Guid, axisName, min, max);
	
	cls();
	setCursorPosition(0, csbi.srWindow.Bottom);
	printf("Saved changes");
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	cls();
	init();
}

std::tuple<char, std::string, std::string> isAcceptedButtonChar(char c) {
	for (auto t : buttonInfos) {
		if (c == std::get<0>(t))
			return t;
	}
	return {};
}

void configDynamicButtons(char c) {
	auto buttonInfo = isAcceptedButtonChar(c);
	if (std::get<1>(buttonInfo).empty()) {
		return;
	}

	cls();

	std::string gameButton = std::get<1>(buttonInfo);
	std::string confTag = std::get<2>(buttonInfo);
	int buttonsActive = 0;

	// Check whether any buttons had been pressed already
	controls.UpdateValues(CarControls::InputDevices::Wheel, true);

	for (auto guid : controls.GetWheel().GetGuids()) {
		for (int i = 0; i < 255; i++) {
			if (controls.GetWheel().IsButtonPressed(i, guid)) {
				buttonsActive++;
			}
		}
		for (auto d : WheelDirectInput::POVDirections) {
			if (controls.GetWheel().IsButtonPressed(d, guid)) {
				buttonsActive++;
			}
		}
	}

	if (buttonsActive > 0) {
		setCursorPosition(0, 0);
		printf("Some button was already pressed on start. Stop pressing that and try again.");
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		cls();
		init();
		return;
	}

	while (true) {
		if (_kbhit()) {
			cls();
			char c = _getch();
			if (c == ESC) {
				return;
			}
			if (c == TAB) {
				g_settings.SteeringSaveButton(confTag, -1, -1);
				cls();
				setCursorPosition(0, csbi.srWindow.Bottom);
				printf("Cleared button settings");
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				cls();
				init();
				return;
			}
		}
		controls.UpdateValues(CarControls::InputDevices::Wheel, true);

		blankLines(0, 5); 
		setCursorPosition(0, 0);
		printf("Button for %s: ", gameButton.c_str());

		for (auto guid : controls.GetWheel().GetGuids()) {
			std::wstring wDevName = controls.GetWheel().FindEntryFromGUID(guid)->diDeviceInstance.tszInstanceName;
			std::string devName = StrUtil::utf8_encode(wDevName);
			for (int i = 0; i < 255; i++) {
				if (controls.GetWheel().IsButtonPressed(i, guid)) {
					printf("%d @ %s", i, devName.c_str());
					saveButton(i, confTag, guid, devName);
					blankLines(csbi.srWindow.Bottom - 5, 6);
					setCursorPosition(0, csbi.srWindow.Bottom);
					printf("Saved changes");
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					cls();
					init();
					return;
				}
			}

			//POV hat
			std::string directionsStr = "?";
			for (auto d : WheelDirectInput::POVDirections) {
				if (controls.GetWheel().IsButtonPressed(d, guid)) {
					if (d == WheelDirectInput::N) directionsStr = "N ";
					if (d == WheelDirectInput::NE) directionsStr = "NE";
					if (d == WheelDirectInput::E) directionsStr = " E";
					if (d == WheelDirectInput::SE) directionsStr = "SE";
					if (d == WheelDirectInput::S) directionsStr = "S ";
					if (d == WheelDirectInput::SW) directionsStr = "SW";
					if (d == WheelDirectInput::W) directionsStr = " W";
					if (d == WheelDirectInput::NW) directionsStr = "NW";
					printf("%s (POV hat) @ %s", directionsStr.c_str(), devName.c_str());
					saveButton(d, confTag, guid, devName);
					blankLines(csbi.srWindow.Bottom - 5, 6);
					setCursorPosition(0, csbi.srWindow.Bottom);
					printf("Saved changes");
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					cls();
					init();
					return;
				}
			}
		}

		setCursorPosition(0, csbi.srWindow.Bottom - 1);
		printf("Configuring %s", gameButton.c_str());
		setCursorPosition(0, csbi.srWindow.Bottom);
		std::cout << "ESC: Cancel config - TAB: Clear button";
		std::cout.flush();
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}

/*
 * Sadly this doesn't seem viable to make it dynamically detect stuff
 * But pressing [RETURN] a bunch of times shouldn't be *that* problematic ;)
 * 
 * 2. User presses g to config h-shifter
 *		- Show "Select device"
 * 3. User presses [device]
 *		- Show "Select 1st gear and press Enter"
 * 4. User presses RETURN
 *		- Show "Select 2nd gear and press Enter"
 * X. User presses RETURN
 *		- Show "Select reverse gear and press Enter"
 * X. User presses RETURN
 *		- User is done
 * 0. User presses ESC
 *		- Cancel at any time, do not save.
 */
void configHShift(char c) {
	if (c != 'g')
		return;
	std::string gameButton;
	std::string confTag;
	if (c == 'g') {
		gameButton = "H-shifter";
		confTag = "SHIFTER";
	}

	int devEntry = -1;

	int buttonsActive = 0;
	GUID devGUID = {};
	std::vector<int> buttonArray(g_numGears);
	std::fill(buttonArray.begin(), buttonArray.end(), -1);
	std::string devName;
	std::wstring wDevName;
	cls();
	int progress = 0;

	while(true) {
		controls.UpdateValues(CarControls::InputDevices::Wheel, true);
		if (_kbhit()) {
			cls();
			char c = _getch();
			if (c == ESC) {
				return;
			}
			if (c == TAB) {
				std::vector<int> empty(g_numGears);
				clearHShifter();
				cls();
				setCursorPosition(0, csbi.srWindow.Bottom);
				printf("Cleared H-shifter settings");
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				cls();
				init();
				return;
			}
			if (progress == 0) { // Device selection

				for (int i = 0; i < controls.GetWheel().GetGuids().size(); i++) {
					if (c == i + '0') {
						devEntry = i;
						int devNumber = 0;
						for (auto guid : controls.GetWheel().GetGuids()) {
							if (devNumber == devEntry) {
								devGUID = guid;
							}
							devNumber++;
						}
						progress++;
					}
				}
				continue;
			}
			if (progress >= 1 && progress <= g_numGears) { // Gear config (8 is for reverse)
				// Gear reverse goes in [0] btw way
				if (c == 0x0D) { // RETURN
					if (buttonsActive > 1) {
						blankLines(0, 5);
						setCursorPosition(0, 0);
						printf("More than 1 button pressed. You can only select one.");
						std::this_thread::yield();
						std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					}
					else if (buttonsActive == 0) {
						blankLines(0, 5);
						setCursorPosition(0, 0);
						printf("No buttons pressed. Skipping this gear!");
						std::this_thread::yield();
						std::this_thread::sleep_for(std::chrono::milliseconds(1000));
						progress++;
					} else {
						progress++;
					}
				}
			}
		}

		blankLines(0, 5);
		buttonsActive = 0;

		switch (progress) {
			case 0: { // Device selection
				setCursorPosition(0, 0);
				int devNumber = 0;
				for (auto guid : controls.GetWheel().GetGuids()) {
					wDevName = controls.GetWheel().FindEntryFromGUID(guid)->diDeviceInstance.tszInstanceName;
					devName = StrUtil::utf8_encode(wDevName);
					std::cout << devNumber << ": " << devName << "\n";
					devNumber++;
				}

				setCursorPosition(0, csbi.srWindow.Bottom - 2);
				printf("Select device for %s", gameButton.c_str());
			}
			break;
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10: {
				setCursorPosition(0, 0);
				printf("Select for gear %d:", progress);
				setCursorPosition(0, 1);
				for (int i = 0; i < 255; i++) {
					if (controls.GetWheel().IsButtonPressed(i, devGUID)) {
						devName = StrUtil::utf8_encode(wDevName);
						printf("%d @ %s", i, devName.c_str());
						buttonsActive++;
						buttonArray[progress] = i;
					}
				}
				if (buttonsActive==0) {
					buttonArray[progress] = -1;
				}
				setCursorPosition(0, csbi.srWindow.Bottom - 3);
				printf("Press ENTER to confirm or skip");
				setCursorPosition(0, csbi.srWindow.Bottom - 2);
				printf("Using device %d: %s", devEntry, devName.c_str());
			}
			break;
			case 11: { // Reverse gear
				setCursorPosition(0, 0);
				printf("Select for reverse gear");
				setCursorPosition(0, 1);
				for (int i = 0; i < 255; i++) {
					if (controls.GetWheel().IsButtonPressed(i, devGUID)) {
						devName = StrUtil::utf8_encode(wDevName);
						printf("%d @ %s", i, devName.c_str());
						buttonsActive++;
						buttonArray[0] = i;
					}
				}
				if (buttonsActive == 0) {
					buttonArray[0] = -1;
				}
				setCursorPosition(0, csbi.srWindow.Bottom - 3);
				printf("Press ENTER to confirm, save and exit");
				setCursorPosition(0, csbi.srWindow.Bottom - 2);
				printf("Using device %d: %s", devEntry, devName.c_str());
			}
			break;
			default: {
				saveHShifter(confTag, devGUID, buttonArray);
				cls();
				setCursorPosition(0, csbi.srWindow.Bottom);
				printf("Saved changes");
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				cls();
				init();
				return;
			}
		}

		setCursorPosition(0, csbi.srWindow.Bottom - 1);
		printf("Configuring %s", gameButton.c_str());
		setCursorPosition(0, csbi.srWindow.Bottom);
		std::cout << "ESC: Cancel config - ENTER: Confirm selection - TAB: Clear H-shifter";
		std::cout.flush();
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}


int main() {
	std::string logFile = Paths::GetRunningExecutableFolder() + "\\" + Paths::GetRunningExecutableNameWithoutExtension() + ".log";
	logger.SetMinLevel(DEBUG);
	logger.SetFile(logFile);

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = false;
	SetConsoleCursorInfo(hConsole, &cursorInfo);

	consoleWindow = GetConsoleWindow();
	GetWindowRect(consoleWindow, &r);
	
	logger.Clear();
	logger.Write(INFO, "Manual Transmission %s - Wheel setup tool", Constants::DisplayVersion);

	init();
	bool justPeeking = true;
	while (true)
	{
		if (_kbhit()) {
			char c = _getch();
			if (c == TAB) {
				init();
			}
			else if (c == ESC) {
				return 0;
			}
			else if (c == 'p') {
				justPeeking = !justPeeking;
				setCursorPosition(0, csbi.srWindow.Bottom);
				blankLines(csbi.srWindow.Bottom, 1);
			}
			else {
				configDynamicButtons(c);
				configHShift(c);
				configDynamicAxes(c);
			}
		}
		controls.GetLastInputDevice(CarControls::InputDevices::Wheel);
		controls.UpdateValues(CarControls::InputDevices::Wheel, justPeeking);

		float steerMult = g_settings.Wheel.Steering.AngleMax / g_settings.Wheel.Steering.AngleCar;
		float effSteer = steerMult * 2.0f * (controls.SteerVal - 0.5f);
		
		GetConsoleScreenBufferInfo(hConsole, &csbi);

		setCursorPosition(0, 0);
		printf("MT %s - Wheel setup tool - Troubleshooting only! - Use the in-game menu!", Constants::DisplayVersion);

		int guidIt = 0;
		int pRowMax = 0;
		for (auto guid : controls.GetWheel().GetGuids()) {
			if (!controls.GetWheel().FindEntryFromGUID(guid)) {
				init();
				break;
			}

			int pRow = 2;
			int xCursorPos = devWidth * guidIt;
			setCursorPosition(xCursorPos, pRow);
			pRow++;
			std::wstring wDevName = controls.GetWheel().FindEntryFromGUID(guid)->diDeviceInstance.tszInstanceName;
			std::string devName = StrUtil::utf8_encode(wDevName);
			if (devName.length() > devWidth) {
				devName.replace(devWidth - 4, 3, "...");
				devName[devWidth - 1] = '\0';
			}
			printf("%s", devName.c_str());
			setCursorPosition(xCursorPos, pRow);
			pRow++;
			std::cout << "Axes: ";

			for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
				blankBlock(xCursorPos, pRow, 1, devWidth);
				setCursorPosition(xCursorPos, pRow);
				printf("    %s: %d",
					   controls.GetWheel().DIAxisHelper[i].c_str(),
					   controls.GetWheel().GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), guid));
				pRow++;
			}

			setCursorPosition(xCursorPos, pRow);
			pRow++;

			std::cout << "Buttons: ";
			blankBlock(xCursorPos, pRow, 1, devWidth);
			setCursorPosition(xCursorPos, pRow);
			for (int i = 0; i < 255; i++) {
				if (controls.GetWheel().IsButtonPressed(i, guid)) {
					std::cout << i << " ";
				}
			}
			pRow++;

			setCursorPosition(xCursorPos, pRow);
			pRow++; // newline

			setCursorPosition(xCursorPos, pRow);
			pRow++;
			std::cout << "POV hat: ";

			std::string directionsStr;
			blankBlock(xCursorPos, pRow, 1, devWidth);
			setCursorPosition(xCursorPos, pRow);
			for (auto d : WheelDirectInput::POVDirections) {
				if (d == WheelDirectInput::N ) directionsStr = "N ";
				if (d == WheelDirectInput::NE) directionsStr = "NE";
				if (d == WheelDirectInput::E ) directionsStr = " E";
				if (d == WheelDirectInput::SE) directionsStr = "SE";
				if (d == WheelDirectInput::S ) directionsStr = "S ";
				if (d == WheelDirectInput::SW) directionsStr = "SW";
				if (d == WheelDirectInput::W ) directionsStr = " W";
				if (d == WheelDirectInput::NW) directionsStr = "NW";
				if (controls.GetWheel().IsButtonPressed(d, guid)) {
					printf("%05d (%s)", d, directionsStr.c_str());
				}
			}
			pRow++;

			setCursorPosition(xCursorPos, pRow);
			pRow++; // newline

			pRowMax = pRow;
			guidIt++;
		}

		blankBlock(8, pRowMax, 7, 32);
		setCursorPosition(0, pRowMax);
		std::cout << "Throttle  " << controls.ThrottleVal << "\n";
		std::cout << "Brake     " << controls.BrakeVal << "\n";
		std::cout << "Clutch    " << controls.ClutchVal << "\n";
		std::cout << "Handbrake " << controls.HandbrakeVal << "\n";
		std::cout << "Steer     " << controls.SteerVal << "\n";
		std::cout << std::fixed << "Softlock  " << effSteer << "\n";

		std::string gear = "N";
		if (controls.ButtonIn(CarControls::WheelControlType::HR)) gear = "R";
		for (uint32_t i = 1; i < 11; ++i) {
			if (controls.ButtonIn(CarControls::WheelControlType::H1)) gear = std::to_string(i);
		}

		std::cout << "Gear      " << gear;
		pRowMax += 6; // Because we printed text 6 lines

		blankLines(pRowMax+2, 1); // Because we want a blank link in between
		setCursorPosition(0, pRowMax+2);
		std::cout << "Active buttons: ";
		if (controls.ButtonIn(CarControls::WheelControlType::ShiftUp)) std::cout << "ShiftUp ";
		if (controls.ButtonIn(CarControls::WheelControlType::ShiftDown)) std::cout << "ShiftDown ";
		if (controls.ButtonIn(CarControls::WheelControlType::Engine)) std::cout << "Engine ";
		if (controls.ButtonIn(CarControls::WheelControlType::Handbrake)) std::cout << "Handbrake ";
		if (controls.ButtonIn(CarControls::WheelControlType::Horn)) std::cout << "Horn ";
		if (controls.ButtonIn(CarControls::WheelControlType::Lights)) std::cout << "Lights ";
		if (controls.ButtonIn(CarControls::WheelControlType::LookBack)) std::cout << "LookBack ";
		if (controls.ButtonIn(CarControls::WheelControlType::Camera)) std::cout << "Camera ";
		if (controls.ButtonIn(CarControls::WheelControlType::RadioNext)) std::cout << "RadioNext ";
		if (controls.ButtonIn(CarControls::WheelControlType::RadioPrev)) std::cout << "RadioPrev ";
		if (controls.ButtonIn(CarControls::WheelControlType::IndicatorLeft)) std::cout << "IndicatorLeft ";
		if (controls.ButtonIn(CarControls::WheelControlType::IndicatorRight)) std::cout << "IndicatorRight ";
		if (controls.ButtonIn(CarControls::WheelControlType::IndicatorHazard)) std::cout << "IndicatorHazard ";
		if (controls.ButtonIn(CarControls::WheelControlType::Toggle)) std::cout << "ToggleMod ";
		if (controls.ButtonIn(CarControls::WheelControlType::ToggleH)) std::cout << "ChangeShiftMode ";

		if (controls.GetWheel().IsConnected(controls.WheelAxes[static_cast<int>(CarControls::WheelAxisType::ForceFeedback)].Guid))
			playWheelEffects(effSteer);
		
		setCursorPosition(0, csbi.srWindow.Bottom - 9);
		std::cout << "Press a key to assign a button:";
		setCursorPosition(0, csbi.srWindow.Bottom - 8);
		int charsPrinted = 0;
		for (auto t : buttonInfos) {
			char tempCharBuff[256];
			sprintf_s(tempCharBuff, "(%c: %s) ", std::get<0>(t), std::get<1>(t).c_str());
			if (charsPrinted % csbi.srWindow.Right + strlen(tempCharBuff) >= csbi.srWindow.Right) {
				charsPrinted += printf("\n");
			}
			charsPrinted += printf("(%c: %s) ", std::get<0>(t), std::get<1>(t).c_str());
			if (t == buttonInfos.back()) {
				printf("(g: h-shifter)");
			}
		}

		setCursorPosition(0, csbi.srWindow.Bottom - 2);
		std::cout << "Press a key to assign an axis:";
		setCursorPosition(0, csbi.srWindow.Bottom - 1);
		charsPrinted = 0;
		for (auto t : axisInfos) {
			char tempCharBuff[256];
			sprintf_s(tempCharBuff, "(%c: %s) ", std::get<0>(t), std::get<1>(t).c_str());
			if (charsPrinted % csbi.srWindow.Right + strlen(tempCharBuff) >= csbi.srWindow.Right) {
				charsPrinted += printf("\n");
			}
			charsPrinted += printf("(%c: %s) ", std::get<0>(t), std::get<1>(t).c_str());
		}

		setCursorPosition(0, csbi.srWindow.Bottom);
		std::string peekTxt = justPeeking ? "Enable Wheel2Keyboard" : "Disable Wheel2Keyboard";
		printf("ESC: Exit - Tab: Reload - P: %s", peekTxt.c_str());
		std::cout.flush();
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
		//cls();
	}
}	
