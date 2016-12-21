// DIUtil.cpp : Defines the entry point for the console application.
//

#include <conio.h>  
#include <stdio.h>  
#include <iostream>
#include "../Gears/Input/WheelDirectInput.hpp"
#include "../Gears/ScriptSettings.hpp"
#include "../Gears/Input/ScriptControls.hpp"
#include <thread>

/* Standard error macro for reporting API errors */
#define PERR(bSuccess, api){if(!(bSuccess)) printf("%s:Error %d from %s on line %d\n", __FILE__, GetLastError(), api, __LINE__);}

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
		// TODO: Handle failure!
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

// TODO: This is pulled straight from Gears/script.cpp and should be refactored lmao
// TODO: Move this shit
bool operator < (const GUID &guid1, const GUID &guid2) {
	if (guid1.Data1 != guid2.Data1) {
		return guid1.Data1 < guid2.Data1;
	}
	if (guid1.Data2 != guid2.Data2) {
		return guid1.Data2 < guid2.Data2;
	}
	if (guid1.Data3 != guid2.Data3) {
		return guid1.Data3 < guid2.Data3;
	}
	for (int i = 0; i<8; i++) {
		if (guid1.Data4[i] != guid2.Data4[i]) {
			return guid1.Data4[i] < guid2.Data4[i];
		}
	}
	return false;
}

std::string GUID2String(GUID g) {
	wchar_t szGuidW[40] = { 0 };
	StringFromGUID2(g, szGuidW, 40);
	std::wstring wGuid = szGuidW;
	return std::string(wGuid.begin(), wGuid.end());
}

void checkGUIDs(const std::vector<_GUID> & guids) {
	auto foundGuids = controls.WheelDI.GetGuids();
	auto reggdGuids = settings.GetGuids();
	// We're only checking for devices that should be used but aren't found
	std::sort(foundGuids.begin(), foundGuids.end());
	std::sort(reggdGuids.begin(), reggdGuids.end());
	std::vector<GUID> missingReg;
	std::vector<GUID> missingFnd;

	// Mental note since I'm dumb:
	// The difference of two sets is formed by the elements that are present in
	// the first set, but not in the second one.

	// Registered but not enumerated
	std::set_difference(
		reggdGuids.begin(), reggdGuids.end(),
		foundGuids.begin(), foundGuids.end(), std::back_inserter(missingReg));

	if (missingReg.size() > 0) {
		logger.Write("Registered but not available: ");
		for (auto g : missingReg) {
			logger.Write(std::string("    ") + GUID2String(g));
		}
	}

	// Enumerated but not registered
	std::set_difference(
		foundGuids.begin(), foundGuids.end(),
		reggdGuids.begin(), reggdGuids.end(), std::back_inserter(missingFnd));

	if (missingFnd.size() > 0) {
		logger.Write("Enumerated but not registered: (available for use in settings_wheel.ini)");
		for (auto g : missingFnd) {
			logger.Write(std::string("    ") + GUID2String(g));
		}
	}
}

int main()
{
	unsigned long long i = 0;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO     cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = false;
	SetConsoleCursorInfo(hConsole, &cursorInfo);

	RECT r;
	HWND consoleWindow = GetConsoleWindow();
	GetWindowRect(consoleWindow, &r);

	logger.Clear();
	logger.Write("Manual Transmission v4.2.0 - DirectInput utility");
	settings.Read(&controls);

	GUID steerGuid = controls.WheelAxesGUIDs[static_cast<int>(ScriptControls::WheelAxisType::Steer)];

	controls.InitWheel();
	checkGUIDs(settings.reggdGuids);

	int activeGuids = static_cast<int>(controls.WheelDI.GetGuids().size());
	int totalWidth = static_cast<int>((activeGuids + 0.5) * 32);
	if (totalWidth < 80) {
		totalWidth = 80;
	}
	std::string modeStr = "MODE " + std::to_string(totalWidth) + ",32";
	system(modeStr.c_str());
	
	while (true)
	{
		if (_kbhit()) {
			char c = _getch();
			if (c == 0x1B) {
				break;
			}
		}
		controls.GetLastInputDevice(ScriptControls::InputDevices::Wheel);
		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false);

		float steerMult;
		steerMult = settings.SteerAngleMax / settings.SteerAngleCar;
		float effSteer = steerMult * 2.0f * (controls.SteerVal - 0.5f);
		
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(hConsole, &csbi);

		int guidIt = 0;
		int pRowMax = 0;
		for (auto guid : controls.WheelDI.GetGuids()) {
			int pRow = 0;

			setCursorPosition(32 * guidIt, pRow);
			pRow++;
			std::wstring wDevName = controls.WheelDI.findEntryFromGUID(guid)->diDeviceInstance.tszInstanceName;
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


		controls.WheelDI.PlayLedsDInput(steerGuid, controls.ThrottleVal, 0.5f, 0.95f);

		auto totalForce = static_cast<int>(controls.ThrottleVal * 20000.0f * 2.0f * (controls.SteerVal - 0.5f));
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
		controls.WheelDI.SetConstantForce(steerGuid, totalForce);

		setCursorPosition(0 , csbi.srWindow.Bottom);
		std::cout << "Press ESC to exit.";
		std::cout.flush();
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
		//cls();
	}
	return 0;
}	
