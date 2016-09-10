#include "WheelDirectInput.hpp"
#include "TimeHelper.hpp"
#include "../../ScriptHookV_SDK/inc/natives.h"
#include "Logger.hpp"

#define LOGFILE_DI "./Gears_D.log"

WheelInput::WheelInput() {
	Logger logger(LOGFILE_DI);
	logger.Write("INIT DInput");
	if (SUCCEEDED(DirectInput8Create(GetModuleHandle(0), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&lpDi, 0))) {
		djs.enumerate(lpDi);
	}
	logger.Write("DInput SUCCESS");


	const DiJoyStick::Entry* e = djs.getEntry(0);

	if (e) {
		e->diDevice->SetCooperativeLevel(
			GetForegroundWindow()
			, DISCL_EXCLUSIVE |
			DISCL_FOREGROUND);

		logger.Write("Acquiring device");
		e->diDevice->Acquire();
		logger.Write("Do we have the effect?");
		if (g_pEffect != nullptr) {
			if (!CreateEffect()) {
				logger.Write("Something went wrong.");
			}
			g_pEffect->Start(1, 0);
			logger.Write("We do the effect.");
		}
	}

}

WheelInput::~WheelInput() {

}

const DIJOYSTATE2* WheelInput::GetState() {
	djs.update();
	Logger logger(LOGFILE_DI);

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

void WheelInput::PlayWheelEffects(
	float speed,
	Vector3 accelVals,
	Vector3 accelValsAvg,
	ScriptSettings* settings,
	bool airborne) {
	Logger log(LOGFILE_DI);

	//LogiPlayLeds(logiWheel.GetIndex(), vehData.Rpm, 0.66f, 0.99f);
	if (settings == nullptr) {
		log.Write("settings == nullptr");
		return;
	}

	/*if (settings->FFDamperStationary < settings->FFDamperMoving) {
		settings->FFDamperMoving = settings->FFDamperStationary;
	}
	if (settings->FFDamperMoving < 1) {
		settings->FFDamperMoving = 1;
	}
	float ratio = (float)(settings->FFDamperStationary - settings->FFDamperMoving) / settings->FFDamperMoving;
	int damperforce = settings->FFDamperStationary - (int)(8 * ratio * ratio * speed*speed);
	if (damperforce < settings->FFDamperMoving) {
		damperforce = settings->FFDamperMoving + (int)(speed * ratio);
	}
	damperforce -= (int)(ratio * accelValsAvg.y);
	if (damperforce > settings->FFDamperStationary) {
		damperforce = settings->FFDamperStationary;
	}
	*/


	//LogiPlayDamperForce(logiWheel.GetIndex(), damperforce);

	int constantForce = 100*static_cast<int>(-settings->FFPhysics * ((3 * accelValsAvg.x + 2 * accelVals.x)));

	const DiJoyStick::Entry* e = djs.getEntry(0);
	if (e) {
		e->diDevice->Acquire();
		if (g_pEffect)
			g_pEffect->Start(1, 0);
	}
	HRESULT hr = SetForce(constantForce);
	switch (hr) {
	case DI_DOWNLOADSKIPPED:
		log.Write("SetForce DI_DOWNLOADSKIPPED");
		break;
	case DI_EFFECTRESTARTED:
		log.Write("SetForce DI_EFFECTRESTARTED");
		break;
	case DI_OK:
		log.Write("SetForce DI_OK");
		break;
	case DI_TRUNCATED:
		log.Write("SetForce DI_TRUNCATED");
		break;
	case DI_TRUNCATEDANDRESTARTED:
		log.Write("SetForce DI_TRUNCATEDANDRESTARTED");
		break;
	case DIERR_NOTINITIALIZED:
		log.Write("SetForce DIERR_NOTINITIALIZED");
		break;
	case DIERR_INCOMPLETEEFFECT:
		log.Write("SetForce DIERR_INCOMPLETEEFFECT");
		break;
	case DIERR_INPUTLOST:
		log.Write("SetForce DIERR_INPUTLOST");
		break;
	case DIERR_INVALIDPARAM:
		log.Write("SetForce DIERR_INVALIDPARAM");
		break;
	case DIERR_EFFECTPLAYING:
		log.Write("SetForce DIERR_EFFECTPLAYING");
		break;
	}	

	//LogiPlayConstantForce(logiWheel.GetIndex(), constantForce);

	/*int centerForcePercentage = (int)(settings->FFCenterSpring * accelValsAvg.y);
	if (centerForcePercentage < 0) {
		centerForcePercentage = 0;
	}

	LogiPlaySpringForce(
		logiWheel.GetIndex(),
		0,
		(int)(settings->FFCenterSpring*speed + centerForcePercentage),
		(int)(settings->FFCenterSpring*speed + centerForcePercentage));

	if (!VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle) && ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(vehicle) > 1.25f) {
		LogiPlayCarAirborne(logiWheel.GetIndex());
	}
	else if (LogiIsPlaying(logiWheel.GetIndex(), LOGI_FORCE_CAR_AIRBORNE)) {
		LogiStopCarAirborne(logiWheel.GetIndex());
	}*/
}

bool WheelInput::CreateEffect() {
	Logger log(LOGFILE_DI);
	log.Write("Init CreateEffect");
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
			log.Write("CreateEffect FAIL");
			return false;
		}
		log.Write("CreateEffect SUCCESS");
		return true;
	}
	return false;
}

HRESULT WheelInput::SetForce(int force) {
	Logger log(LOGFILE_DI);
	log.Write("Init SetForce");
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
