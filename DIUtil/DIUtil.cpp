// DIUtil.cpp : Defines the entry point for the console application.

#include <conio.h>  
#include <stdio.h>  
#include <iostream>
#include "../Gears/Input/WheelDirectInput.hpp"
#include "../Gears/ScriptSettings.hpp"
#include "../Gears/Input/ScriptControls.hpp"
#include <thread>

/* Standard error macro for reporting API errors */
#define PERR(bSuccess, api){if(!(bSuccess)) printf("%s:Error %d from %s on line %d\n", __FILE__, GetLastError(), api, __LINE__);}

HANDLE hConsole;
CONSOLE_CURSOR_INFO cursorInfo;
RECT r;
HWND consoleWindow;
CONSOLE_SCREEN_BUFFER_INFO csbi;

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
	COORD coord = { (SHORT)x, (SHORT)y };
	SetConsoleCursorPosition(hOut, coord);
}
Logger logger("./DIUtil.log");
ScriptControls controls(logger);
ScriptSettings settings("./settings_general.ini","./settings_wheel.ini", logger);

void init() {
	settings.Read(&controls);
	logger.Write("Settings read");

	controls.InitWheel();
	controls.CheckGUIDs(settings.reggdGuids);
	controls.SteerAxisType = ScriptControls::WheelAxisType::Steer;
	controls.SteerGUID = controls.WheelAxesGUIDs[static_cast<int>(controls.SteerAxisType)];
	
	int totalWidth = 0;
	for (auto guid : controls.WheelDI.GetGuids()) {
		std::wstring wDevName = controls.WheelDI.FindEntryFromGUID(guid)->diDeviceInstance.tszInstanceName;
		totalWidth += static_cast<int>(wDevName.length()) + 4;
	}
	
	if (totalWidth < 80) {
		totalWidth = 80;
	}
	std::string modeStr = "MODE " + std::to_string(totalWidth) + ",32";
	system(modeStr.c_str());
}

void playWheelEffects(float effSteer) {
	if (settings.LogiLEDs) {
		controls.WheelDI.PlayLedsDInput(controls.SteerGUID, controls.ThrottleVal, 0.5f, 0.95f);
	}

	auto totalForce = static_cast<int>(controls.ThrottleVal * 20000.0f * 2.0f * (controls.SteerVal - 0.5f));

	auto adjustRatio = static_cast<float>(settings.DamperMax) / static_cast<float>(settings.TargetSpeed);
	auto damperForce = settings.DamperMax - static_cast<int>(10 * (1.0-controls.BrakeVal) * adjustRatio);

	if (damperForce < settings.DamperMin) {
		damperForce = settings.DamperMin;
	}
	// steerSpeed is to dampen the steering wheel
	auto steerAxis = controls.WheelDI.StringToAxis(controls.WheelAxes[static_cast<int>(controls.SteerAxisType)]);
	auto steerSpeed = controls.WheelDI.GetAxisSpeed(steerAxis, controls.SteerGUID) / 20;

	totalForce += (int)(damperForce * 0.1f * steerSpeed);

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
	controls.WheelDI.SetConstantForce(controls.SteerGUID, totalForce);
}

