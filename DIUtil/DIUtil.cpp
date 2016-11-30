// DIUtil.cpp : Defines the entry point for the console application.
//

#include <conio.h>  
#include <stdio.h>  
#include <iostream>
#include "../Gears/Input/WheelDirectInput.hpp"

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

int main()
{
	Logger logger(LOGFILE);
	logger.Clear();
	logger.Write("Something to test DirectInput devices and stuff");

	WheelDirectInput di;

	std::string axis = "lX";
	di.InitWheel(di.StringToAxis(axis));

	std::string guidStr = "{F69653F0-19B9-11E6-8002-444553540000}";
	std::wstring clsidStr;
	clsidStr.assign(guidStr.begin(), guidStr.end());

	GUID guid;
	HRESULT hr = CLSIDFromString(clsidStr.c_str(), &guid);
	if (hr != NOERROR) {
		std::string errStr;
		switch (hr) {
			case CO_E_CLASSSTRING:
				errStr = "The class string was improperly formatted.";
				break;
			case REGDB_E_CLASSNOTREG	:
				errStr = "The CLSID corresponding to the class string was not found in the registry.";
				break;
			case REGDB_E_READREGDB:
				errStr = "The registry could not be opened for reading.";
				break;
			default:
				errStr = "Something went terribly wrong.";
				break;
		}
		logger.Write("CLSIDFromString error: " + errStr);
	}
	di.InitFFB(guid, di.StringToAxis(axis));

	unsigned long long i = 0;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO     cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = false; 
	SetConsoleCursorInfo(hConsole, &cursorInfo);

	while (!_kbhit())
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(hConsole, &csbi);
		setCursorPosition(0, 0);
		di.UpdateState();
		di.UpdateButtonChangeStates();
		std::cout << "Axes:\n";
		for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis-1; i++) {
			std::cout << "    " << di.DIAxisHelper[i] << ": " << di.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), 0) << "\n";
		}
		
		std::cout << "Buttons: ";
		for (int i = 0; i < 255; i++) {
			if (di.IsButtonPressed(i))
				std::cout << i << " ";
		}
		std::cout << "\n";

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
		for (auto d : directions) {
			if (di.IsButtonPressed(d))
				std::cout << d << " ";
		}


		setCursorPosition(0 , csbi.srWindow.Bottom-1);
		std::cout << "Hit any key to exit";
		std::cout.flush();
		Sleep(4);
		cls();
	}
	return 0;
}
