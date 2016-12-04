#include "WheelDirectInput.hpp"
#include "../Util/TimeHelper.hpp"
#include "../Util/Logger.hpp"
#include <sstream>

#include <winerror.h>
#include <chrono>
#include <vector>

WheelDirectInput::WheelDirectInput() : logger(LOGFILE),
                                       pCFEffect{nullptr},
                                       pFREffect{nullptr} { }

WheelDirectInput::WheelDirectInput(Logger logAlt) : logger(logAlt),
													pCFEffect{ nullptr },
													pFREffect{ nullptr } { }


bool WheelDirectInput::InitWheel() {
	if (SUCCEEDED(DirectInput8Create(GetModuleHandle(nullptr),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		reinterpret_cast<void**>(&lpDi),
		nullptr))) {
		logger.Write("Initializing steering wheel");

		foundGuids.clear();
		djs.enumerate(lpDi);
		nEntry = djs.getEntryCount();
		logger.Write("Found " + std::to_string(nEntry) + " device(s)");

		for (int i = 0; i < nEntry; i++) {
			auto device = djs.getEntry(i);
			std::wstring wDevName = device->diDeviceInstance.tszInstanceName;
			logger.Write("Device: " + std::string(wDevName.begin(), wDevName.end()));

			GUID guid = device->diDeviceInstance.guidInstance;
			wchar_t szGuidW[40] = { 0 };
			StringFromGUID2(guid, szGuidW, 40);
			std::wstring wGuid = szGuidW;//std::wstring(szGuidW);
			logger.Write("GUID:   " + std::string(wGuid.begin(), wGuid.end()));
			foundGuids.push_back(guid);
		}

		djs.update();
		return true;
	}
	logger.Write("No wheel detected");
	return false;
}

/* 
 * if an empty GUID is given, just try the first device
 * Update - That was a horrible idea. Just return null and handle it whenever idk fuck this SHIT FUCK
 */
const DiJoyStick::Entry *WheelDirectInput::findEntryFromGUID(GUID guid) {
	//int nEntry = djs.getEntryCount();

	if (nEntry > 0) {
		if (guid == GUID_NULL) {
			return nullptr;
		}

		for (int i = 0; i < nEntry; i++) {
			auto tempEntry = djs.getEntry(i);
			if (guid == tempEntry->diDeviceInstance.guidInstance) {
				return tempEntry;
			}
		}
	}
	return nullptr;
}

bool WheelDirectInput::InitFFB(GUID guid, WheelDirectInput::DIAxis ffAxis) {
	logger.Write("Initializing force feedback device");
	auto e = findEntryFromGUID(guid);
	
	if (!e) {
		logger.Write("Force feedback device not found");
		return false;
	}

	e->diDevice->Unacquire();
	HRESULT hr;
	if (FAILED(hr = e->diDevice->SetCooperativeLevel(
			GetForegroundWindow(),
		DISCL_EXCLUSIVE | DISCL_FOREGROUND))) {
		std::string hrStr;
		switch (hr) {
			case DI_OK: hrStr = "DI_OK";
				break;
			case DIERR_INVALIDPARAM: hrStr = "DIERR_INVALIDPARAM";
				break;
			case DIERR_NOTINITIALIZED: hrStr = "DIERR_NOTINITIALIZED";
				break;
			case DIERR_ALREADYINITIALIZED: hrStr = "DIERR_ALREADYINITIALIZED";
				break;
			case DIERR_INPUTLOST: hrStr = "DIERR_INPUTLOST";
				break;
			case DIERR_ACQUIRED: hrStr = "DIERR_ACQUIRED";
				break;
			case DIERR_NOTACQUIRED: hrStr = "DIERR_NOTACQUIRED";
				break;
			case E_HANDLE: hrStr = "E_HANDLE";
				break;
			default: hrStr = "UNKNOWN";
				break;
		}
		logger.Write("HRESULT = " + hrStr);
		std::stringstream ss;
		ss << std::hex << hr;
		logger.Write("Error: " + ss.str());
		ss.str(std::string());
		ss << std::hex << GetForegroundWindow();
		logger.Write("HWND: " + ss.str());
		return false;
	}
	logger.Write("Initializing force feedback effect");
	if (!CreateConstantForceEffect(e, ffAxis)) {
		logger.Write("That steering axis doesn't support force feedback");
		NoFeedback = true;
		return false;
	}
	logger.Write("Initializing force feedback success");
	UpdateState(); // I don't understand
	UpdateState(); // Why do I need to call this twice?
	prevTime = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns
	logger.Write("Initializing wheel success");
	return true;
}

void WheelDirectInput::UpdateState() {
	djs.update();
}

