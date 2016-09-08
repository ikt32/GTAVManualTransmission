#include <string>
#include <sstream>
#include <iomanip>

#include "../../ScriptHookV_SDK/inc/natives.h"
#include "../../ScriptHookV_SDK/inc/types.h"
#include "../../ScriptHookV_SDK/inc/enums.h"
#include "../../ScriptHookV_SDK/inc/main.h"

#include "script.h"

#include "ScriptSettings.hpp"
#include "ScriptControls.hpp"
#include "Logger.hpp"
#include "VehicleData.hpp"
#include "MemoryPatcher.hpp"
#include "XboxController.hpp"
#include "WheelLogiInput.hpp"

Logger logger(LOGFILE);
ScriptControls controls;
ScriptSettings settings;

Player player;
Ped playerPed;
Vehicle vehicle;
Vehicle prevVehicle;
VehicleData vehData;
VehicleExtensions ext;
Hash model;

XboxController controller(1);
WORD buttonState;

//WheelLogiInput logiWheel(0);

bool patched = false;
bool simpleBike = false;
int prevNotification = 0;
int prevInput = 0;

float prevRpm;

// This gonna be refactored into vehData or LogiInput somehow
bool blinkerLeft = false;
bool blinkerRight = false;
bool blinkerHazard = false;
bool truckShiftUp = false;
DIJOYSTATE2* logiState;


