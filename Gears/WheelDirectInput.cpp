#include "WheelDirectInput.hpp"

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

bool WheelInput::IsButtonPressed() {
	return false;
}

bool WheelInput::IsButtonJustPressed() {
	return false;
}

bool WheelInput::IsButtonJustReleased() {
	return false;
}

bool WheelInput::WasButtonHeldForMs() {
	return false;
}
void WheelInput::UpdateButtonChangeStates() {}