bool getConfigAxisWithValues(std::vector<std::tuple<GUID, std::string, int>> startStates, std::tuple<GUID, std::string> &selectedDevice, int hyst, bool &positive, int &startValue_) {
	for (auto guid : controls.WheelDI.GetGuids()) {
		for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
			for (auto startState : startStates) {
				std::string axisName = controls.WheelDI.DIAxisHelper[i];
				int axisValue = controls.WheelDI.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), guid);
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

void saveAxis(std::string gameAxis, std::string confTag, std::tuple<GUID, std::string> selectedDevice, int min, int max) {
	std::wstring wDevName = controls.WheelDI.FindEntryFromGUID(std::get<0>(selectedDevice))->diDeviceInstance.tszInstanceName;
	std::string devName = std::string(wDevName.begin(), wDevName.end());
	int index = settings.SteeringAppendDevice(std::get<0>(selectedDevice), devName);
	settings.SteeringSaveAxis(confTag, index, std::get<1>(selectedDevice), min, max);
	if (gameAxis == "steering") {
		settings.SteeringSaveFFBAxis(confTag, index, std::get<1>(selectedDevice));
	}
}

/*
 * 1. User selects input to be configured
 *		- Show "Press w/a/s/d"
 * 2. User presses input to be tied to game axis
 *		- Show "AWAITING INPUT"
 * 3. Save result
 * 0. User can exit at any time
 */
void configDynamicAxes(char c) {
	if (c != 'w' && c != 's' && c != 'c' && c != 'd' && c != 'h') {
		return;
	}
	std::string gameAxis;
	std::string confTag;
	std::string additionalInfo;

	if (c == 'w') {
		gameAxis = "throttle";
		confTag = "THROTTLE";
	}
	if (c == 's') {
		gameAxis = "brake";
		confTag = "BRAKES";
	}
	if (c == 'c') {
		gameAxis = "clutch";
		confTag = "CLUTCH";
	}
	if (c == 'd') {
		gameAxis = "steering";
		confTag = "STEER";
		additionalInfo = "Steer right to register axis";
	}
	if (c == 'h') {
		gameAxis = "handbrake";
		confTag = "HANDBRAKE_ANALOG";
		additionalInfo = "Pull handbrake to register axis";
	}

	controls.UpdateValues(ScriptControls::InputDevices::Wheel, false);
	
	// Save current state
	std::vector<std::tuple<GUID, std::string, int>> startStates;
	for (auto guid : controls.WheelDI.GetGuids()) {
		for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
			std::string axisName = controls.WheelDI.DIAxisHelper[i];
			int axisValue = controls.WheelDI.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), guid);
			startStates.push_back(std::tuple<GUID, std::string, int>(guid, axisName, axisValue));
		}
	}

	std::tuple<GUID, std::string> selectedDevice;

	// Ignore inputs before selection if they moved less than hyst
	int hyst = 65536/8;

	// To track direction of physical <-> digital value.
	bool positive = true;

	int startValue;
	int endValue = -1;

	cls();
	while (true) {
		if (_kbhit()) {
			cls();
			char c = _getch();
			if (c == 0x1B) { // ESC
				return;
			}
		}
		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false);

		if (getConfigAxisWithValues(startStates, selectedDevice, hyst, positive, startValue)) {
			cls();
			break;
		}


		setCursorPosition(0, csbi.srWindow.Bottom - 3);
		printf("If nothing seems to happen, press ESC and try again");
		setCursorPosition(0, csbi.srWindow.Bottom - 2);
		if (additionalInfo.empty()) {
			printf("Fully press and release the %s pedal to register axis", gameAxis.c_str());
		}
		else {
			printf("%s", additionalInfo.c_str());
		}
		setCursorPosition(0, csbi.srWindow.Bottom - 1);
		printf("Configuring %s, awaiting input...", gameAxis.c_str());
		setCursorPosition(0, csbi.srWindow.Bottom);
		std::cout << "ESC: Cancel config";
		std::cout.flush();
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	if (gameAxis == "steering") {
		int min = (positive ? 0 : 65535);
		int max = (positive ? 65535 : 0);
		saveAxis(gameAxis, confTag, selectedDevice, min, max);
		cls();
		setCursorPosition(0, csbi.srWindow.Bottom);
		printf("Saved changes");
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		cls();
		init();
		return;
	}

	int prevAxisValue = controls.WheelDI.GetAxisValue(controls.WheelDI.StringToAxis(std::get<1>(selectedDevice)), std::get<0>(selectedDevice));
	std::wstring wDevName = controls.WheelDI.FindEntryFromGUID(std::get<0>(selectedDevice))->diDeviceInstance.tszInstanceName;
	std::string selectedDevName = std::string(wDevName.begin(), wDevName.end()).c_str();
	std::string selectedAxis = std::get<1>(selectedDevice);
	while(true) {
		if (_kbhit()) {
			cls();
			char c = _getch();
			if (c == 0x1B) { // ESC
				return;
			}
		}
		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false);
		
		int axisValue = controls.WheelDI.GetAxisValue(controls.WheelDI.StringToAxis(std::get<1>(selectedDevice)), std::get<0>(selectedDevice));

		if (positive && axisValue < prevAxisValue) {
			endValue = prevAxisValue;
			cls();
			break;
		}

		if (!positive && axisValue > prevAxisValue) {
			endValue = prevAxisValue;
			cls();
			break;
		}

		prevAxisValue = axisValue;

		setCursorPosition(0, csbi.srWindow.Bottom - 3);
		printf("If nothing seems to happen, press ESC and try again");
		setCursorPosition(0, csbi.srWindow.Bottom - 2);
		if (additionalInfo.empty()) {
			printf("Fully press and release the %s pedal to register axis", gameAxis.c_str());
		}
		else {
			printf("%s", additionalInfo.c_str());
		}
		setCursorPosition(0, 0);
		printf("Using axis %s on %s", selectedAxis.c_str(), selectedDevName.c_str());
		setCursorPosition(0, csbi.srWindow.Bottom - 1);
		printf("Configuring %s, awaiting input...", gameAxis.c_str());
		setCursorPosition(0, csbi.srWindow.Bottom);
		std::cout << "ESC: Cancel config";
		std::cout.flush();
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	int min = startValue;// (positive ? 0 : 65535);
	int max = endValue;// (positive ? 65535 : 0);
	saveAxis(gameAxis, confTag, selectedDevice, min, max);
	cls();
	setCursorPosition(0, csbi.srWindow.Bottom);
	printf("Saved changes");
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	cls();
	init();
}