void update() {
	///////////////////////////////////////////////////////////////////////////
	//                    Gathering data
	///////////////////////////////////////////////////////////////////////////
	player = PLAYER::PLAYER_ID();
	playerPed = PLAYER::PLAYER_PED_ID();

	if (!ENTITY::DOES_ENTITY_EXIST(playerPed) ||
		!PLAYER::IS_PLAYER_CONTROL_ON(player) ||
		ENTITY::IS_ENTITY_DEAD(playerPed) ||
		PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE)) {
		reset();
		return;
	}

	if (!PED::IS_PED_IN_ANY_VEHICLE(playerPed, true)) {
		reset();
		return;
	}

	vehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);
	model = ENTITY::GET_ENTITY_MODEL(vehicle);

	if (!ENTITY::DOES_ENTITY_EXIST(vehicle) ||
		VEHICLE::IS_THIS_MODEL_A_BICYCLE(model) ||
		!(VEHICLE::IS_THIS_MODEL_A_CAR(model) ||
			VEHICLE::IS_THIS_MODEL_A_BIKE(model) ||
			VEHICLE::IS_THIS_MODEL_A_QUADBIKE(model)
		)) {
		reset();
		return;
	}

	vehData.ReadMemData(ext, vehicle);
	vehData.LockGear = (0xFFFF0000 & vehData.LockGears) >> 16;
	simpleBike = vehData.IsBike && settings.SimpleBike;

	if (prevVehicle != vehicle) {
		//std::stringstream vehName;
		//vehName << "New: " << VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(model);
		//showNotification(vehName.str().c_str());
		if (vehData.NoClutch) {
			vehData.SimulatedNeutral = false;
		}
		else {
			vehData.SimulatedNeutral = settings.DefaultNeutral;
		}
		shiftTo(1, true);
	}
	prevVehicle = vehicle;


	if (controller.IsConnected()) {
		buttonState = controller.GetState().Gamepad.wButtons;
		controller.UpdateButtonChangeStates();
	}

	if (settings.LogiWheel) {
		LogiUpdate();
	}

	if (controls.IsKeyJustPressed(controls.Control[static_cast<int>(ScriptControls::KeyboardControlType::Toggle)], ScriptControls::KeyboardControlType::Toggle) ||
		controller.WasButtonHeldForMs(controller.StringToButton(controls.ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Toggle)]), buttonState, controls.CToggleTime)) {
		// ||	(LogiButtonTriggered(logiWheel.GetIndex(), controls.LogiControl[(int)ScriptControls::LogiControlType::Toggle]))) {
		toggleManual();
	}

	crossScriptComms();

	if (settings.Debug) {
		showDebugInfo();
	}

	if (!settings.EnableManual ||
	(!VEHICLE::IS_VEHICLE_DRIVEABLE(vehicle, false) &&
		VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) < -100.0f)) {
		return;
	}
	///////////////////////////////////////////////////////////////////////////
	// Active whenever Manual is enabled from here
	///////////////////////////////////////////////////////////////////////////
	handleVehicleButtons();

	if (controls.IsKeyJustPressed(controls.Control[static_cast<int>(ScriptControls::KeyboardControlType::ToggleH)], ScriptControls::KeyboardControlType::ToggleH)) {
		// || (logiWheel.IsActive(&settings) && LogiButtonTriggered(logiWheel.GetIndex(), controls.LogiControl[(int)ScriptControls::LogiControlType::ToggleH]))) {
		settings.Hshifter = !settings.Hshifter;
		if (!settings.Hshifter && vehData.CurrGear > 1) {
			vehData.SimulatedNeutral = false;
		}
		std::stringstream message;
		message << "Mode: " <<
				(settings.Hshifter ? "H-Shifter" : "Sequential");
		showNotification(const_cast<char *>(message.str().c_str()));
		settings.Save();
	}

	/*if (logiWheel.IsActive(&settings)) {
		logiWheel.UpdateLogiValues();
		logiState = LogiGetState(logiWheel.GetIndex());
	}*/

	if (prevInput != getLastInputDevice(prevInput)) {
		prevInput = getLastInputDevice(prevInput);
		switch (prevInput) {
			case InputDevices::Keyboard:
				showNotification("Switched to keyboard/mouse");
				break;
			case InputDevices::Controller: // Controller
				if (settings.Hshifter) {
					showNotification("Switched to controller\nSequential re-initiated");
					settings.Hshifter = false;
					settings.Save();
				}
				else {
					showNotification("Switched to controller");
				}
				break;
				//case InputDevices::Wheel: // Wheel
				//	//CONTROLS::DISABLE_ALL_CONTROL_ACTIONS
				//	showNotification("Switched to wheel");
				//	break;
			default:
				break;
		}
	}
	switch (prevInput) {
		case InputDevices::Keyboard:
			controls.Rtvalf = (controls.IsKeyPressed(controls.Control[static_cast<int>(ScriptControls::KeyboardControlType::Throttle)]) ? 1.0f : 0.0f);
			controls.Ltvalf = (controls.IsKeyPressed(controls.Control[static_cast<int>(ScriptControls::KeyboardControlType::Brake)]) ? 1.0f : 0.0f);
			controls.Clutchvalf = (controls.IsKeyPressed(controls.Control[static_cast<int>(ScriptControls::KeyboardControlType::Clutch)]) ? 1.0f : 0.0f);
			break;
		case InputDevices::Controller: // Controller
			controls.Rtvalf = controller.GetAnalogValue(controller.StringToButton(controls.ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Throttle)]), buttonState);
			controls.Ltvalf = controller.GetAnalogValue(controller.StringToButton(controls.ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Brake)]), buttonState);
			controls.Clutchvalf = controller.GetAnalogValue(controller.StringToButton(controls.ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Clutch)]), buttonState);
			break;
			//case InputDevices::Wheel: // Wheel
			//	controls.Rtvalf = logiWheel.GetLogiThrottleVal();
			//	controls.Ltvalf = logiWheel.GetLogiBrakeVal();
			//	controls.Clutchvalf = 1 - logiWheel.GetLogiClutchVal();
			//	break;
	}

	controls.Accelval = CONTROLS::GET_CONTROL_VALUE(0, ControlVehicleAccelerate);
	controls.Accelvalf = (controls.Accelval - 127) / 127.0f;

	/*if (logiWheel.IsActive(&settings) && logiState != NULL && prevInput == InputDevices::Wheel) {
		if (settings.FFEnable)
			playWheelEffects();
		float steerVal = (logiState->lX) / (-32768.0f);
		ext.SetSteeringAngle1(vehicle, steerVal);
		logiWheel.DoWheelSteering(steerVal);
	}*/

	///////////////////////////////////////////////////////////////////////////
	//                            Patching
	///////////////////////////////////////////////////////////////////////////
	if (!patched && settings.EnableManual) {
		patched = MemoryPatcher::PatchInstructions();
	}

	if (settings.UITips) {
		if (vehData.SimulatedNeutral) {
			showText(settings.UITips_X, settings.UITips_Y, settings.UITips_Size, "N");
		}
		else {
			showText(settings.UITips_X, settings.UITips_Y, settings.UITips_Size, const_cast<char *>(std::to_string(vehData.CurrGear).c_str()));
		}
	}
	///////////////////////////////////////////////////////////////////////////
	// Actual mod operations
	///////////////////////////////////////////////////////////////////////////

	// Reverse behavior
	// For bikes, do this automatically.
	if (vehData.IsBike) {
		functionAutoReverse();
		/*handlePedalsDefault(
			logiWheel.GetLogiThrottleVal(),
			logiWheel.GetLogiBrakeVal());*/
	}
	else {
		// New reverse: Reverse with throttle
		if (settings.RealReverse) {
			functionRealReverse();
			/*if (logiWheel.IsActive(&settings) && prevInput == InputDevices::Wheel) {
				handlePedalsRealReverse(
					logiWheel.GetLogiThrottleVal(),
					logiWheel.GetLogiBrakeVal());
			}*/
		}
		// Reversing behavior: just block the direction you don't wanna go to
		else {
			functionSimpleReverse();
			/*if (logiWheel.IsActive(&settings) && prevInput == InputDevices::Wheel) {
				handlePedalsDefault(
					logiWheel.GetLogiThrottleVal(),
					logiWheel.GetLogiBrakeVal());
			}*/
		}
	}


	// Limit truck speed per gear upon game wanting to shift, but we block it.
	if (vehData.IsTruck) {
		functionTruckLimiting();
	}

	// Since we know prevGear and the speed for that, let's simulate "engine" braking.
	if (settings.EngBrake) {
		functionEngBrake();
	}

	// Engine damage: RPM Damage
	if (settings.EngDamage) {
		functionEngDamage();
	}

	if (!vehData.SimulatedNeutral && !simpleBike && !vehData.NoClutch) {
		// Stalling
		if (settings.EngStall) {
			functionEngStall();
		}

		// Simulate "catch point"
		// When the clutch "grabs" and the car starts moving without input
		if (settings.ClutchCatching) {
			functionClutchCatch();
		}
	}


	// Manual shifting
	if (settings.Hshifter && !vehData.IsBike) {
		/*if (logiWheel.IsActive(&settings) && prevInput == InputDevices::Wheel) {
			functionHShiftLogitech();
		}
		else {
			functionHShiftKeyboard();
		}*/
		functionHShiftKeyboard();
	}
	else {
		functionSShift();
	}

	// Finally, update memory each loop
	handleRPM();
	ext.SetGears(vehicle, vehData.LockGears);
}

