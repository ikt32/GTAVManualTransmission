#include "WheelDirectInput.hpp"
#include "TimeHelper.hpp"

WheelInput::WheelInput() {
	if (SUCCEEDED(DirectInput8Create(GetModuleHandle(0), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&lpDi, 0))) {
		djs.enumerate(lpDi);
	}
}

WheelInput::~WheelInput() {}

const DIJOYSTATE2* WheelInput::GetState() {
	djs.update();

	const DiJoyStick::Entry* e = djs.getEntry(0);

	if (e) {

		const DIJOYSTATE2* js = &e->joystate;
		joyState = e->joystate;
		return js;
		/*
		int pov = js->rgdwPOV[0];
		if (pov < 0) {
			pov = -1;
		}
		else {
			pov /= 100;
		}

		int btn = 0;
		for (int i = 0; i < 32; ++i) {
			if (js->rgbButtons[i]) {
				btn |= 1 << i;
			}
		}

		int wheel = js->lX;
		int throttle = js->lY;
		int brake = js->lRz;
		int clutch = js->rglSlider[1];

		printf("t=%i, b=%i, c=%i, wheel=%6d pov=%3d, button=%08x\r", throttle, brake, clutch, wheel, pov, btn);

		*
		*/
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
