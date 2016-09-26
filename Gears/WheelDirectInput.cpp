#include "WheelDirectInput.hpp"
#include "TimeHelper.hpp"
#include "Logger.hpp"
#include <sstream>

#include <winerror.h>
#include <chrono>

WheelDirectInput::WheelDirectInput(): pCFEffect{nullptr}
                                    , pFREffect{nullptr}  {
}

bool WheelDirectInput::InitWheel(std::string &ffAxis) {
	Logger logger(LOGFILE);

	if (SUCCEEDED(DirectInput8Create(GetModuleHandle(nullptr),
									 DIRECTINPUT_VERSION, 
									 IID_IDirectInput8, 
									 reinterpret_cast<void**>(&lpDi), 
									 nullptr))) {
		logger.Write("Initializing steering wheel");

		djs.enumerate(lpDi);
		djs.update();
		auto e = djs.getEntry(0);

		if (e) {
			logger.Write("Initializing force feedback");
			e->diDevice->Unacquire();
			HRESULT hr;
			if (FAILED( hr = e->diDevice->SetCooperativeLevel(
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
			logger.Write("Initializing force feedback effect on axis " + ffAxis);
			if (!CreateConstantForceEffect(ffAxis)) {
				logger.Write("Error initializing FF - wrong axis?");
				NoFeedback = true;
				return false;
			}
			logger.Write("Initializing force feedback success");
			UpdateState();
			UpdateState(); // kill me
			prevTime = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns
			logger.Write("Initializing wheel success");
			return true;
		}
	}
	logger.Write("No wheel detected");
	return false;
}



WheelDirectInput::~WheelDirectInput() {

}

void WheelDirectInput::UpdateState() {
	djs.update();

	const DiJoyStick::Entry* e = djs.getEntry(0);
	if (e) {
		JoyState = e->joystate;
	}
}

bool WheelDirectInput::IsConnected() const {
	auto e = djs.getEntry(0);

	if (!e) {
		return false;
	}	
	return true;
}

// Mental note: buttonType in these args means physical button number
// like how they are in DirectInput.
// If it matches the cardinal stuff the button is a POV hat thing

bool WheelDirectInput::IsButtonPressed(int buttonType) {
	if (buttonType > 127) {
		switch (buttonType) {
		case N:
			if (JoyState.rgdwPOV[0] == 0) {
				return true;
			}
		case NE:
		case E:
		case SE:
		case S:
		case SW:
		case W:
		case NW:
			if (buttonType == JoyState.rgdwPOV[0])
				return true;
		default:
			return false;
		}
	}	
	if (JoyState.rgbButtons[buttonType])
		return true;
	return false;
}

bool WheelDirectInput::IsButtonJustPressed(int buttonType) {
	if (buttonType > 127) { // POV
		povButtonCurr[buttonType] = IsButtonPressed(buttonType);

		// raising edge
		if (povButtonCurr[buttonType] && !povButtonPrev[buttonType]) {
			return true;
		}
		return false;
	}
	rgbButtonCurr[buttonType] = IsButtonPressed(buttonType);

	// raising edge
	if (rgbButtonCurr[buttonType] && !rgbButtonPrev[buttonType]) {
		return true;
	}
	return false;
}

bool WheelDirectInput::IsButtonJustReleased(int buttonType) {
	if (buttonType > 127) { // POV
		povButtonCurr[buttonType] = IsButtonPressed(buttonType);

		// falling edge
		if (!povButtonCurr[buttonType] && povButtonPrev[buttonType]) {
			return true;
		}
		return false;
	}
	rgbButtonCurr[buttonType] = IsButtonPressed(buttonType);

	// falling edge
	if (!rgbButtonCurr[buttonType] && rgbButtonPrev[buttonType]) {
		return true;
	}
	return false;
}

bool WheelDirectInput::WasButtonHeldForMs(int buttonType, int millis) {
	if (buttonType > 127) { // POV
		if (IsButtonJustPressed(buttonType)) {
			povPressTime[buttonType] = milliseconds_now();
		}
		if (IsButtonJustReleased(buttonType)) {
			povReleaseTime[buttonType] = milliseconds_now();
		}

		if ((povReleaseTime[buttonType] - povPressTime[buttonType]) >= millis) {
			povPressTime[buttonType] = 0;
			povReleaseTime[buttonType] = 0;
			return true;
		}
		return false;
	}
	if (IsButtonJustPressed(buttonType)) {
		rgbPressTime[buttonType] = milliseconds_now();
	}
	if (IsButtonJustReleased(buttonType)) {
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

bool WheelDirectInput::CreateConstantForceEffect(std::string &axis) {
	if (NoFeedback)
		return false;

	DWORD ffAxis;
	if (axis == "X") {
		ffAxis = DIJOFS_X;
	}
	else if (axis == "Y") {
		ffAxis = DIJOFS_Y;
	} 
	else if (axis == "Z") {
		ffAxis = DIJOFS_Z;
	}
	else {
		return false;
	}

	DWORD rgdwAxes[1] = { ffAxis };
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

	const DiJoyStick::Entry* e = djs.getEntry(0);
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

	const DiJoyStick::Entry* e = djs.getEntry(0);
	if (e) {
		e->diDevice->Acquire();
		if (pCFEffect)
			pCFEffect->Start(1, 0);
	} else {
		return E_HANDLE;
	}

	return hr;
}

WheelDirectInput::DIAxis WheelDirectInput::StringToAxis(std::string& axisString) {
	for (int i = 0; i < SIZEOF_DIAxis; i++) {
		if (axisString == DIAxisHelper[i]) {
			return static_cast<DIAxis>(i);
		}
	}
	return UNKNOWN_AXIS;
}


int WheelDirectInput::GetAxisValue(DIAxis axis) {
	if (!IsConnected())
		return 0;
	switch (axis) {
		case lX:			return JoyState.lX;
		case lY:			return JoyState.lY;
		case lZ:			return JoyState.lZ;
		case lRx:			return JoyState.lRx;
		case lRy:			return JoyState.lRy;
		case lRz:			return JoyState.lRz;
		case rglSlider0:	return JoyState.rglSlider[0];
		case rglSlider1:	return JoyState.rglSlider[1];
		default:			return 0;
	}
}

// Returns in units/s
float WheelDirectInput::GetAxisSpeed(DIAxis axis) {
	auto time = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns
	auto position = GetAxisValue(axis);
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