void main() {
	settings.Read(&controls);
	/*if (settings.LogiWheel) {
		logiWheel.InitWheel(&settings, &logger);
	}*/

	while (true) {
		update();
		WAIT(0);
	}
}

void ScriptMain() {
	srand(GetTickCount());
	main();
}

///////////////////////////////////////////////////////////////////////////////
//                           Helper functions/tools
///////////////////////////////////////////////////////////////////////////////

void showText(float x, float y, float scale, const char* text) {
	UI::SET_TEXT_FONT(0);
	UI::SET_TEXT_SCALE(scale, scale);
	UI::SET_TEXT_COLOUR(255, 255, 255, 255);
	UI::SET_TEXT_WRAP(0.0, 1.0);
	UI::SET_TEXT_CENTRE(0);
	UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
	UI::SET_TEXT_EDGE(1, 0, 0, 0, 205);
	UI::_SET_TEXT_ENTRY("STRING");
	UI::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(const_cast<char *>(text));
	UI::_DRAW_TEXT(x, y);
}

void showNotification(char* message) {
	if (prevNotification)
		UI::_REMOVE_NOTIFICATION(prevNotification);
	UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
	UI::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(message);
	prevNotification = UI::_DRAW_NOTIFICATION(false, false);
}

void showDebugInfo() {
	std::stringstream infos;
	infos << "RPM: " << std::setprecision(3) << vehData.Rpm <<
			"\nCurrGear: " << vehData.CurrGear <<
			"\nNextGear: " << vehData.NextGear <<
			"\nClutch: " << std::setprecision(3) << vehData.Clutch <<
			"\nThrottle: " << std::setprecision(3) << vehData.Throttle <<
			"\nTurbo: " << std::setprecision(3) << vehData.Turbo <<
			"\nAddress: " << std::hex << vehData.Address <<
			"\nE: " << (settings.EnableManual ? "Y" : "N");
	const char* infoc = infos.str().c_str();
	showText(0.01f, 0.5f, 0.4f, infoc);

	/*if (logiWheel.IsActive(&settings) && logiState != NULL) {
		std::stringstream throttleDisplay;
		throttleDisplay << "ThrottleVal: " << logiWheel.GetLogiThrottleVal() << std::endl <<
			"ThrottleRaw: " << logiState->lY;
		std::stringstream brakeDisplay;
		brakeDisplay	<< "Brake Value: " << logiWheel.GetLogiBrakeVal() << std::endl <<
			"Brake   Raw: " << logiState->lRz;
		std::stringstream clutchDisplay;
		clutchDisplay	<< "ClutchValue: " << logiWheel.GetLogiClutchVal() << std::endl <<
			"Clutch  Raw: " << logiState->rglSlider[1];

		showText(0.85, 0.04, 0.4, throttleDisplay.str().c_str());
		showText(0.85, 0.10, 0.4, brakeDisplay.str().c_str());
		showText(0.85, 0.16, 0.4, clutchDisplay.str().c_str());
	}*/
}

// Limitations: Detects on pressing throttle on any of the 3 input methods
int getLastInputDevice(int previousInput) {
	if (controls.IsKeyJustPressed(controls.Control[static_cast<int>(ScriptControls::KeyboardControlType::Throttle)], ScriptControls::KeyboardControlType::Throttle) ||
		controls.IsKeyPressed(controls.Control[static_cast<int>(ScriptControls::KeyboardControlType::Throttle)])) {
		return InputDevices::Keyboard;
	}
	if (controller.IsButtonJustPressed(controller.StringToButton(controls.ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Throttle)]), buttonState) ||
		controller.IsButtonPressed(controller.StringToButton(controls.ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Throttle)]), buttonState)) {
		return InputDevices::Controller;
	}
	/*if (logiWheel.IsActive(&settings) &&
		(logiWheel.GetLogiThrottleVal() > 0.1f ||
			logiWheel.GetLogiBrakeVal() > 0.1f ||
			logiWheel.GetLogiClutchVal() < 0.9f)) {
		return InputDevices::Wheel;
	}*/
	return previousInput;
}

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Mod control
///////////////////////////////////////////////////////////////////////////////

void reInit() {
	settings.Read(&controls);
	vehData.LockGears = 0x00010001;
	vehData.SimulatedNeutral = settings.DefaultNeutral;
	/*if (settings.LogiWheel) {
		logiWheel.InitWheel(&settings, &logger);
	}*/
}

void reset() {
	/*if (logiWheel.IsActive(&settings)) {
		resetWheelFeedback(logiWheel.GetIndex());
	}*/
	prevVehicle = 0;
	if (patched) {
		patched = !MemoryPatcher::RestoreInstructions();
	}
}

void toggleManual() {
	settings.EnableManual = !settings.EnableManual;
	settings.Save();
	std::stringstream message;
	message << "Manual Transmission " <<
			(settings.EnableManual ? "Enabled" : "Disabled");
	showNotification(const_cast<char *>(message.str().c_str()));
	logger.Write(const_cast<char *>(message.str().c_str()));
	if (ENTITY::DOES_ENTITY_EXIST(vehicle)) {
		VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, false, true);
	}
	if (!settings.EnableManual) {
		reset();
		LogiSteeringShutdown();
	}
	reInit();
}

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Shifting
///////////////////////////////////////////////////////////////////////////////

