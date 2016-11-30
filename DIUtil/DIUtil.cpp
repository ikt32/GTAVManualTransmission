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

	di.InitWheel(di.StringToAxis(std::string("lX")));

	//di.InitFFB(GUID_NULL , di.StringToAxis(std::string("lX")));

	// the fuck, Microsoft?
	LPWSTR clsidStr = LPWSTR("{F69653F0-19B9-11E6-8002-444553540000}");

	GUID guid;
	HRESULT hr = CLSIDFromString(clsidStr, (LPCLSID)&guid);
	if (hr != NOERROR) {
		std::string errStr;
		switch (hr) {
			case CO_E_CLASSSTRING:
				errStr = "CO_E_CLASSSTRING";
				break;
			case REGDB_E_CLASSNOTREG	:
				errStr = "REGDB_E_CLASSNOTREG";
				break;
			case REGDB_E_READREGDB:
				errStr = "REGDB_E_READREGDB";
				break;
			default:
				errStr = "fuck if i know";
				break;
		}
		logger.Write("CLSIDFromString " + errStr);
	}
	di.InitFFB(guid, di.StringToAxis(std::string("lX")));


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