bool WheelDirectInput::IsConnected(GUID device) {
	auto e = findEntryFromGUID(device);
	if (!e) {
		return false;
	}
	return true;
}

// Mental note: buttonType in these args means physical button number
// like how they are in DirectInput.
// If it matches the cardinal stuff the button is a POV hat thing

bool WheelDirectInput::IsButtonPressed(int buttonType, GUID device) {
	auto e = findEntryFromGUID(device);

	if (!e) {
		/*wchar_t szGuidW[40] = { 0 };
		StringFromGUID2(device, szGuidW, 40);
		std::wstring wGuid = szGuidW;
		log.Write("DBG: Button " + std::to_string(buttonType) + " with GUID " + std::string(wGuid.begin(), wGuid.end()));*/
		return false;
	}

	if (buttonType > 127) {
		switch (buttonType) {
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
				if (buttonType == e->joystate.rgdwPOV[0])
					return true;
			default:
				return false;
		}
	}
	if (e->joystate.rgbButtons[buttonType])
		return true;
	return false;
}

bool WheelDirectInput::IsButtonJustPressed(int buttonType, GUID device) {
	if (buttonType > 127) { // POV
		povButtonCurr[buttonType] = IsButtonPressed(buttonType,device);

		// raising edge
		if (povButtonCurr[buttonType] && !povButtonPrev[buttonType]) {
			return true;
		}
		return false;
	}
	rgbButtonCurr[buttonType] = IsButtonPressed(buttonType,device);

	// raising edge
	if (rgbButtonCurr[buttonType] && !rgbButtonPrev[buttonType]) {
		return true;
	}
	return false;
}

bool WheelDirectInput::IsButtonJustReleased(int buttonType, GUID device) {
	if (buttonType > 127) { // POV
		povButtonCurr[buttonType] = IsButtonPressed(buttonType,device);

		// falling edge
		if (!povButtonCurr[buttonType] && povButtonPrev[buttonType]) {
			return true;
		}
		return false;
	}
	rgbButtonCurr[buttonType] = IsButtonPressed(buttonType,device);

	// falling edge
	if (!rgbButtonCurr[buttonType] && rgbButtonPrev[buttonType]) {
		return true;
	}
	return false;
}

bool WheelDirectInput::WasButtonHeldForMs(int buttonType, GUID device, int millis) {
	if (buttonType > 127) { // POV
		if (IsButtonJustPressed(buttonType,device)) {
			povPressTime[buttonType] = milliseconds_now();
		}
		if (IsButtonJustReleased(buttonType,device)) {
			povReleaseTime[buttonType] = milliseconds_now();
		}

		if ((povReleaseTime[buttonType] - povPressTime[buttonType]) >= millis) {
			povPressTime[buttonType] = 0;
			povReleaseTime[buttonType] = 0;
			return true;
		}
		return false;
	}
	if (IsButtonJustPressed(buttonType,device)) {
		rgbPressTime[buttonType] = milliseconds_now();
	}
	if (IsButtonJustReleased(buttonType,device)) {
		rgbReleaseTime[buttonType] = milliseconds_now();
	}

	if ((rgbReleaseTime[buttonType] - rgbPressTime[buttonType]) >= millis) {
		rgbPressTime[buttonType] = 0;
		rgbReleaseTime[buttonType] = 0;
		return true;
	}
	return false;
}

void WheelDirectInput::UpdateButtonChangeStates() {
	for (int i = 0; i < MAX_RGBBUTTONS; i++) {
		rgbButtonPrev[i] = rgbButtonCurr[i];
	}
	for (int i = 0; i < SIZEOF_POV; i++) {
		povButtonPrev[i] = povButtonCurr[i];
	}
}

bool WheelDirectInput::CreateConstantForceEffect(const DiJoyStick::Entry *e, WheelDirectInput::DIAxis ffAxis) {
	if (!e || NoFeedback)
		return false;

	DWORD axis;
	if (ffAxis == lX) {
		axis = DIJOFS_X;
	}
	else if (ffAxis == lY) {
		axis = DIJOFS_Y;
	}
	else if (ffAxis == lZ) {
		axis = DIJOFS_Z;
	}
	else {
		return false;
	}

	DWORD rgdwAxes[1] = { axis };
	LONG rglDirection[1] = {0};
	DICONSTANTFORCE cf = {0};

	DIEFFECT eff;
	ZeroMemory(&eff, sizeof(eff));
	eff.dwSize = sizeof(DIEFFECT);
	eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
	eff.dwDuration = INFINITE;
	eff.dwSamplePeriod = 0;
	eff.dwGain = DI_FFNOMINALMAX;
	eff.dwTriggerButton = DIEB_NOTRIGGER;
	eff.dwTriggerRepeatInterval = 0;
	eff.cAxes = 1;
	eff.rgdwAxes = rgdwAxes;
	eff.rglDirection = rglDirection;
	eff.lpEnvelope = nullptr;
	eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
	eff.lpvTypeSpecificParams = &cf;
	eff.dwStartDelay = 0;

	if (e) {
		e->diDevice->CreateEffect(
			 GUID_ConstantForce,
			 &eff,
			 &pCFEffect,
			 nullptr);
	}

	if (!pCFEffect) {
		return false;
	}

	return true;
}

