#include "WheelDirectInput.hpp"
#include "TimeHelper.hpp"

WheelInput::WheelInput() {
	if (SUCCEEDED(DirectInput8Create(GetModuleHandle(0), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&lpDi, 0))) {
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
	}

}

WheelInput::~WheelInput() {

}

const DIJOYSTATE2* WheelInput::GetState() {
	djs.update();

	const DiJoyStick::Entry* e = djs.getEntry(0);

	if (e) {

		const DIJOYSTATE2* js = &e->joystate;
		joyState = e->joystate;
		return js;
	}
	return nullptr;
}

bool WheelInput::IsConnected() const {
	if (djs.getEntryCount() > 0)
		return true;
	return false;
}

bool WheelInput::IsButtonPressed(int buttonType) {
	if (joyState.rgbButtons[buttonType]) {
		return true;
	}
	return false;
}

bool WheelInput::IsButtonJustPressed(int buttonType) {
	rgbButtonCurr[buttonType] = IsButtonPressed(buttonType);

	// raising edge
	if (rgbButtonCurr[buttonType] && !rgbButtonPrev[buttonType]) {
		return true;
	}
	return false;
}

bool WheelInput::IsButtonJustReleased(int buttonType) {
	rgbButtonCurr[buttonType] = IsButtonPressed(buttonType);

	// falling edge
	if (!rgbButtonCurr[buttonType] && rgbButtonPrev[buttonType]) {
		return true;
	}
	return false;
}

bool WheelInput::WasButtonHeldForMs(int buttonType, int millis) {
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
void WheelInput::UpdateButtonChangeStates() {
	for (int i = 0; i < MAX_RGBBUTTONS; i++) {
		rgbButtonPrev[i] = rgbButtonCurr[i];
	}
}

bool WheelInput::CreateEffect() {
	DWORD rgdwAxes[1] = { DIJOFS_X };
	LONG rglDirection[1] = { 0 };
	DICONSTANTFORCE cf = { 0 };

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
	eff.lpEnvelope = 0;
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

HRESULT WheelInput::SetForce(int force) {

	const DiJoyStick::Entry* e = djs.getEntry(0);
	if (e) {
		e->diDevice->Acquire();
		if (g_pEffect)
			g_pEffect->Start(1, 0);
	}
	LONG rglDirection[1] = { 0 };
	DICONSTANTFORCE cf;
	cf.lMagnitude = force;

	DIEFFECT eff;
	ZeroMemory(&eff, sizeof(eff));
	eff.dwSize = sizeof(DIEFFECT);
	eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
	eff.cAxes = 1;
	eff.rglDirection = rglDirection;
	eff.lpEnvelope = 0;
	eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
	eff.lpvTypeSpecificParams = &cf;
	eff.dwStartDelay = 0;

	return g_pEffect->SetParameters(&eff, DIEP_DIRECTION |
		DIEP_TYPESPECIFICPARAMS |
		DIEP_START);
}