void configDynamicButtons() {
	
}

void blankLines(int y, int lines) {
	setCursorPosition(0, y);
	for (int i = 0; i < csbi.srWindow.Right * lines; i++) {
		printf(" ");
	}
}

void configButtons(char c) {
	if (c != '+' && c != '-' && c != '=' && c != '_') {
		return;
	}
	std::string gameButton;
	std::string confTag;
	int buttonsActive = 0;
	if (c == '+' || c == '=') {
		gameButton = "shift up";
		confTag = "SHIFT_UP";
	}
	if (c == '-' || c == '_') {
		gameButton = "shift down";
		confTag = "SHIFT_DOWN";
	}
	GUID devGUID = {};
	int button;
	std::string devName;

	cls();
	while (true) {
		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false);
		if (_kbhit()) {
			cls();
			char c = _getch();
			if (c == 0x1B) { // ESC
				return;
			}
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
					printf("No buttons pressed. Select one.");
					std::this_thread::yield();
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				} else {
					int index = settings.SteeringAppendDevice(devGUID, devName.c_str());
					settings.SteeringSaveButton(confTag, index, button);
					cls();
					setCursorPosition(0, csbi.srWindow.Bottom);
					printf("Saved changes");
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					cls();
					init();
					return;
				}

			}
		}

		blankLines(0, 5);
		buttonsActive = 0;
		devGUID = {};
		button = -1;
		setCursorPosition(0, 0);
		printf("Button for %s: ", gameButton.c_str());

		for(auto guid : controls.WheelDI.GetGuids()) {
			std::wstring wDevName = controls.WheelDI.FindEntryFromGUID(guid)->diDeviceInstance.tszInstanceName;
			devName = std::string(wDevName.begin(), wDevName.end()).c_str();
			for (int i = 0; i < 255; i++) {
				if (controls.WheelDI.IsButtonPressed(i, guid)) {
					printf("%d @ %s", i, devName.c_str());
					buttonsActive++;
					button = i;
					devGUID = guid;
				}
			}
			//POV hat shit
			std::string directionsStr = "?";
			std::vector<int> directions;
			directions.push_back(WheelDirectInput::POV::N);
			directions.push_back(WheelDirectInput::POV::NE);
			directions.push_back(WheelDirectInput::POV::E);
			directions.push_back(WheelDirectInput::POV::SE);
			directions.push_back(WheelDirectInput::POV::S);
			directions.push_back(WheelDirectInput::POV::SW);
			directions.push_back(WheelDirectInput::POV::W);
			directions.push_back(WheelDirectInput::POV::NW);
			for (auto d : directions) {
				if (controls.WheelDI.IsButtonPressed(d, guid)) {
					if (d == WheelDirectInput::N) directionsStr  = "N ";
					if (d == WheelDirectInput::NE) directionsStr = "NE";
					if (d == WheelDirectInput::E) directionsStr  = " E";
					if (d == WheelDirectInput::SE) directionsStr = "SE";
					if (d == WheelDirectInput::S) directionsStr  = "S ";
					if (d == WheelDirectInput::SW) directionsStr = "SW";
					if (d == WheelDirectInput::W) directionsStr  = " W";
					if (d == WheelDirectInput::NW) directionsStr = "NW";
					printf("%s (POV hat) @ %s", directionsStr.c_str(), devName.c_str());
					buttonsActive++;
					button = d;
					devGUID = guid;
				}
			}
		}

		setCursorPosition(0, csbi.srWindow.Bottom - 1);
		printf("Configuring %s", gameButton.c_str());
		setCursorPosition(0, csbi.srWindow.Bottom);
		std::cout << "ESC: Cancel config - ENTER: Confirm selection";
		std::cout.flush();
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}