void shiftTo(int gear, bool autoClutch) {
	if (autoClutch) {
		controls.Clutchvalf = 1.0f;
		ext.SetThrottle(vehicle, 0.0f);
	}
	vehData.LockGears = gear | (gear << 16);
	vehData.LockTruck = false;
	vehData.PrevGear = vehData.CurrGear;
	if (vehData.IsTruck && vehData.Rpm < 0.9) {
		return;
	}
	vehData.LockSpeeds[vehData.CurrGear] = vehData.Velocity;
}

void functionHShiftTo(int i) {
	if (settings.ClutchShifting && !vehData.NoClutch) {
		if (controls.Clutchvalf > 1.0f - settings.ClutchCatchpoint) {
			shiftTo(i, false);
			vehData.SimulatedNeutral = false;
		}
		else {
			vehData.SimulatedNeutral = true;
			if (settings.EngDamage) {
				VEHICLE::SET_VEHICLE_ENGINE_HEALTH(
					vehicle,
					VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) - settings.MisshiftDamage);
			}
		}
	}
	else {
		shiftTo(i, true);
		vehData.SimulatedNeutral = false;
	}
}

void functionHShiftKeyboard() {
	// Highest shifter button == 8
	int clamp = 8;
	if (vehData.TopGear <= clamp) {
		clamp = vehData.TopGear;
	}
	for (uint8_t i = 0; i <= clamp; i++) {
		if (controls.IsKeyJustPressed(controls.Control[i], static_cast<ScriptControls::KeyboardControlType>(i))) {
			functionHShiftTo(i);
		}
	}
	if (controls.IsKeyJustPressed(controls.Control[static_cast<int>(ScriptControls::KeyboardControlType::HN)], ScriptControls::KeyboardControlType::HN)
		&& !vehData.NoClutch) {
		vehData.SimulatedNeutral = !vehData.SimulatedNeutral;
	}
}

//void functionhshiftlogitech() {
//	// highest shifter button == 6
//	int clamp = 6;
//	if (vehdata.topgear <= clamp) {
//		clamp = vehdata.topgear;
//	}
//	for (uint8_t i = 0; i <= clamp; i++) {
//		if (logibuttontriggered(logiwheel.getindex(), controls.logicontrol[i])) {
//			functionhshiftto(i);
//		}
//	}
//
//	
//	if (logibuttonreleased(logiwheel.getindex(), controls.logicontrol[(int)scriptcontrols::logicontroltype::h1]) ||
//		logibuttonreleased(logiwheel.getindex(), controls.logicontrol[(int)scriptcontrols::logicontroltype::h2]) ||
//		logibuttonreleased(logiwheel.getindex(), controls.logicontrol[(int)scriptcontrols::logicontroltype::h3]) ||
//		logibuttonreleased(logiwheel.getindex(), controls.logicontrol[(int)scriptcontrols::logicontroltype::h4]) ||
//		logibuttonreleased(logiwheel.getindex(), controls.logicontrol[(int)scriptcontrols::logicontroltype::h5]) ||
//		logibuttonreleased(logiwheel.getindex(), controls.logicontrol[(int)scriptcontrols::logicontroltype::h6])) {
//		if (settings.clutchshifting && settings.engdamage && !vehdata.noclutch) {
//			if (controls.clutchvalf < 1.0-settings.clutchcatchpoint) {
//				vehicle::set_vehicle_engine_health(
//					vehicle,
//					vehicle::get_vehicle_engine_health(vehicle) - settings.misshiftdamage/10);
//			}
//		}
//		vehdata.simulatedneutral = !vehdata.noclutch;
//	}
//	
//	if (logibuttonreleased(logiwheel.getindex(), controls.logicontrol[(int)scriptcontrols::logicontroltype::hr])) {
//		shiftto(1, true);
//		vehdata.simulatedneutral = !vehdata.noclutch;
//	}
//}


void functionSShift() {
	// Shift up
	if (controller.IsButtonJustReleased(controller.StringToButton(controls.ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ShiftUp)]), buttonState) ||
		controls.IsKeyJustPressed(controls.Control[static_cast<int>(ScriptControls::KeyboardControlType::ShiftUp)], ScriptControls::KeyboardControlType::ShiftUp)) {
		//|| (logiWheel.IsActive(&settings) && LogiButtonTriggered(logiWheel.GetIndex(), controls.LogiControl[(int)ScriptControls::LogiControlType::ShiftUp]))) {

		if (vehData.NoClutch) {
			if (vehData.CurrGear < vehData.TopGear) {
				shiftTo(vehData.LockGear + 1, true);
			}
			return;
		}

		// Reverse to Neutral
		if (vehData.CurrGear == 0 && !vehData.SimulatedNeutral) {
			shiftTo(1, true);
			vehData.SimulatedNeutral = true;
			return;
		}

		// Neutral to 1
		if (vehData.CurrGear == 1 && vehData.SimulatedNeutral) {
			vehData.SimulatedNeutral = false;
			return;
		}

		// 1 to X
		if (vehData.CurrGear < vehData.TopGear) {
			shiftTo(vehData.LockGear + 1, true);
		}
	}

	// Shift down
	if (controller.IsButtonJustReleased(controller.StringToButton(controls.ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ShiftDown)]), buttonState) ||
		controls.IsKeyJustPressed(controls.Control[static_cast<int>(ScriptControls::KeyboardControlType::ShiftDown)], ScriptControls::KeyboardControlType::ShiftDown)) {
		// || (logiWheel.IsActive(&settings) && LogiButtonTriggered(logiWheel.GetIndex(), controls.LogiControl[(int)ScriptControls::LogiControlType::ShiftDown]))) {

		if (vehData.NoClutch) {
			if (vehData.CurrGear > 0) {
				shiftTo(vehData.LockGear - 1, true);
			}
			return;
		}

		// 1 to Neutral
		if (vehData.CurrGear == 1 && !vehData.SimulatedNeutral) {
			vehData.SimulatedNeutral = true;
			return;
		}

		// Neutral to R
		if (vehData.CurrGear == 1 && vehData.SimulatedNeutral) {
			shiftTo(0, true);
			vehData.SimulatedNeutral = false;
			return;
		}

		// X to 1
		if (vehData.CurrGear > 1) {
			shiftTo(vehData.LockGear - 1, true);
		}
	}

}

