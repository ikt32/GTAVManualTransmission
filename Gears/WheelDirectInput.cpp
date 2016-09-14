#include "WheelDirectInput.hpp"
#include "TimeHelper.hpp"
#include "Logger.hpp"
#include <sstream>

//#define FAILED(hr) (((HRESULT)(hr)) < 0)
#include <winerror.h>

WheelDirectInput::WheelDirectInput(): pCFEffect{nullptr},
                                      pFREffect{nullptr} {
}

bool WheelDirectInput::InitWheel(std::string ffAxis) {
	Logger logger(LOGFILE);
	//logger.Clear();
	//logger.Write("Entered WheelDirectInput constructor");

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
			logger.Write("Initializing Force Feedback");
			HRESULT hr;
			if (FAILED( hr = e->diDevice->SetCooperativeLevel(
					GetForegroundWindow(),
					//nullptr, // wtf
					DISCL_EXCLUSIVE | DISCL_FOREGROUND))) {
				//if (hr == E_HANDLE) {
				std::stringstream ss;
				ss << std::hex << hr;
				logger.Write("Error " + ss.str());
				//E_HANDLE;
				//}
				return false;
			}

			logger.Write("Axis: " + ffAxis);
			if (!CreateConstantForceEffect(ffAxis)) {
				logger.Write("Error initializing Constant Force Effect");
			}
			/*if (!CreateCustomForceEffect(ffAxis, GUID_Friction)) {
				logger.Write("Error initializing Friction Force Effect");
			}
			if (!CreateCustomForceEffect(ffAxis, GUID_Damper)) {
				logger.Write("Error initializing Damper Force Effect");
			}*/
			JoyState = e->joystate;
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
		JoyState = e->joystate;;
	}
}

bool WheelDirectInput::IsConnected() {
	auto e = djs.getEntry(0);

	if (!e) {
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

bool WheelDirectInput::CreateConstantForceEffect(std::string axis) {
	DWORD ffAxis = DIJOFS_X;
	Logger log(LOGFILE);
	log.Write(axis);
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

bool WheelDirectInput::CreateCustomForceEffect(std::string axis, GUID effectGUID) {
	DWORD ffAxis = DIJOFS_X;
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
	LONG rglDirection[1] = { 0 };
	DICUSTOMFORCE cf = { 0 };

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
	eff.cbTypeSpecificParams = sizeof(DICUSTOMFORCE);
	eff.lpvTypeSpecificParams = &cf;
	eff.dwStartDelay = 0;

	const DiJoyStick::Entry* e = djs.getEntry(0);
	if (e) {
		e->diDevice->CreateEffect(
			effectGUID,
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
		return E_FAIL;
	}

	return hr;
}

HRESULT WheelDirectInput::SetCustomForce(int fric, int damp) {
	/*const DiJoyStick::Entry* e = djs.getEntry(0);
	if (e) {
		e->diDevice->Acquire();
		if (pCFEffect)
			pCFEffect->Start(1, 0);
		if (pFREffect)
			pFREffect->Start(1, 0);
	}
	LONG rglDirection[1] = { 0 };


	DICUSTOMFORCE cfFric;
	DICUSTOMFORCE cfDamp;

	cfFric.cChannels = 1;

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

	return pCFEffect->SetParameters(&cfEffect, DIEP_DIRECTION |
		DIEP_TYPESPECIFICPARAMS |
		DIEP_START);
	//ConditionalEffect *eff = static_cast<ConditionalEffect*>(effect->getForceEffect());

	//DWORD           rgdwAxes[2] = { DIJOFS_X, DIJOFS_Y };
	//LONG            rglDirection[2] = { 0, 0 };
	//DIENVELOPE      diEnvelope;
	//DICONDITION     cf;
	//DIEFFECT        diEffect;

	//cf.lOffset = eff->deadband;
	//cf.lPositiveCoefficient = eff->rightCoeff;
	//cf.lNegativeCoefficient = eff->leftCoeff;
	//cf.dwPositiveSaturation = eff->rightSaturation;
	//cf.dwNegativeSaturation = eff->leftSaturation;
	//cf.lDeadBand = eff->deadband;

	//_setCommonProperties(&diEffect, rgdwAxes, rglDirection, &diEnvelope, sizeof(DICONDITION), &cf, effect, 0);

	//switch (effect->type)
	//{
	//case OIS::Effect::Friction:	_upload(GUID_Friction, &diEffect, effect); break;
	//case OIS::Effect::Damper: _upload(GUID_Damper, &diEffect, effect); break;
	//case OIS::Effect::Inertia: _upload(GUID_Inertia, &diEffect, effect); break;
	//case OIS::Effect::Spring: _upload(GUID_Spring, &diEffect, effect); break;
	//default: break;
	//}*/
	return NULL;
}

WheelDirectInput::DIAxis WheelDirectInput::StringToAxis(std::string axisString) {
	for (int i = 0; i < SIZEOF_DIAxis; i++) {
		if (axisString == DIAxisHelper[i]) {
			return static_cast<DIAxis>(i);
		}
	}
	return UNKNOWN;
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

