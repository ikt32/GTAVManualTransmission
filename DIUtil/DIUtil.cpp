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


	/*GUID guid = { MAKELONG(0x045E, 0x028E), 0x0000, 0x0000, { 0x00, 0x00, 0x50, 0x49, 0x44, 0x56, 0x49, 0x44 } };
	LPOLESTR* bstrGuid;
	StringFromCLSID(guid, bstrGuid);
	std::wstring wGuid = std::wstring(*bstrGuid);
	logger.Write("360 :   " + std::string(wGuid.begin(), wGuid.end()));*/

	//while (true) {
	//	std::cout << "Test\n";
	//	// Do stuff

	//	if (std::cin.get())
	//		break;
	//}
	std::cin.get();
    return 0;
}