///////////////////////////////////////////////////////////////////////////////
//                          Mod functions: Features
///////////////////////////////////////////////////////////////////////////////

void functionClutchCatch() {
	if (controls.Clutchvalf < 1.0f - settings.ClutchCatchpoint) {
		// Forward
		if (vehData.CurrGear > 0 && vehData.Velocity < vehData.CurrGear * 2.2f &&
			controls.Rtvalf < 0.25f && controls.Ltvalf < 0.95) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, 0.37f);
		}
	}
}

void functionEngStall() {
	if (controls.Clutchvalf < 1.0f - settings.StallingThreshold && vehData.Rpm < 0.25f &&
		((vehData.Speed < vehData.CurrGear * 1.4f) || (vehData.CurrGear == 0 && vehData.Speed < 1.0f))) {
		if (VEHICLE::_IS_VEHICLE_ENGINE_ON(vehicle)) {
			VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
		}
	}
}

void functionEngDamage() {
	if (vehData.Rpm > 0.98f && controls.Accelvalf > 0.99f) {
		VEHICLE::SET_VEHICLE_ENGINE_HEALTH(vehicle, VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) - (settings.RPMDamage));
	}
}

void functionEngBrake() {
	// Save speed @ shift
	if (vehData.CurrGear < vehData.NextGear) {
		if (vehData.PrevGear <= vehData.CurrGear ||
			vehData.Velocity <= vehData.LockSpeed ||
			vehData.LockSpeed < 0.01f) {
			vehData.LockSpeed = vehData.Velocity;
		}
	}
	// Braking
	if (vehData.CurrGear > 0 &&
		vehData.Velocity > vehData.LockSpeeds[vehData.CurrGear] &&
		controls.Rtvalf < 0.1 && vehData.Rpm > 0.80) {
		float brakeForce = -0.1f * (1.0f - controls.Clutchvalf) * vehData.Rpm;
		ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(vehicle, 1, 0.0f, brakeForce, 0.0f, true, true, true, true);
	}
}

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Gearbox control
///////////////////////////////////////////////////////////////////////////////
void fakeRev() {
	float timeStep = SYSTEM::TIMESTEP();
	float accelRatio = 2 * timeStep;
	float rpmVal;
	float rpmValTemp = (prevRpm > vehData.Rpm ? (prevRpm - vehData.Rpm) : 0.0f);
	if (vehData.CurrGear == 1) {
		rpmValTemp *= 2.0f;
	}
	rpmVal =
			vehData.Rpm + // Base value
			rpmValTemp + // Keep it constant
			controls.Accelvalf * accelRatio; // Addition value, depends on delta T

	ext.SetCurrentRPM(vehicle, rpmVal);
}


void handleRPM() {
	float finalClutch = 0.0f;
	bool skip = false;

	// Game wants to shift up. Triggered at high RPM, high speed.
	// Desired result: high RPM, same gear, no more accelerating
	// Result:	Is as desired. Speed may drop a bit because of game clutch.

	if (vehData.CurrGear > 0 &&
		(vehData.CurrGear < vehData.NextGear && vehData.Speed > 2.0f)) {
		ext.SetThrottle(vehicle, 1.0f);
		fakeRev();
	}

	// Emulate previous "shift down wanted" behavior.
	if (vehData.CurrGear > 1 && vehData.Rpm < 0.4f) {
		VEHICLE::_SET_VEHICLE_ENGINE_TORQUE_MULTIPLIER(vehicle, vehData.Rpm * 2.5f);
	}

	/*
		Game doesn't rev on disengaged clutch in any gear but 1
		This workaround tries to emulate this
		Default: vehData.Clutch >= 0.6: Normal
		Default: vehData.Clutch < 0.6: Nothing happens
		Fix: Map 0.0-1.0 to 0.6-1.0 (clutchdata)
		Fix: Map 0.0-1.0 to 1.0-0.6 (control)
	*/
	if (vehData.CurrGear > 1) {
		// When pressing clutch and throttle, handle clutch and RPM
		if (controls.Clutchvalf > 0.4f && controls.Accelvalf > 0.05f &&
			!vehData.SimulatedNeutral) {
			fakeRev();
			ext.SetThrottle(vehicle, controls.Accelvalf);
			float tempVal = (1.0f - controls.Clutchvalf) * 0.4f + 0.6f;
			if (controls.Clutchvalf > 0.95) {
				tempVal = -0.5f;
			}
			finalClutch = tempVal;
			skip = true;
		}
		// Don't care about clutch slippage, just handle RPM now
		if (vehData.SimulatedNeutral) {
			fakeRev();
			ext.SetThrottle(vehicle, 1.0f); // For a fuller sound
		}
	}

	// Set the clutch depending on neutral status
	if (vehData.SimulatedNeutral || controls.Clutchvalf > 0.95) {
		/*
			To prevent a the clutch not being registered as fully pressed
			by the game. Negative values seem to work, but this amount
			differs from vehicle to vehicle, so it is at -0.1f to cover
			every case. Stronger negative values don't seem problematic.
			+Supercars seem to have a higher offset. -0.2f now,
			+Tuning the transmission messes this up, so just use -0.5f.
			+Still messes up the T20 @ Max Transmission upgrade.
		*/
		finalClutch = -0.5f;
	}
	else {
		if (!skip) {
			finalClutch = 1.0f - controls.Clutchvalf;
		}
	}
	prevRpm = vehData.Rpm;
	ext.SetClutch(vehicle, finalClutch);
}

