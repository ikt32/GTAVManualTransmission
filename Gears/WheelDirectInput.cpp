#include "WheelDirectInput.hpp"
#include "TimeHelper.hpp"
#include "Logger.hpp"

#define FAILED(hr) (((HRESULT)(hr)) < 0)

WheelDirectInput::WheelDirectInput() {
	Logger logger(LOGFILE);
	
	if (SUCCEEDED(DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<void**>(&lpDi), nullptr))) {
		djs.enumerate(lpDi);
	}


	const DiJoyStick::Entry* e = djs.getEntry(0);

	if (e) {
		e->diDevice->SetCooperativeLevel(
			 GetForegroundWindow()
			 , DISCL_EXCLUSIVE |
			 DISCL_FOREGROUND);

		e->diDevice->Acquire();
		if (g_pEffect != nullptr) {
			if (!CreateEffect()) {
			}
			g_pEffect->Start(1, 0);
		}
		logger.Write("Found joy/wheel");
	}
}

WheelDirectInput::~WheelDirectInput() {

}

void WheelDirectInput::UpdateState() {
	djs.update();

	const DiJoyStick::Entry* e = djs.getEntry(0);

	if (e) {
		JoyState = e->joystate;;
	}
}

bool WheelDirectInput::IsConnected() {
	const DiJoyStick::Entry* e = djs.getEntry(0);

	if (e) {
		HRESULT hr;
		if ( FAILED( hr = e->diDevice->GetDeviceState( sizeof( DIJOYSTATE2 ), &JoyState ) ) ) {
			return false;
		}
	} else {
		return false;
	}

	if (djs.getEntryCount() > 0) {
		return true;
	}
	return false;
}

bool WheelDirectInput::IsButtonPressed(int buttonType) {
	if (JoyState.rgbButtons[buttonType]) {
		return true;
	}
	return false;
}

bool WheelDirectInput::IsButtonJustPressed(int buttonType) {
	rgbButtonCurr[buttonType] = IsButtonPressed(buttonType);

	// raising edge
	if (rgbButtonCurr[buttonType] && !rgbButtonPrev[buttonType]) {
		return true;
	}
	return false;
}

bool WheelDirectInput::IsButtonJustReleased(int buttonType) {
	rgbButtonCurr[buttonType] = IsButtonPressed(buttonType);

	// falling edge
	if (!rgbButtonCurr[buttonType] && rgbButtonPrev[buttonType]) {
		return true;
	}
	return false;
}

bool WheelDirectInput::WasButtonHeldForMs(int buttonType, int millis) {
	if (IsButtonJustPressed(buttonType)) {
		pressTime[buttonType] = milliseconds_now();
	}
	if (IsButtonJustReleased(buttonType)) {
		releaseTime[buttonType] = milliseconds_now();
	}

	if ((releaseTime[buttonType] - pressTime[buttonType]) >= millis) {
		pressTime[buttonType] = 0;
		releaseTime[buttonType] = 0;
		return true;
	}
	return false;
}

void WheelDirectInput::UpdateButtonChangeStates() {
	for (int i = 0; i < MAX_RGBBUTTONS; i++) {
		rgbButtonPrev[i] = rgbButtonCurr[i];
	}
}

bool WheelDirectInput::CreateEffect() {
	DWORD rgdwAxes[1] = {DIJOFS_X};
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
			 &g_pEffect,
			 nullptr);
		if (!g_pEffect) {
			return false;
		}
		return true;
	}
	return false;
}

HRESULT WheelDirectInput::SetForce(int force) const {
	
	const DiJoyStick::Entry* e = djs.getEntry(0);
	if (e) {
		e->diDevice->Acquire();
		if (g_pEffect)
			g_pEffect->Start(1, 0);
	}
	LONG rglDirection[1] = {0};
	DICONSTANTFORCE cf;
	cf.lMagnitude = force;

	DIEFFECT eff;
	ZeroMemory(&eff, sizeof(eff));
	eff.dwSize = sizeof(DIEFFECT);
	eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
	eff.cAxes = 1;
	eff.rglDirection = rglDirection;
	eff.lpEnvelope = nullptr;
	eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
	eff.lpvTypeSpecificParams = &cf;
	eff.dwStartDelay = 0;

	return g_pEffect->SetParameters(&eff, DIEP_DIRECTION |
	                                DIEP_TYPESPECIFICPARAMS |
	                                DIEP_START);
}

WheelDirectInput::DIAxis WheelDirectInput::StringToAxis(std::string axisString) {
	for (int i = 0; i < SIZEOF_DIAxis; i++) {
		if (axisString == DIAxisHelper[i]) {
			return static_cast<DIAxis>(i);
		}
	}
	return UNKNOWN;
}


// fucking hell why
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
		case lVX:			return JoyState.lVX;
		case lVY:			return JoyState.lVY;
		case lVZ:			return JoyState.lVZ;
		case lVRx:			return JoyState.lVRx;
		case lVRy:			return JoyState.lVRy;
		case lVRz:			return JoyState.lVRz;
		case rglVSlider0:	return JoyState.rglVSlider[0];
		case rglVSlider1:	return JoyState.rglVSlider[1];
		case lAX:			return JoyState.lAX;
		case lAY:			return JoyState.lAY;
		case lAZ:			return JoyState.lAZ;
		case lARx:			return JoyState.lARx;
		case lARy:			return JoyState.lARy;
		case lARz:			return JoyState.lARz;
		case rglASlider0:	return JoyState.rglASlider[0];
		case rglASlider1:	return JoyState.rglASlider[1];
		case lFX:			return JoyState.lFX;
		case lFY:			return JoyState.lFY;
		case lFZ:			return JoyState.lFZ;
		case lFRx:			return JoyState.lFRx;
		case lFRy:			return JoyState.lFRy;
		case lFRz:			return JoyState.lFRz;
		case rglFSlider0:	return JoyState.rglFSlider[0];
		case rglFSlider1:	return JoyState.rglFSlider[0];
		default:			return 0;
	}
}

