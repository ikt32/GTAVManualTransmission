#include "WheelDirectInput.hpp"
#include "../Util/TimeHelper.hpp"
#include "../Util/Logger.hpp"
#include <sstream>

#include <winerror.h>
#include <chrono>
#include <vector>

// TODO Look into crashes

WheelDirectInput::WheelDirectInput() : pCFEffect{nullptr},
                                       pFREffect{nullptr} {
	// Just set up to ensure djs can always be used.
	if (FAILED(DirectInput8Create(GetModuleHandle(nullptr),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		reinterpret_cast<void**>(&lpDi),
		nullptr))) {
		return;
	}
	djs.enumerate(lpDi);
}

WheelDirectInput::~WheelDirectInput() { }

bool WheelDirectInput::InitWheel() {
	logger.Write("WHEEL: Initializing steering wheel"); 
	if (lpDi == nullptr) {
		if (FAILED(DirectInput8Create(GetModuleHandle(nullptr),
			DIRECTINPUT_VERSION,
			IID_IDirectInput8,
			reinterpret_cast<void**>(&lpDi),
			nullptr))) {
			logger.Write("WHEEL: DirectInput create failed again");
			return false;
		}
	}

	djs.enumerate(lpDi);
	logger.Write("WHEEL: Found " + std::to_string(djs.getEntryCount()) + " device(s)");

	if (djs.getEntryCount() < 1) {
		logger.Write("WHEEL: No wheel detected");
		return false;
	}

	foundGuids.clear();
	for (int i = 0; i < djs.getEntryCount(); i++) {
		auto device = djs.getEntry(i);
		std::wstring wDevName = device->diDeviceInstance.tszInstanceName;
		logger.Write("WHEEL: Device: " + std::string(wDevName.begin(), wDevName.end()));

		GUID guid = device->diDeviceInstance.guidInstance;
		wchar_t szGuidW[40] = { 0 };
		StringFromGUID2(guid, szGuidW, 40);
		std::wstring wGuid = szGuidW;//std::wstring(szGuidW);
		logger.Write("WHEEL: GUID:   " + std::string(wGuid.begin(), wGuid.end()));
		foundGuids.push_back(guid);
	}

	logger.Write("WHEEL: Init steering wheel success");
	return true;
}

bool WheelDirectInput::InitFFB(GUID guid, DIAxis ffAxis) {
	logger.Write("WHEEL: Init FFB device");
	auto e = FindEntryFromGUID(guid);
	
	if (!e) {
		logger.Write("WHEEL: FFB device not found");
		return false;
	}

	e->diDevice->Unacquire();
	HRESULT hr;
	if (FAILED(hr = e->diDevice->SetCooperativeLevel(GetForegroundWindow(),
													 DISCL_EXCLUSIVE | 
													 DISCL_FOREGROUND))) {
		std::string hrStr;
		formatError(hr, hrStr);
		logger.Write("WHEEL: Acquire FFB device error");
		logger.Write("WHEEL: HRESULT = " + hrStr);
		std::stringstream ss;
		ss << std::hex << hr;
		logger.Write("WHEEL: ERRCODE = " + ss.str());
		ss.str(std::string());
		ss << std::hex << GetForegroundWindow();
		logger.Write("WHEEL: HWND =    " + ss.str());
		return false;
	}
	logger.Write("WHEEL: Init FFB effect on axis " + DIAxisHelper[ffAxis]);
	if (!createConstantForceEffect(e, ffAxis)) {
		logger.Write("WHEEL: Init FFB effect failed");
	} else {
		logger.Write("WHEEL: Init FFB success");
		hasForceFeedback = true;
	}
	return true;
}

void WheelDirectInput::UpdateCenterSteering(GUID guid, DIAxis steerAxis) {
	djs.update(); // TODO: Figure out why this needs to be called TWICE
	djs.update(); // Otherwise the wheel keeps turning/value is not updated?
	prevTime = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns
	prevPosition = GetAxisValue(steerAxis, guid);
}																																						

/*
 * Return NULL when device isn't found
 */