void functionTruckLimiting() {
	// Update speed
	if (vehData.CurrGear < vehData.NextGear) {
		vehData.LockSpeed = vehData.Velocity;
		vehData.LockTruck = true;
	}

	// Limit
	if ((vehData.Velocity > vehData.LockSpeed && vehData.LockTruck) ||
		(vehData.Velocity > vehData.LockSpeed && vehData.PrevGear > vehData.CurrGear)) {
		controls.Clutchvalf = 1.0f;
		truckShiftUp = true;
	}
	else {
		truckShiftUp = false;
	}
}

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Reverse
///////////////////////////////////////////////////////////////////////////////
void functionRealReverse() {
	// Forward gear
	// Desired: Only brake
	if (vehData.CurrGear > 0) {
		// LT behavior when still: Just brake
		if (controls.Ltvalf > 0.02f && controls.Rtvalf < controls.Ltvalf &&
			vehData.Velocity <= 0.5f && vehData.Velocity >= -0.1f) {
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			ext.SetThrottleP(vehicle, 0.0f);
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
		}
		else {
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
		}
		// LT behavior when rolling back: Brake
		if (controls.Ltvalf > 0.02f && controls.Rtvalf < controls.Ltvalf &&
			vehData.Velocity < -0.1f) {
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, controls.Ltvalf);
			ext.SetThrottle(vehicle, 0.0f);
			ext.SetThrottleP(vehicle, 0.1f);
		}
		// RT behavior when rolling back: Burnout
		if (vehData.CurrGear == 1 &&
			controls.Rtvalf > 0.5f && vehData.Velocity < -1.0f) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, controls.Rtvalf);
			if (controls.Ltvalf < 0.1f) {
				VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, false);
			}
		}
	}
	// Reverse gear
	// Desired: RT reverses, LT brakes
	if (vehData.CurrGear == 0) {
		ext.SetThrottleP(vehicle, -0.1f);
		// RT behavior
		if (controls.Rtvalf > 0.02f && controls.Rtvalf > controls.Ltvalf) {
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, controls.Rtvalf);
		}
		// LT behavior when still
		if (controls.Ltvalf > 0.0f && controls.Rtvalf <= controls.Ltvalf &&
			vehData.Velocity > -0.55f && vehData.Velocity <= 0.5f) {
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
		}
		else {
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
		}
		// LT behavior when reversing
		if (controls.Ltvalf > 0.02f &&
			vehData.Velocity <= -0.5f) {
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, controls.Ltvalf);
		}
	}
}


// Forward gear: Throttle accelerates, Brake brakes (exclusive)
// Reverse gear: Throttle reverses, Brake brakes (exclusive)

void handlePedalsRealReverse(float logiThrottleVal, float logiBrakeVal) {
	if (vehData.CurrGear > 0) {
		// Throttle Pedal normal
		if (logiThrottleVal > 0.02f) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, logiThrottleVal);
		}
		// Brake Pedal normal
		if (logiBrakeVal > 0.02f) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, logiBrakeVal);
		}
		// Brake Pedal still
		if (logiBrakeVal > 0.02f && logiThrottleVal < logiBrakeVal &&
			vehData.Velocity <= 0.5f && vehData.Velocity >= -0.1f) {
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			ext.SetThrottleP(vehicle, 0.0f);
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
		}
	}

	if (vehData.CurrGear == 0) {
		// Throttle Pedal Reverse
		if (logiThrottleVal > 0.02f && logiThrottleVal > logiBrakeVal) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, logiThrottleVal);
		}
		// Brake Pedal stationary
		if (logiBrakeVal > 0.02f && logiThrottleVal <= logiBrakeVal &&
			vehData.Velocity > -0.55f && vehData.Velocity <= 0.5f) {
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
		}

		// Brake Pedal Reverse
		if (logiBrakeVal > 0.02f &&
			vehData.Velocity <= -0.5f) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, logiBrakeVal);
		}
	}
}

// Pedals behave like RT/LT
void handlePedalsDefault(float logiThrottleVal, float logiBrakeVal) {
	if (logiThrottleVal > 0.02f) {
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, logiThrottleVal);
	}
	if (logiBrakeVal > 0.02f) {
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, logiBrakeVal);
	}
}


