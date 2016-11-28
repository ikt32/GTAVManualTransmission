// DIUtil.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "../Gears/Input/WheelDirectInput.hpp"


int main()
{
	Logger logger(LOGFILE);
	logger.Clear();
	logger.Write("Something to test DirectInput devices and stuff");

	WheelDirectInput di;

	di.InitWheel("X");

	/*GUID guid = { MAKELONG(0x045E, 0x028E), 0x0000, 0x0000, { 0x00, 0x00, 0x50, 0x49, 0x44, 0x56, 0x49, 0x44 } };
	LPOLESTR* bstrGuid;
	StringFromCLSID(guid, bstrGuid);
	std::wstring wGuid = std::wstring(*bstrGuid);
	logger.Write("360 :   " + std::string(wGuid.begin(), wGuid.end()));*/

	while (true) {
		std::cout << "Test\n";
		// Do stuff

		if (std::cin.get())
			break;
	}
    return 0;
}

