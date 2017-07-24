#include "GenDInput.h"

#include "../Util/TimeHelper.hpp"
#include "../Util/Logger.hpp"
#include <sstream>

#include <winerror.h>
#include <chrono>
#include <vector>

GenDInput::GenDInput() { }

GenDInput::~GenDInput() { }

bool GenDInput::PreInit() {
	// Just set up to ensure djs can always be used.
	if (FAILED(DirectInput8Create(GetModuleHandle(nullptr),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		reinterpret_cast<void**>(&lpDi),
		nullptr))) {
		return false;
	}
	djs.enumerate(lpDi);
	return true;
}

bool GenDInput::InitDevice() {
	logger.Write("DINPUT: Initializing device");
	if (lpDi == nullptr) {
		if (FAILED(DirectInput8Create(GetModuleHandle(nullptr),
			DIRECTINPUT_VERSION,
			IID_IDirectInput8,
			reinterpret_cast<void**>(&lpDi),
			nullptr))) {
			logger.Write("DINPUT: DirectInput create failed again");
			return false;
		}
	}

	djs.enumerate(lpDi);
	logger.Write("DINPUT: Found " + std::to_string(djs.getEntryCount()) + " device(s)");

	if (djs.getEntryCount() < 1) {
		logger.Write("DINPUT: No device detected");
		return false;
	}

	foundGuids.clear();
	for (int i = 0; i < djs.getEntryCount(); i++) {
		auto device = djs.getEntry(i);
		std::wstring wDevName = device->diDeviceInstance.tszInstanceName;
		logger.Write("DINPUT: Device: " + std::string(wDevName.begin(), wDevName.end()));

		GUID guid = device->diDeviceInstance.guidInstance;
		wchar_t szGuidW[40] = { 0 };
		StringFromGUID2(guid, szGuidW, 40);
		std::wstring wGuid = szGuidW;//std::wstring(szGuidW);
		logger.Write("DINPUT: GUID:   " + std::string(wGuid.begin(), wGuid.end()));
		foundGuids.push_back(guid);
	}

	logger.Write("DINPUT: Init device success");
	return true;
}

/*
* Return NULL when device isn't found
*/
const DiJoyStick::Entry *GenDInput::FindEntryFromGUID(GUID guid) {
	if (djs.getEntryCount() > 0) {
		if (guid == GUID_NULL) {
			return nullptr;
		}

		for (int i = 0; i < djs.getEntryCount(); i++) {
			auto tempEntry = djs.getEntry(i);
			if (guid == tempEntry->diDeviceInstance.guidInstance) {
				return tempEntry;
			}
		}
	}
	return nullptr;
}

void GenDInput::Update() {
	djs.update();
}

bool GenDInput::IsConnected(GUID device) {
	auto e = FindEntryFromGUID(device);
	if (!e) {
		return false;
	}
	return true;
}

bool GenDInput::IsButtonPressed(GUID device, int button) {
	auto e = FindEntryFromGUID(device);

	if (!e) {
		return false;
	}

	if (button > 127) {
		switch (button) {
			case N:
				if (e->joystate.rgdwPOV[0] == 0) {
					return true;
				}
			case NE:
			case E:
			case SE:
			case S:
			case SW:
			case W:
			case NW:
				if (button == e->joystate.rgdwPOV[0])
					return true;
			default:
				return false;
		}
	}
	if (e->joystate.rgbButtons[button])
		return true;
	return false;
}

bool GenDInput::IsButtonJustPressed(GUID device, int button) {
	if (button > 127) { // POV
		povButtonCurr[button] = IsButtonPressed(device, button);

		// raising edge
		if (povButtonCurr[button] && !povButtonPrev[button]) {
			return true;
		}
		return false;
	}
	rgbButtonCurr[button] = IsButtonPressed(device, button);

	// raising edge
	if (rgbButtonCurr[button] && !rgbButtonPrev[button]) {
		return true;
	}
	return false;
}

bool GenDInput::IsButtonJustReleased(GUID device, int button) {
	if (button > 127) { // POV
		povButtonCurr[button] = IsButtonPressed(device, button);

		// falling edge
		if (!povButtonCurr[button] && povButtonPrev[button]) {
			return true;
		}
		return false;
	}
	rgbButtonCurr[button] = IsButtonPressed(device, button);

	// falling edge
	if (!rgbButtonCurr[button] && rgbButtonPrev[button]) {
		return true;
	}
	return false;
}

bool GenDInput::WasButtonHeldForMs(GUID device, int button, int millis) {
	if (button > 127) { // POV
		if (IsButtonJustPressed(device, button)) {
			povPressTime[button] = milliseconds_now();
		}
		if (IsButtonJustReleased(device, button)) {
			povReleaseTime[button] = milliseconds_now();
		}

		if ((povReleaseTime[button] - povPressTime[button]) >= millis) {
			povPressTime[button] = 0;
			povReleaseTime[button] = 0;
			return true;
		}
		return false;
	}
	if (IsButtonJustPressed(device, button)) {
		rgbPressTime[button] = milliseconds_now();
	}
	if (IsButtonJustReleased(device, button)) {
		rgbReleaseTime[button] = milliseconds_now();
	}

	if ((rgbReleaseTime[button] - rgbPressTime[button]) >= millis) {
		rgbPressTime[button] = 0;
		rgbReleaseTime[button] = 0;
		return true;
	}
	return false;
}

void GenDInput::UpdateButtonChangeStates() {
	for (int i = 0; i < MAX_RGBBUTTONS; i++) {
		rgbButtonPrev[i] = rgbButtonCurr[i];
	}
	for (int i = 0; i < SIZEOF_POV; i++) {
		povButtonPrev[i] = povButtonCurr[i];
	}
}

GenDInput::DIAxis GenDInput::StringToAxis(std::string &axisString) {
	for (int i = 0; i < SIZEOF_DIAxis; i++) {
		if (axisString == DIAxisHelper[i]) {
			return static_cast<DIAxis>(i);
		}
	}
	return UNKNOWN_AXIS;
}

// -1 means device not accessible
int GenDInput::GetAxisValue(GUID device, DIAxis axis) {
	if (!IsConnected(device)) {
		return -1;
	}
	auto e = FindEntryFromGUID(device);
	if (!e)
		return -1;
	switch (axis) {
		case lX: return  e->joystate.lX;
		case lY: return  e->joystate.lY;
		case lZ: return  e->joystate.lZ;
		case lRx: return e->joystate.lRx;
		case lRy: return e->joystate.lRy;
		case lRz: return e->joystate.lRz;
		case rglSlider0: return e->joystate.rglSlider[0];
		case rglSlider1: return e->joystate.rglSlider[1];
		default: return 0;
	}
}

// Returns in units/s
// TODO: Re-write axis so it's a class or struct? We can put speed and stuff in there then.
float GenDInput::GetAxisSpeed(GUID device, DIAxis axis) {
	//auto time = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns
	//auto position = GetAxisValue(device, axis);
	//auto result = (position - prevPosition) / ((time - prevTime) / 1e9f);

	//prevTime = time;
	//prevPosition = position;

	//samples[averageIndex] = result;
	//averageIndex = (averageIndex + 1) % (AVGSAMPLES - 1);

	////return result;
	//auto sum = 0.0f;
	//for (auto i = 0; i < AVGSAMPLES; i++) {
	//	sum += samples[i];
	//}
	//return sum / AVGSAMPLES;
	return 0;
}

std::vector<GUID> GenDInput::GetGUIDs() {
	return foundGuids;
}