void functionSimpleReverse() {
	// Prevent going forward in gear 0.
	if (vehData.CurrGear == 0 && vehData.Velocity > -0.55f && vehData.Velocity <= 0.5f
		&& controls.Rtvalf > 0) {
		VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
		CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
	}
	// Prevent reversing in gear >= 1.
	if (vehData.CurrGear > 0 && vehData.Velocity > -0.55f && vehData.Velocity <= 0.5f
		&& controls.Ltvalf > 0) {
		VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
		CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
	}
}

void functionAutoReverse() {
	// Go forward
	if (CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) && !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake) &&
		vehData.Velocity > -1.0f &&
		vehData.CurrGear == 0) {
		vehData.LockGears = 0x00010001;
	}

	// Reverse
	if (CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake) && !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) &&
		vehData.Velocity < 1.0f &&
		vehData.CurrGear > 0) {
		vehData.SimulatedNeutral = false;
		vehData.LockGears = 0x00000000;
	}
}

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Buttons
///////////////////////////////////////////////////////////////////////////////
void handleVehicleButtons() {
	if (!VEHICLE::_IS_VEHICLE_ENGINE_ON(vehicle) &&
	(controller.IsButtonJustPressed(controller.StringToButton(controls.ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Engine)]), buttonState) ||
		controls.IsKeyJustPressed(controls.Control[static_cast<int>(ScriptControls::KeyboardControlType::Engine)], ScriptControls::KeyboardControlType::Engine))) {
		// ||			(logiWheel.IsActive(&settings) && LogiButtonTriggered(logiWheel.GetIndex(), controls.LogiControl[(int)ScriptControls::LogiControlType::Engine])) ||
		//	controls.Rtvalf > 0.9f)) {
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, false, true);
	}

	//if (logiWheel.IsActive(&settings) && logiState != NULL) {
	//	if ((logiState->rgbButtons[controls.LogiControl[(int)ScriptControls::LogiControlType::Handbrake]] & 0x80) == 0x80 &&
	//		controls.LogiControl[(int)ScriptControls::LogiControlType::Handbrake] != -1) {
	//		//VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
	//		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, 1.0f);
	//	}
	//	if ((logiState->rgbButtons[controls.LogiControl[(int)ScriptControls::LogiControlType::Horn]] & 0x80) == 0x80 &&
	//		controls.LogiControl[(int)ScriptControls::LogiControlType::Horn] != -1) {
	//		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHorn, 1.0f);
	//	}
	//	if (LogiButtonTriggered(logiWheel.GetIndex(), controls.LogiControl[(int)ScriptControls::LogiControlType::Lights]) &&
	//		controls.LogiControl[(int)ScriptControls::LogiControlType::Lights] != -1) {
	//		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHeadlight, 1.0f);
	//	}
	//	if ((logiState->rgbButtons[controls.LogiControl[(int)ScriptControls::LogiControlType::LookBack]] & 0x80) == 0x80 &&
	//		controls.LogiControl[(int)ScriptControls::LogiControlType::LookBack] != -1) {
	//		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleLookBehind, 1.0f);
	//	}
	//	if (LogiButtonTriggered(logiWheel.GetIndex(), controls.LogiControl[(int)ScriptControls::LogiControlType::Camera]) &&
	//		controls.LogiControl[(int)ScriptControls::LogiControlType::Camera] != -1) {
	//		CONTROLS::_SET_CONTROL_NORMAL(0, ControlNextCamera, 1.0f);
	//	}
	//	if (LogiButtonTriggered(logiWheel.GetIndex(), controls.LogiControl[(int)ScriptControls::LogiControlType::RadioNext]) &&
	//		controls.LogiControl[(int)ScriptControls::LogiControlType::RadioNext] != -1) {
	//		AUDIO::SET_RADIO_TO_STATION_INDEX(AUDIO::GET_PLAYER_RADIO_STATION_INDEX() + 1);
	//	}
	//	if (LogiButtonTriggered(logiWheel.GetIndex(), controls.LogiControl[(int)ScriptControls::LogiControlType::RadioPrev]) &&
	//		controls.LogiControl[(int)ScriptControls::LogiControlType::RadioPrev] != -1) {
	//		AUDIO::SET_RADIO_TO_STATION_INDEX(AUDIO::GET_PLAYER_RADIO_STATION_INDEX() - 1);
	//	}

	//	if (LogiButtonTriggered(logiWheel.GetIndex(), controls.LogiControl[(int)ScriptControls::LogiControlType::IndicatorLeft]) &&
	//		controls.LogiControl[(int)ScriptControls::LogiControlType::IndicatorLeft] != -1) {
	//		if (!blinkerLeft) {
	//			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);	// L
	//			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, true);	// R
	//			blinkerLeft = true;
	//			blinkerRight = false;
	//			blinkerHazard = false;
	//		}
	//		else {
	//			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
	//			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
	//			blinkerLeft = false;
	//			blinkerRight = false;
	//			blinkerHazard = false;
	//		}
	//	}
	//	if (LogiButtonTriggered(logiWheel.GetIndex(), controls.LogiControl[(int)ScriptControls::LogiControlType::IndicatorRight]) &&
	//		controls.LogiControl[(int)ScriptControls::LogiControlType::IndicatorRight] != -1) {
	//		if (!blinkerRight) {
	//			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, true);	// L
	//			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);	// R
	//			blinkerLeft = false;
	//			blinkerRight = true;
	//			blinkerHazard = false;
	//		}
	//		else {
	//			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
	//			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
	//			blinkerLeft = false;
	//			blinkerRight = false;
	//			blinkerHazard = false;
	//		}
	//	}
	//	if (LogiButtonTriggered(logiWheel.GetIndex(), controls.LogiControl[(int)ScriptControls::LogiControlType::IndicatorHazard]) &&
	//		controls.LogiControl[(int)ScriptControls::LogiControlType::IndicatorHazard] != -1) {
	//		if (!blinkerHazard) {
	//			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, true);	// L
	//			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, true);	// R
	//			blinkerLeft = false;
	//			blinkerRight = false;
	//			blinkerHazard = true;
	//		}
	//		else {
	//			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
	//			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
	//			blinkerLeft = false;
	//			blinkerRight = false;
	//			blinkerHazard = false;
	//		}
	//	}

	//	if (!settings.DisableDpad) {
	//		switch (logiState->rgdwPOV[0]) {
	//		case 0:
	//			break;
	//		case 9000:
	//			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleNextRadio, 1.0f);
	//			break;
	//		case 18000:
	//			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, 1.0f);
	//			break;
	//		case 27000:
	//			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehiclePrevRadio, 1.0f);
	//			break;
	//		}
	//	}

	//}
}

