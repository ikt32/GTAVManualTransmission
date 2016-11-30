// DIUtil.cpp : Defines the entry point for the console application.
//

#include <conio.h>  
#include <stdio.h>  
#include <iostream>
#include "../Gears/Input/WheelDirectInput.hpp"

/* Standard error macro for reporting API errors */
#define PERR(bSuccess, api){if(!(bSuccess)) printf("%s:Error %d from %s on line %d\n", __FILE__, GetLastError(), api, __LINE__);}

void cls(HANDLE hConsole)
{
	COORD coordScreen = { 0, 0 };    /* here's where we'll home the
									 cursor */
	BOOL bSuccess;
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
	DWORD dwConSize;                 /* number of character cells in
									 the current buffer */

									 /* get the number of character cells in the current buffer */

	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	PERR(bSuccess, "GetConsoleScreenBufferInfo");
	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

	/* fill the entire screen with blanks */

	bSuccess = FillConsoleOutputCharacter(hConsole, (TCHAR) ' ',
										  dwConSize, coordScreen, &cCharsWritten);
	PERR(bSuccess, "FillConsoleOutputCharacter");

	/* get the current text attribute */

	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	PERR(bSuccess, "ConsoleScreenBufferInfo");

	/* now set the buffer's attributes accordingly */

	bSuccess = FillConsoleOutputAttribute(hConsole, csbi.wAttributes,
										  dwConSize, coordScreen, &cCharsWritten);
	PERR(bSuccess, "FillConsoleOutputAttribute");

	/* put the cursor at (0, 0) */

	bSuccess = SetConsoleCursorPosition(hConsole, coordScreen);
	PERR(bSuccess, "SetConsoleCursorPosition");
	return;
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
		COORD coordScreen = { 0, 0 };
		GetConsoleScreenBufferInfo(hConsole, &csbi);
		SetConsoleCursorPosition(hConsole, coordScreen);
		di.UpdateState();
		di.UpdateButtonChangeStates();
		std::cout << "Axes:\n";
		for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis; i++) {
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


		SetConsoleCursorPosition(hConsole, { 0 , csbi.srWindow.Bottom });
		std::cout << "Hit any key to exit";
		std::cout.flush();
		Sleep(1);
		cls(hConsole);
	}
	std::cin.get();
    return 0;
}