const DiJoyStick::Entry *WheelDirectInput::FindEntryFromGUID(GUID guid) {
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

void WheelDirectInput::Update() {
	djs.update();
}

bool WheelDirectInput::IsConnected(GUID device) {
	auto e = FindEntryFromGUID(device);
	if (!e) {
		return false;
	}
	return true;
}

// Mental note: buttonType in these args means physical button number
// like how they are in DirectInput.
// If it matches the cardinal stuff the button is a POV hat thing

bool WheelDirectInput::IsButtonPressed(int buttonType, GUID device) {
	auto e = FindEntryFromGUID(device);

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

bool WheelDirectInput::createConstantForceEffect(const DiJoyStick::Entry *e, DIAxis ffAxis) {
	if (!e)
		return false;

	HRESULT hr;

	DWORD axis;
	if		(ffAxis == lX)	{ axis = DIJOFS_X; }
	else if (ffAxis == lY)	{ axis = DIJOFS_Y; }
	else if (ffAxis == lZ)	{ axis = DIJOFS_Z; }
	else if (ffAxis == lRx) { axis = DIJOFS_RX; }
	else if (ffAxis == lRy) { axis = DIJOFS_RY; }
	else if (ffAxis == lRz) { axis = DIJOFS_RZ; }
	else { return false; }

	// I'm focusing on steering wheels, so you get one axis.
	DWORD rgdwAxes[1] = { axis };
	LONG rglDirection[1] = { 0 };
	DICONSTANTFORCE cf = { 0 };

	/* 
	 * I don't know why we need to init DIEFFECT if we're using a new DIEFFECT
	 * afterwards anyway.
	 */
	DIEFFECT diEffect;
	ZeroMemory(&diEffect, sizeof(diEffect));
	diEffect.dwSize = sizeof(DIEFFECT);
	diEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
	diEffect.dwDuration = INFINITE;
	diEffect.dwSamplePeriod = 0;
	diEffect.dwGain = DI_FFNOMINALMAX;
	diEffect.dwTriggerButton = DIEB_NOTRIGGER;
	diEffect.dwTriggerRepeatInterval = 0;
	diEffect.cAxes = 1;
	diEffect.rgdwAxes = rgdwAxes;
	diEffect.rglDirection = rglDirection;
	diEffect.lpEnvelope = nullptr;
	diEffect.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
	diEffect.lpvTypeSpecificParams = &cf;
	diEffect.dwStartDelay = 0;

	hr = e->diDevice->CreateEffect(GUID_ConstantForce, &diEffect, &pCFEffect, nullptr);
	
	if (FAILED(hr) || !pCFEffect) {
		return false;
	}

	return true;
}

void WheelDirectInput::SetConstantForce(GUID device, int force) {
	auto e = FindEntryFromGUID(device);
	if (!e || !pCFEffect || !hasForceFeedback)
		return;

	LONG rglDirection[1] = {0};

	DICONSTANTFORCE cf;
	cf.lMagnitude = force;

	DIEFFECT diEffect;
	ZeroMemory(&diEffect, sizeof(diEffect));
	diEffect.dwSize = sizeof(DIEFFECT);
	diEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
	diEffect.cAxes = 1;
	diEffect.rglDirection = rglDirection;
	diEffect.lpEnvelope = nullptr;
	diEffect.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
	diEffect.lpvTypeSpecificParams = &cf;
	diEffect.dwStartDelay = 0;
	
	// This should also automagically play.
	pCFEffect->SetParameters(&diEffect, 
		DIEP_DIRECTION |  DIEP_TYPESPECIFICPARAMS | DIEP_START);

	// When window focus is lost this should reacquire stuff I guess
	/*if (FAILED(e->diDevice->Poll())) {
		HRESULT hr = e->diDevice->Acquire();
		while (hr == DIERR_INPUTLOST) {
			hr = e->diDevice->Acquire();
		}
		pCFEffect->Start(1, 0);
	}*/
	/*e->diDevice->Acquire();
	pCFEffect->Start(1, 0);*/
}

WheelDirectInput::DIAxis WheelDirectInput::StringToAxis(std::string &axisString) {
	for (int i = 0; i < SIZEOF_DIAxis; i++) {
		if (axisString == DIAxisHelper[i]) {
			return static_cast<DIAxis>(i);
		}
	}
	return UNKNOWN_AXIS;
}

// -1 means device not accessible
int WheelDirectInput::GetAxisValue(DIAxis axis, GUID device) {
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

// Only confirmed to work on my own G27
void WheelDirectInput::PlayLedsDInput(GUID guid, const FLOAT currentRPM, const FLOAT rpmFirstLedTurnsOn, const FLOAT rpmRedLine) {
	auto e = FindEntryFromGUID(guid);

	if (!e)
		return;

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

	//HRESULT hr;
	//hr = e->diDevice->Escape(&data_);
	e->diDevice->Escape(&data_);
}

void WheelDirectInput::formatError(HRESULT hr, std::string &hrStr) {
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
}