///////////////////////////////////////////////////////////////////////////////
//                             Wheel functions
///////////////////////////////////////////////////////////////////////////////
// Despair: Won't work if arguments are passed?!
//void playWheelEffects() {
//	Vector3 accelVals = vehData.getAccelerationVectors(ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true));
//	Vector3 accelValsAvg = vehData.getAccelerationVectorsAverage();
//	
//	LogiPlayLeds(logiWheel.GetIndex(), vehData.Rpm, 0.66f, 0.99f);
//
//	if (settings.FFDamperStationary < settings.FFDamperMoving) {
//		settings.FFDamperMoving = settings.FFDamperStationary;
//	}
//	if (settings.FFDamperMoving < 1) {
//		settings.FFDamperMoving = 1;
//	}
//	float ratio = (float)(settings.FFDamperStationary - settings.FFDamperMoving)/settings.FFDamperMoving;
//	int damperforce = settings.FFDamperStationary - (int)(8 * ratio * ratio * vehData.Speed*vehData.Speed);
//	if (damperforce < settings.FFDamperMoving) {
//		damperforce = settings.FFDamperMoving + (int)(vehData.Speed * ratio);
//	}
//	damperforce -= (int)(ratio * accelValsAvg.y);
//	if (damperforce > settings.FFDamperStationary) {
//		damperforce = settings.FFDamperStationary;
//	}
//	LogiPlayDamperForce(logiWheel.GetIndex(), damperforce);
//
//	int constantForce = (int)(-settings.FFPhysics*((3 * accelValsAvg.x + 2 * accelVals.x)));
//	LogiPlayConstantForce(logiWheel.GetIndex(), constantForce);
//	
//	int centerForcePercentage = (int)(settings.FFCenterSpring * accelValsAvg.y);
//	if (centerForcePercentage < 0) {
//		centerForcePercentage = 0;
//	}
//
//	LogiPlaySpringForce(
//		logiWheel.GetIndex(),
//		0,
//		(int)(settings.FFCenterSpring*vehData.Speed + centerForcePercentage),
//		(int)(settings.FFCenterSpring*vehData.Speed + centerForcePercentage));
//	
//	if (!VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle) && ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(vehicle) > 1.25f) {
//		LogiPlayCarAirborne(logiWheel.GetIndex());
//	}
//	else if (LogiIsPlaying(logiWheel.GetIndex(), LOGI_FORCE_CAR_AIRBORNE)) {
//		LogiStopCarAirborne(logiWheel.GetIndex());
//	}
//}

// Since playing back is f'd - This too, will be in the main class
//void resetWheelFeedback(int index) {
//	LogiStopSpringForce(index);
//	LogiStopConstantForce(index);
//	LogiStopDamperForce(index);
//	LogiStopDirtRoadEffect(index);
//	LogiStopBumpyRoadEffect(index);
//	LogiStopSlipperyRoadEffect(index);
//	LogiStopSurfaceEffect(index);
//	LogiStopCarAirborne(index);
//	LogiStopSoftstopForce(index);
//}

void crossScriptComms() {
	// Other scripts. 0 = nothing, 1 = Shift up, 2 = Shift down
	if (vehData.CurrGear < vehData.NextGear || truckShiftUp) {
		DECORATOR::DECOR_SET_INT(vehicle, "hunt_score", 1);
	}
	else if (vehData.CurrGear > 1 && vehData.Rpm < 0.4f) {
		DECORATOR::DECOR_SET_INT(vehicle, "hunt_score", 2);
	}
	else if (vehData.CurrGear == vehData.NextGear) {
		DECORATOR::DECOR_SET_INT(vehicle, "hunt_score", 0);
	}

	if (vehData.SimulatedNeutral) {
		DECORATOR::DECOR_SET_INT(vehicle, "hunt_weapon", 1);
	}
	else {
		DECORATOR::DECOR_SET_INT(vehicle, "hunt_weapon", 0);
	}
}