/*
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
	std::array<int, 8> buttonArray; // There are gears 1-7 + R
	std::fill(buttonArray.begin(), buttonArray.end(), -1);
	std::string devName;
	std::wstring wDevName;
	cls();
	int progress = 0;

	while(true) {
		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false);
		if (_kbhit()) {
			cls();
			char c = _getch();
			if (c == 0x1B) { // ESC
				return;
			}
			if (progress == 0) { // Device selection
				for (int i = 0; i < controls.WheelDI.nEntry; i++) {
					if (c == i + '0') {
						devEntry = i;
						int devNumber = 0;
						for (auto guid : controls.WheelDI.GetGuids()) {
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
			if (progress >= 1 && progress <= 8) { // Gear config (8 is for reverse)
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
				for (auto guid : controls.WheelDI.GetGuids()) {
					wDevName = controls.WheelDI.FindEntryFromGUID(guid)->diDeviceInstance.tszInstanceName;
					devName = std::string(wDevName.begin(), wDevName.end()).c_str();
					std::cout << devNumber << ": " << devName << "\n";
					devNumber++;
				}

				setCursorPosition(0, csbi.srWindow.Bottom - 2);
				printf("Select device for %s (0 to %d)", gameButton.c_str(), controls.WheelDI.nEntry - 1);
			}
			break;
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7: { // Muh GEARS
				setCursorPosition(0, 0);
				printf("Select for gear %d:", progress);
				setCursorPosition(0, 1);
				for (int i = 0; i < 255; i++) {
					if (controls.WheelDI.IsButtonPressed(i, devGUID)) {
						devName = std::string(wDevName.begin(), wDevName.end()).c_str();
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
			case 8: { // Reverse gear
				setCursorPosition(0, 0);
				printf("Select for reverse gear");
				setCursorPosition(0, 1);
				for (int i = 0; i < 255; i++) {
					if (controls.WheelDI.IsButtonPressed(i, devGUID)) {
						devName = std::string(wDevName.begin(), wDevName.end()).c_str();
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
			default: { // save all the shit
				int index = settings.SteeringAppendDevice(devGUID, devName);
				settings.SteeringSaveHShifter(confTag, index, buttonArray.data());
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
		std::cout << "ESC: Cancel config - ENTER: Confirm selection";
		std::cout.flush();
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}

int main()
{
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = false;
	SetConsoleCursorInfo(hConsole, &cursorInfo);

	consoleWindow = GetConsoleWindow();
	GetWindowRect(consoleWindow, &r);

	logger.Clear();
	logger.Write("Manual Transmission v4.2.0 - DirectInput utility");

	init();
	
	while (true)
	{
		if (_kbhit()) {
			char c = _getch();
			switch (c) {
				case ' ':
					init();
					break;
				case 0x1B: // ESC
					return 0;
				default:
					//configAxis(c);
					configButtons(c);
					configHShift(c);
					configDynamicAxes(c);
					break;
			}
		}
		controls.GetLastInputDevice(ScriptControls::InputDevices::Wheel);
		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false);

		float steerMult;
		steerMult = settings.SteerAngleMax / settings.SteerAngleCar;
		float effSteer = steerMult * 2.0f * (controls.SteerVal - 0.5f);
		
		GetConsoleScreenBufferInfo(hConsole, &csbi);

		int guidIt = 0;
		int pRowMax = 0;
		for (auto guid : controls.WheelDI.GetGuids()) {
			int pRow = 0;

			setCursorPosition(32 * guidIt, pRow);
			pRow++;
			std::wstring wDevName = controls.WheelDI.FindEntryFromGUID(guid)->diDeviceInstance.tszInstanceName;
			std::cout << std::string(wDevName.begin(), wDevName.end());
			
			setCursorPosition(32 * guidIt, pRow);
			pRow++;
			std::cout << "Axes: ";

			for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
				setCursorPosition(32 * guidIt, pRow);
				std::cout << "                                ";
				setCursorPosition(32 * guidIt, pRow);
				std::cout << "    " << controls.WheelDI.DIAxisHelper[i] << ": " << controls.WheelDI.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), guid) << "\n";
				pRow++;
			}

			setCursorPosition(32 * guidIt, pRow);
			pRow++;

			std::cout << "Buttons: ";
			setCursorPosition(32 * guidIt, pRow);
			std::cout << "                                ";
			setCursorPosition(32 * guidIt, pRow);
			for (int i = 0; i < 255; i++) {
				if (controls.WheelDI.IsButtonPressed(i, guid)) {
					std::cout << i << " ";
				}
			}
			pRow++;

			setCursorPosition(32 * guidIt, pRow);
			pRow++; // newline

			setCursorPosition(32 * guidIt, pRow);
			pRow++;
			std::cout << "POV hat: ";
			std::vector<int> directions;
			std::string directionsStr;
			directions.push_back(WheelDirectInput::POV::N);
			directions.push_back(WheelDirectInput::POV::NE);
			directions.push_back(WheelDirectInput::POV::E);
			directions.push_back(WheelDirectInput::POV::SE);
			directions.push_back(WheelDirectInput::POV::S);
			directions.push_back(WheelDirectInput::POV::SW);
			directions.push_back(WheelDirectInput::POV::W);
			directions.push_back(WheelDirectInput::POV::NW);
			setCursorPosition(32 * guidIt, pRow);
			std::cout << "                                ";
			setCursorPosition(32 * guidIt, pRow);
			for (auto d : directions) {
				if (d == WheelDirectInput::N) directionsStr  = "N ";
				if (d == WheelDirectInput::NE) directionsStr = "NE";
				if (d == WheelDirectInput::E) directionsStr  = " E";
				if (d == WheelDirectInput::SE) directionsStr = "SE";
				if (d == WheelDirectInput::S) directionsStr  = "S ";
				if (d == WheelDirectInput::SW) directionsStr = "SW";
				if (d == WheelDirectInput::W) directionsStr  = " W";
				if (d == WheelDirectInput::NW) directionsStr = "NW";
				if (controls.WheelDI.IsButtonPressed(d, guid)) {
					printf("%05d (%s)", d, directionsStr.c_str());
				}
			}
			pRow++;

			setCursorPosition(32 * guidIt, pRow);
			pRow++; // newline

			pRowMax = pRow;
			guidIt++;
		}

		setCursorPosition(8, pRowMax+0);
		std::cout << "                                ";
		setCursorPosition(8, pRowMax+1);
		std::cout << "                                ";
		setCursorPosition(8, pRowMax+2);
		std::cout << "                                ";
		setCursorPosition(8, pRowMax+3);
		std::cout << "                                ";
		setCursorPosition(8, pRowMax+4);
		std::cout << "                                ";
		setCursorPosition(8, pRowMax+5);
		std::cout << "                                ";

		setCursorPosition(0, pRowMax);
		std::cout << "Throttle  " << controls.ThrottleVal << "\n";
		std::cout << "Brake     " << controls.BrakeVal << "\n";
		std::cout << "Clutch    " << controls.ClutchVal << "\n";
		std::cout << "Handbrake " << controls.HandbrakeVal << "\n";
		std::cout << "Steer     " << controls.SteerVal << "\n";
		std::cout << std::fixed << "Softlock  " << effSteer << "\n";

		std::string gear = "N";
		if (controls.ButtonIn(ScriptControls::WheelControlType::H1))
			gear = "1";
		if (controls.ButtonIn(ScriptControls::WheelControlType::H2))
			gear = "2";
		if (controls.ButtonIn(ScriptControls::WheelControlType::H3))
			gear = "3";
		if (controls.ButtonIn(ScriptControls::WheelControlType::H4))
			gear = "4";
		if (controls.ButtonIn(ScriptControls::WheelControlType::H5))
			gear = "5";
		if (controls.ButtonIn(ScriptControls::WheelControlType::H6))
			gear = "6";
		if (controls.ButtonIn(ScriptControls::WheelControlType::H7))
			gear = "7";
		if (controls.ButtonIn(ScriptControls::WheelControlType::HR))
			gear = "R";

		std::cout << "Gear      " << gear << "\n";

		if (!controls.WheelDI.NoFeedback && controls.WheelDI.IsConnected(controls.SteerGUID))
			playWheelEffects(effSteer);

		setCursorPosition(0, csbi.srWindow.Bottom - 4);
		std::cout << "Press a key to assign a button:";
		setCursorPosition(0, csbi.srWindow.Bottom - 3);
		std::cout << "g: shifter - +/=: shift up - -/_: shift down";
		setCursorPosition(0, csbi.srWindow.Bottom-2);
		std::cout << "Press a key to assign an axis:";
		setCursorPosition(0, csbi.srWindow.Bottom-1);
		std::cout << "w: throttle - s: brake - d: steer - c: clutch - h: handbrake";
		setCursorPosition(0, csbi.srWindow.Bottom);
		std::cout << "ESC: Exit - Space: Reload";
		std::cout.flush();
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
		//cls();
	}
}	