HRESULT WheelDirectInput::SetConstantForce(int force) const {
	if (NoFeedback)
		return false;

	HRESULT hr;
	LONG rglDirection[1] = {0};


	DICONSTANTFORCE cf;
	cf.lMagnitude = force;

	DIEFFECT cfEffect;
	ZeroMemory(&cfEffect, sizeof(cfEffect));
	cfEffect.dwSize = sizeof(DIEFFECT);
	cfEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
	cfEffect.cAxes = 1;
	cfEffect.rglDirection = rglDirection;
	cfEffect.lpEnvelope = nullptr;
	cfEffect.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
	cfEffect.lpvTypeSpecificParams = &cf;
	cfEffect.dwStartDelay = 0;

	hr = pCFEffect->SetParameters(&cfEffect, DIEP_DIRECTION |
	                              DIEP_TYPESPECIFICPARAMS |
	                              DIEP_START);

	const DiJoyStick::Entry *e = djs.getEntry(0);
	if (e) {
		e->diDevice->Acquire();
		if (pCFEffect)
			pCFEffect->Start(1, 0);
	}
	else {
		return E_HANDLE;
	}

	return hr;
}

WheelDirectInput::DIAxis WheelDirectInput::StringToAxis(std::string &axisString) {
	for (int i = 0; i < SIZEOF_DIAxis; i++) {
		if (axisString == DIAxisHelper[i]) {
			return static_cast<DIAxis>(i);
		}
	}
	return UNKNOWN_AXIS;
}


int WheelDirectInput::GetAxisValue(DIAxis axis, GUID device) {
	auto e = findEntryFromGUID(device);
	if (!IsConnected(device) || e == nullptr)
		return 0;
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
float WheelDirectInput::GetAxisSpeed(DIAxis axis, GUID device) {
	auto time = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns
	auto position = GetAxisValue(axis , device);
	auto result = (position - prevPosition) / ((time - prevTime) / 1e9f);

	prevTime = time;
	prevPosition = position;

	samples[averageIndex] = result;
	averageIndex = (averageIndex + 1) % (SAMPLES - 1);

	//return result;
	auto sum = 0.0f;
	for (auto i = 0; i < SAMPLES; i++) {
		sum += samples[i];
	}
	return sum / SAMPLES;
}

std::vector<GUID> WheelDirectInput::GetGuids() {
	return foundGuids;
}

void WheelDirectInput::PlayLedsDInput(GUID guid, CONST FLOAT currentRPM, CONST FLOAT rpmFirstLedTurnsOn, CONST FLOAT rpmRedLine)
{
	auto e = findEntryFromGUID(guid);

	if (!e)
	{
		return;
	}

	CONST DWORD ESCAPE_COMMAND_LEDS = 0;
	CONST DWORD LEDS_VERSION_NUMBER = 0x00000001;

	struct LedsRpmData
	{
		FLOAT currentRPM;
		FLOAT rpmFirstLedTurnsOn;
		FLOAT rpmRedLine;
	};

	struct WheelData
	{
		DWORD size;
		DWORD versionNbr;
		LedsRpmData rpmData;
	};

	
	WheelData wheelData_;
	ZeroMemory(&wheelData_, sizeof(wheelData_));

	wheelData_.size = sizeof(WheelData);
	wheelData_.versionNbr = LEDS_VERSION_NUMBER;
	wheelData_.rpmData.currentRPM = currentRPM;
	wheelData_.rpmData.rpmFirstLedTurnsOn = rpmFirstLedTurnsOn;
	wheelData_.rpmData.rpmRedLine = rpmRedLine;

	DIEFFESCAPE data_;
	ZeroMemory(&data_, sizeof(data_));

	data_.dwSize = sizeof(DIEFFESCAPE);
	data_.dwCommand = ESCAPE_COMMAND_LEDS;
	data_.lpvInBuffer = &wheelData_;
	data_.cbInBuffer = sizeof(wheelData_);

	HRESULT hr;
	hr = e->diDevice->Escape(&data_);
}