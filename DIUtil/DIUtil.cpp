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

/*
 * 1. User selects input to be configured
 *		- Show "Press W/A/S/D/Z for steer/throttle etc"
 * 2. User presses WASDZ to config axis
 *		- Show "Press 1-6 for axis etc"
 * 3. User presses 1-6 to choose which game axis is which hw axis
 *		- Show "Steer left/lift foot etc"
 *		- Show "Press Enter to register"
 * 4. User presses RETURN
 *		- Show "Steer right/press pedal to desired max pos"
 *		- Show "Press Enter to register"
 * 5. User presses RETURN
 *		- Save to file
 *		- We done now!
 * 0. User presses ESC
 *		- Cancel at any time, do not save.
 */
// Why did I make this monstrosity?!
void configAxis(char c) {
	if (c != 'w' && c != 's' && c != 'c' && c != 'a' && c != 'd' && c != 'h') {
		return;
	}
	std::string gameAxis;
	std::string confTag;
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
	if (c == 'a' || c == 'd') {
		gameAxis = "steering";
		confTag = "STEER";
	}
	if (c == 'h') {
		gameAxis = "handbrake";
		confTag = "HANDBRAKE_ANALOG";
	}

	int progress = 0;
	int devEntry = -1;
	int devAxis = -1;
	std::array<std::string, 8> devAxisH {
		"lX	",
		"lY	",
		"lZ	",
		"lRx",
		"lRy",
		"lRz",
		"rglSlider0",
		"rglSlider1"
	};
	GUID devGUID = {};
	std::wstring wDevName;
	int minVal = -1;
	int maxVal = -1;

	cls();
	while (true) {
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
			if (progress == 1) { // Axis selection
				for (int i = 1; i <= 8; i++) {
					if (c == i + '0') {
						devAxis = i;
						progress++;
					}
				}
				continue;
			}
			if (progress == 2 || progress == 3) { // Input setting
				if (c == 0x0D) { // Return
					if (progress == 2) { // save min
						minVal = controls.WheelDI.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(devAxis - 1), devGUID);
					}
					if (progress == 3) { // save max
						maxVal = controls.WheelDI.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(devAxis - 1), devGUID);
					}
					progress++;
				}
				continue;
			}
		}

		switch (progress) {
			case 0: { // Device selection
				setCursorPosition(0, 0);
				int devNumber = 0;
				for (auto guid : controls.WheelDI.GetGuids()) {
					wDevName = controls.WheelDI.FindEntryFromGUID(guid)->diDeviceInstance.tszInstanceName;
					std::cout << devNumber << ": " << std::string(wDevName.begin(), wDevName.end()) << "\n";
					devNumber++;
				}

				setCursorPosition(0, csbi.srWindow.Bottom - 2);
				printf("Select device for %s (0 to %d)", gameAxis.c_str(), controls.WheelDI.nEntry - 1);
			}
			break;
			case 1: { // Axis selection
				setCursorPosition(0, 0);
				std::cout << "Axes: ";
				int pRow = 1;
				for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
					setCursorPosition(0, pRow);
					std::cout << "                                ";
					setCursorPosition(0, pRow);
					std::cout << std::to_string(i+1) << ". " << controls.WheelDI.DIAxisHelper[i] << ": " << controls.WheelDI.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), devGUID) << "\n";
					pRow++;
				}

				setCursorPosition(0, csbi.srWindow.Bottom - 2);
				printf("Using device %d: %s", devEntry, std::string(wDevName.begin(), wDevName.end()).c_str());
				setCursorPosition(0, csbi.srWindow.Bottom - 3);
				printf("Select axis for %s", gameAxis.c_str());
			}
			break;
			case 2: { // Min (no input) register
				setCursorPosition(0, 0);
				for (int i = 0; i < csbi.srWindow.Right; i++) {
					printf(" ");
				}
				setCursorPosition(0, 0);
				printf("Min position: %d", controls.WheelDI.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(devAxis - 1), devGUID));

				setCursorPosition(0, csbi.srWindow.Bottom - 4);
				printf("Using axis %s", devAxisH[devAxis - 1].c_str());
				setCursorPosition(0, csbi.srWindow.Bottom - 2);
				printf("Using device %d: %s", devEntry, std::string(wDevName.begin(), wDevName.end()).c_str());
				setCursorPosition(0, csbi.srWindow.Bottom - 3);
				printf("Min position for %s, Enter to confirm", gameAxis.c_str());
			}
			break;
			case 3: { // Max (full stomp) register
				setCursorPosition(0, 0);
				for (int i = 0; i < csbi.srWindow.Right; i++) {
					printf(" ");
				}
				setCursorPosition(0, 0);
				printf("Max position: %d", controls.WheelDI.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(devAxis - 1), devGUID));

				setCursorPosition(0, csbi.srWindow.Bottom - 4);
				printf("Using axis %s", devAxisH[devAxis - 1].c_str());
				setCursorPosition(0, csbi.srWindow.Bottom - 2);
				printf("Using device %d: %s", devEntry, std::string(wDevName.begin(), wDevName.end()).c_str());
				setCursorPosition(0, csbi.srWindow.Bottom - 3);
				printf("Max position for %s, Enter to confirm", gameAxis.c_str());
			}
			break;
			default: { // save all the shit
				int index = settings.SteeringAppendDevice(devGUID, std::string(wDevName.begin(), wDevName.end()).c_str());
				settings.SteeringSave(confTag, index, devAxisH[devAxis - 1], minVal, maxVal);
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
		printf("Configuring %s", gameAxis.c_str());
		setCursorPosition(0, csbi.srWindow.Bottom);
		std::cout << "ESC: Cancel config";
		std::cout.flush();
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}
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
					configAxis(c);
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
				if (controls.WheelDI.IsButtonPressed(d, guid)) {
					std::cout << d << " ";
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

		setCursorPosition(0, csbi.srWindow.Bottom-2);
		std::cout << "Assign axes:";
		setCursorPosition(0, csbi.srWindow.Bottom-1);
		std::cout << "w: throttle - s: brake - a or d: steer - c: clutch - h: handbrake";
		setCursorPosition(0, csbi.srWindow.Bottom);
		std::cout << "ESC: Exit - Space: Reload";
		std::cout.flush();
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
		//cls();
	}
}	
