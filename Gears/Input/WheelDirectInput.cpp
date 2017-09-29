#include "WheelDirectInput.hpp"

#include <winerror.h>

#include <sstream>
#include <chrono>
#include <vector>

#include "../Util/TimeHelper.hpp"
#include "../Util/Logger.hpp"

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }

WheelDirectInput::WheelDirectInput() : pCFEffect{nullptr},
                                       pFREffect{nullptr} { }

WheelDirectInput::~WheelDirectInput() {
	for (int i = 0; i < djs.getEntryCount(); i++) {
		auto device = djs.getEntry(i);
		if (device)
			device->diDevice->Unacquire();
	}
	
	SAFE_RELEASE(pCFEffect);
	SAFE_RELEASE(pFREffect);
	SAFE_RELEASE(lpDi);
}

bool WheelDirectInput::PreInit() {
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

	rgbPressTime.clear();
	rgbReleaseTime.clear();
	rgbButtonCurr.clear();
	rgbButtonPrev.clear();
	povPressTime.clear();
	povReleaseTime.clear();
	povButtonCurr.clear();
	povButtonPrev.clear();

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

		rgbPressTime.insert(	std::pair<GUID, std::array<__int64, MAX_RGBBUTTONS>>(guid, {}));
		rgbReleaseTime.insert(	std::pair<GUID, std::array<__int64, MAX_RGBBUTTONS>>(guid, {}));
		rgbButtonCurr.insert(	std::pair<GUID, std::array<bool,	MAX_RGBBUTTONS>>(guid, {}));
		rgbButtonPrev.insert(	std::pair<GUID, std::array<bool,	MAX_RGBBUTTONS>>(guid, {}));
		povPressTime.insert(	std::pair<GUID, std::array<__int64, POVDIRECTIONS>>(guid, {}));
		povReleaseTime.insert(	std::pair<GUID, std::array<__int64, POVDIRECTIONS>>(guid, {}));
		povButtonCurr.insert(	std::pair<GUID, std::array<bool, POVDIRECTIONS>>(guid, {}));
		povButtonPrev.insert(	std::pair<GUID, std::array<bool, POVDIRECTIONS>>(guid, {}));
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
		hasForceFeedback[guid][ffAxis] = true;
	}
	return true;
}

void WheelDirectInput::UpdateCenterSteering(GUID guid, DIAxis steerAxis) {
	djs.update(); // TODO: Figure out why this needs to be called TWICE
	djs.update(); // Otherwise the wheel keeps turning/value is not updated?
	prevTime[guid][steerAxis] = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns
	prevPosition[guid][steerAxis] = GetAxisValue(steerAxis, guid);
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
	updateAxisSpeed();
}

bool WheelDirectInput::IsConnected(GUID device) {
	auto e = FindEntryFromGUID(device);
	if (!e) {
		return false;
	}
	return true;
}

bool WheelDirectInput::IsButtonPressed(int buttonType, GUID device) {
	auto e = FindEntryFromGUID(device);
	if (!e) {
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
		povButtonCurr[device][povDirectionToIndex(buttonType)] = IsButtonPressed(buttonType,device);

		// raising edge
		if (povButtonCurr[device][povDirectionToIndex(buttonType)] && !povButtonPrev[device][povDirectionToIndex(buttonType)]) {
			return true;
		}
		return false;
	}
	rgbButtonCurr[device][buttonType] = IsButtonPressed(buttonType,device);

	// raising edge
	if (rgbButtonCurr[device][buttonType] && !rgbButtonPrev[device][buttonType]) {
		return true;
	}
	return false;
}

bool WheelDirectInput::IsButtonJustReleased(int buttonType, GUID device) {
	if (buttonType > 127) { // POV
		povButtonCurr[device][povDirectionToIndex(buttonType)] = IsButtonPressed(buttonType,device);

		// falling edge
		if (!povButtonCurr[device][povDirectionToIndex(buttonType)] && povButtonPrev[device][povDirectionToIndex(buttonType)]) {
			return true;
		}
		return false;
	}
	rgbButtonCurr[device][buttonType] = IsButtonPressed(buttonType,device);

	// falling edge
	if (!rgbButtonCurr[device][buttonType] && rgbButtonPrev[device][buttonType]) {
		return true;
	}
	return false;
}

bool WheelDirectInput::WasButtonHeldForMs(int buttonType, GUID device, int millis) {
	if (buttonType > 127) { // POV
		if (IsButtonJustPressed(buttonType,device)) {
			povPressTime[device][povDirectionToIndex(buttonType)] = milliseconds_now();
		}
		if (IsButtonJustReleased(buttonType,device)) {
			povReleaseTime[device][povDirectionToIndex(buttonType)] = milliseconds_now();
		}

		if ((povReleaseTime[device][povDirectionToIndex(buttonType)] - povPressTime[device][povDirectionToIndex(buttonType)]) >= millis) {
			povPressTime[device][povDirectionToIndex(buttonType)] = 0;
			povReleaseTime[device][povDirectionToIndex(buttonType)] = 0;
			return true;
		}
		return false;
	}
	if (IsButtonJustPressed(buttonType,device)) {
		rgbPressTime[device][buttonType] = milliseconds_now();
	}
	if (IsButtonJustReleased(buttonType,device)) {
		rgbReleaseTime[device][buttonType] = milliseconds_now();
	}

	if ((rgbReleaseTime[device][buttonType] - rgbPressTime[device][buttonType]) >= millis) {
		rgbPressTime[device][buttonType] = 0;
		rgbReleaseTime[device][buttonType] = 0;
		return true;
	}
	return false;
}

void WheelDirectInput::UpdateButtonChangeStates() {
	for (auto device : foundGuids) {
		for (int i = 0; i < MAX_RGBBUTTONS; i++) {
			rgbButtonPrev[device][i] = rgbButtonCurr[device][i];
		}
		for (auto pov : POVDirections) {
			povButtonPrev[device][povDirectionToIndex(pov)] = povButtonCurr[device][povDirectionToIndex(pov)];
		}
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

	// TODO: Make joystickable
	// I'm focusing on steering wheels, so you get one axis.
	DWORD rgdwAxes[1] = { axis };
	LONG rglDirection[1] = { 0 };
	DICONSTANTFORCE cf = { 0 };

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

void WheelDirectInput::SetConstantForce(GUID device, WheelDirectInput::DIAxis ffAxis, int force) {
	// As per Microsoft's DirectInput example:
	// Modifying an effect is basically the same as creating a new one, except
	// you need only specify the parameters you are modifying
	auto e = FindEntryFromGUID(device);
	if (!e || !pCFEffect || !hasForceFeedback[device][ffAxis])
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
}

void WheelDirectInput::StopConstantForce() {
	pCFEffect->Stop();
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

void WheelDirectInput::updateAxisSpeed() {
	for (GUID device : foundGuids) {
		for (int i = 0; i < SIZEOF_DIAxis; i++) {
			DIAxis axis = static_cast<DIAxis>(i);

			auto time = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns
			auto position = GetAxisValue(axis, device);
			auto result = (position - prevPosition[device][i]) / ((time - prevTime[device][i]) / 1e9f);

			prevTime[device][i] = time;
			prevPosition[device][i] = position;

			samples[device][i][averageIndex[device][i]] = result;
			averageIndex[device][i] = (averageIndex[device][i] + 1) % (AVGSAMPLES - 1);
		}
	}
}

// Returns in units/s
float WheelDirectInput::GetAxisSpeed(DIAxis axis, GUID device) {
	auto sum = 0.0f;
	for (auto i = 0; i < AVGSAMPLES; i++) {
		sum += samples[device][axis][i];
	}
	return sum / AVGSAMPLES;
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

int WheelDirectInput::povDirectionToIndex(int povDirection) {
	switch (povDirection) {
		case N:		return 0;
		case NE:	return 1;
		case E:		return 2;
		case SE:	return 3;
		case S:		return 4;
		case SW:	return 5;
		case W:		return 6;
		case NW:	return 7;
		default:
			return -1;
	}
}