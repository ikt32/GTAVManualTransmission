#include <string>
#include <sstream>
#include <iomanip>

#include "../../ScriptHookV_SDK/inc/natives.h"
#include "../../ScriptHookV_SDK/inc/enums.h"
#include "../../ScriptHookV_SDK/inc/main.h"

#include "script.h"

#include "ScriptSettings.hpp"
#include "ScriptControls.hpp"
#include "Logger.hpp"
#include "VehicleData.hpp"
#include "MemoryPatcher.hpp"

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

bool patched = false;
bool simpleBike = false;
int prevNotification = 0;
ScriptControls::InputDevices prevInput;

float prevRpm;

// This gonna be refactored into vehData or LogiInput somehow
bool blinkerLeft = false;
bool blinkerRight = false;
bool blinkerHazard = false;
bool truckShiftUp = false;

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

	controls.UpdateValues(prevInput);

	if (controls.ButtonJustPressed(ScriptControls::KeyboardControlType::Toggle) ||
		controls.ButtonHeld(ScriptControls::ControllerControlType::Toggle) ||
		controls.ButtonJustPressed(ScriptControls::WheelControlType::Toggle)) {
		toggleManual();
	}

	if (settings.CrossScript) {
		crossScriptComms();
	}

	if (settings.Debug) {
		showDebugInfo();
	}


	if (!VEHICLE::IS_VEHICLE_DRIVEABLE(vehicle, false) &&
		VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) < -100.0f) {
		return;
	}

	//TODO THINGS
	if (!settings.EnableManual &&
		settings.WheelWithoutManual &&
		//controls.WheelDI != nullptr &&
		controls.WheelDI.IsConnected()) {
		updateLastInputDevice();
		handleVehicleButtons();
		handlePedalsDefault(controls.ThrottleVal, controls.BrakeVal);
		doWheelSteering();
		playWheelEffects(
		vehData.Speed,
		vehData.getAccelerationVectors(ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true)),
		vehData.getAccelerationVectorsAverage(),
		settings,
		false);
	}

	if (!settings.EnableManual) {
		return;
	}



	///////////////////////////////////////////////////////////////////////////
	// Active whenever Manual is enabled from here
	///////////////////////////////////////////////////////////////////////////
	updateLastInputDevice();
	handleVehicleButtons();

	if (//controls.WheelDI != nullptr &&
		controls.WheelDI.IsConnected()) {
		doWheelSteering();
		playWheelEffects(
			vehData.Speed,
			vehData.getAccelerationVectors(ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true)),
			vehData.getAccelerationVectorsAverage(),
			settings,
			false);
	}
	

	if (controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ToggleH) ||
		controls.ButtonJustPressed(ScriptControls::WheelControlType::ToggleH)) {
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

	if (settings.UITips) {
		if (vehData.SimulatedNeutral) {
			showText(settings.UITips_X,
			         settings.UITips_Y,
			         settings.UITips_Size,
			         "N");
		}
		else {
			showText(settings.UITips_X,
			         settings.UITips_Y,
			         settings.UITips_Size,
			         const_cast<char *>(std::to_string(vehData.CurrGear).c_str()));
		}
	}

	///////////////////////////////////////////////////////////////////////////
	//                            Patching
	///////////////////////////////////////////////////////////////////////////
	if (!patched && settings.EnableManual) {
		patched = MemoryPatcher::PatchInstructions();
	}
	///////////////////////////////////////////////////////////////////////////
	// Actual mod operations
	///////////////////////////////////////////////////////////////////////////

	// Reverse behavior
	// For bikes, do this automatically.
	if (vehData.IsBike) {
		functionAutoReverse();
		handlePedalsDefault(
			controls.ThrottleVal,
			controls.BrakeVal);
	}
	else {
		// New reverse: Reverse with throttle
		if (settings.RealReverse) {
			functionRealReverse();
			if (prevInput == ScriptControls::InputDevices::Wheel) {
				handlePedalsRealReverse(
					controls.ThrottleVal,
					controls.BrakeVal);
			}
		}
		// Reversing behavior: just block the direction you don't wanna go to
		else {
			functionSimpleReverse();
			if (prevInput == ScriptControls::InputDevices::Wheel) {
				handlePedalsDefault(
					controls.ThrottleVal,
					controls.BrakeVal);
			}
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
		functionHShiftWheel();
		functionHShiftKeyboard();
	}
	else {
		functionSShift();
	}

	// Finally, update memory each loop
	handleRPM();
	ext.SetGears(vehicle, vehData.LockGears);
}

///////////////////////////////////////////////////////////////////////////////
//                              Script entry
///////////////////////////////////////////////////////////////////////////////

void main() {
	//settings.Read(&controls);
	reInit();
	logger.Write("Settings read");
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
	showText(0.01f, 0.5f, 0.4f, infos.str().c_str());

	std::stringstream throttleDisplay;
	throttleDisplay << "ThrottleVal: " << controls.ThrottleVal;
	std::stringstream brakeDisplay;
	brakeDisplay << "Brake Value: " << controls.BrakeVal;
	std::stringstream clutchDisplay;
	clutchDisplay << "ClutchValue: " << controls.ClutchVal;
	std::stringstream clutcheDisplay;
	clutcheDisplay << "Disabled: " << (controls.ClutchDisable ? "Y" : "N");

	showText(0.85, 0.04, 0.4, throttleDisplay.str().c_str());
	showText(0.85, 0.08, 0.4, brakeDisplay.str().c_str());
	showText(0.85, 0.12, 0.4, clutchDisplay.str().c_str());
	showText(0.70, 0.12, 0.4, clutcheDisplay.str().c_str());


	if (settings.WheelEnabled) {
		std::stringstream dinputDisplay;
		dinputDisplay << "Wheel Avail: " << controls.WheelDI.IsConnected();
		showText(0.85, 0.20, 0.4, dinputDisplay.str().c_str());
		std::stringstream steerDisplay;
		steerDisplay << "SteerValue: " << controls.SteerVal;
		showText(0.85, 0.16, 0.4, steerDisplay.str().c_str());
	}
	


	/*for (int i = 0; i < MAX_RGBBUTTONS; i++) {
		if (controls.wheelState->rgbButtons[i]) {
			showText(0.4, 0.4, 2.0, std::to_string(i).c_str());
		}
	}*/
}

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Mod control
///////////////////////////////////////////////////////////////////////////////

void reInit() {
	settings.Read(&controls);
	vehData.LockGears = 0x00010001;
	vehData.SimulatedNeutral = settings.DefaultNeutral;
	if (settings.WheelEnabled) {
		if (//controls.WheelDI != nullptr &&
			!controls.WheelDI.IsConnected())
			controls.InitWheel();
	}
}

void reset() {
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
		//LogiSteeringShutdown();
	}
	reInit();
}

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Shifting
///////////////////////////////////////////////////////////////////////////////

void shiftTo(int gear, bool autoClutch) {
	if (autoClutch) {
		controls.ClutchVal = 1.0f;
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
		if (controls.ClutchVal > 1.0f - settings.ClutchCatchpoint) {
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
		if (controls.ButtonJustPressed(static_cast<ScriptControls::KeyboardControlType>(i))) {
			functionHShiftTo(i);
		}
	}
	if (controls.ButtonJustPressed(ScriptControls::KeyboardControlType::HN) &&
		!vehData.NoClutch) {
		vehData.SimulatedNeutral = !vehData.SimulatedNeutral;
	}
}

void functionHShiftWheel() {
	// highest shifter button == 6
	int clamp = 6;
	if (vehData.TopGear <= clamp) {
		clamp = vehData.TopGear;
	}
	for (uint8_t i = 0; i <= clamp; i++) {
		if (controls.ButtonJustPressed(static_cast<ScriptControls::WheelControlType>(i))) {
			functionHShiftTo(i);
		}
	}
	if (controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H1)) ||
		controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H2)) ||
		controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H3)) ||
		controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H4)) ||
		controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H5)) ||
		controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H6))
	) {
		if (settings.ClutchShifting &&
			settings.EngDamage &&
			!vehData.NoClutch) {
			if (controls.ClutchVal < 1.0 - settings.ClutchCatchpoint) {
				VEHICLE::SET_VEHICLE_ENGINE_HEALTH(
					vehicle,
					VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) - settings.MisshiftDamage / 10);
			}
		}
		vehData.SimulatedNeutral = !vehData.NoClutch;
	}

	if (controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::HR))) {
		shiftTo(1, true);
		vehData.SimulatedNeutral = !vehData.NoClutch;
	}
}


void functionSShift() {
	// Shift up
	if (controls.ButtonJustPressed(ScriptControls::ControllerControlType::ShiftUp) ||
		controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ShiftUp) ||
		controls.ButtonJustPressed(ScriptControls::WheelControlType::ShiftUp)) {
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

	if (controls.ButtonJustPressed(ScriptControls::ControllerControlType::ShiftDown) ||
		controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ShiftDown) ||
		controls.ButtonJustPressed(ScriptControls::WheelControlType::ShiftDown)) {
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
	if (controls.ClutchVal < 1.0f - settings.ClutchCatchpoint) {
		// Forward
		if (vehData.CurrGear > 0 && vehData.Velocity < vehData.CurrGear * 2.2f &&
		                            controls.ThrottleVal < 0.25f && controls.BrakeVal < 0.95) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, 0.37f);
		}
	}
}

void functionEngStall() {
	if (controls.ClutchVal < 1.0f - settings.StallingThreshold &&
		vehData.Rpm < 0.25f &&
		((vehData.Speed < vehData.CurrGear * 1.4f) || (vehData.CurrGear == 0 && vehData.Speed < 1.0f)) &&
		VEHICLE::_IS_VEHICLE_ENGINE_ON(vehicle)) {
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
	}
}

void functionEngDamage() {
	if (vehData.Rpm > 0.98f && controls.AccelValGTAf > 0.99f) {
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
		controls.ThrottleVal < 0.1 && vehData.Rpm > 0.80) {
		float brakeForce = -0.1f * (1.0f - controls.ClutchVal) * vehData.Rpm;
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
			controls.AccelValGTAf * accelRatio; // Addition value, depends on delta T

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
		if (controls.ClutchVal > 0.4f && controls.AccelValGTAf > 0.05f &&
		                                 !vehData.SimulatedNeutral) {
			fakeRev();
			ext.SetThrottle(vehicle, controls.AccelValGTAf);
			float tempVal = (1.0f - controls.ClutchVal) * 0.4f + 0.6f;
			if (controls.ClutchVal > 0.95) {
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
	if (vehData.SimulatedNeutral || controls.ClutchVal > 0.95) {
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
			finalClutch = 1.0f - controls.ClutchVal;
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
		controls.ClutchVal = 1.0f;
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
		if (controls.BrakeVal > 0.02f && controls.ThrottleVal < controls.BrakeVal &&
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
		if (controls.BrakeVal > 0.02f && controls.ThrottleVal < controls.BrakeVal &&
		                                 vehData.Velocity < -0.1f) {
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, controls.BrakeVal);
			ext.SetThrottle(vehicle, 0.0f);
			ext.SetThrottleP(vehicle, 0.1f);
		}
		// RT behavior when rolling back: Burnout
		if (vehData.CurrGear == 1 &&
			controls.ThrottleVal > 0.5f && vehData.Velocity < -1.0f) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, controls.ThrottleVal);
			if (controls.BrakeVal < 0.1f) {
				VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, false);
			}
		}
	}
	// Reverse gear
	// Desired: RT reverses, LT brakes
	if (vehData.CurrGear == 0) {
		ext.SetThrottleP(vehicle, -0.1f);
		// RT behavior
		if (controls.ThrottleVal > 0.02f && controls.ThrottleVal > controls.BrakeVal) {
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, controls.ThrottleVal);
		}
		// LT behavior when still
		if (controls.BrakeVal > 0.0f && controls.ThrottleVal <= controls.BrakeVal &&
		                                vehData.Velocity > -0.55f && vehData.Velocity <= 0.5f) {
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
		}
		else {
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
		}
		// LT behavior when reversing
		if (controls.BrakeVal > 0.02f &&
			vehData.Velocity <= -0.5f) {
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, controls.BrakeVal);
		}
	}
}


// Forward gear: Throttle accelerates, Brake brakes (exclusive)
// Reverse gear: Throttle reverses, Brake brakes (exclusive)

void handlePedalsRealReverse(float wheelThrottleVal, float wheelBrakeVal) {
	if (vehData.CurrGear > 0) {
		// Throttle Pedal normal
		if (wheelThrottleVal > 0.02f) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, wheelThrottleVal);
		}
		// Brake Pedal normal
		if (wheelBrakeVal > 0.02f) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, wheelBrakeVal);
		}
		// Brake Pedal still
		if (wheelBrakeVal > 0.02f && wheelThrottleVal < wheelBrakeVal &&
		                             vehData.Velocity <= 0.5f && vehData.Velocity >= -0.1f) {
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			ext.SetThrottleP(vehicle, 0.0f);
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
		}
	}

	if (vehData.CurrGear == 0) {
		// Throttle Pedal Reverse
		if (wheelThrottleVal > 0.02f && wheelThrottleVal > wheelBrakeVal) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, wheelThrottleVal);
		}
		// Brake Pedal stationary
		if (wheelBrakeVal > 0.02f && wheelThrottleVal <= wheelBrakeVal &&
		                             vehData.Velocity > -0.55f && vehData.Velocity <= 0.5f) {
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
		}

		// Brake Pedal Reverse
		if (wheelBrakeVal > 0.02f &&
			vehData.Velocity <= -0.5f) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, wheelBrakeVal);
		}
	}
}

// Pedals behave like RT/LT
void handlePedalsDefault(float wheelThrottleVal, float wheelBrakeVal) {
	if (wheelThrottleVal > 0.02f) {
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, wheelThrottleVal);
	}
	if (wheelBrakeVal > 0.02f) {
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, wheelBrakeVal);
	}
}


void functionSimpleReverse() {
	// Prevent going forward in gear 0.
	if (vehData.CurrGear == 0 && vehData.Velocity > -0.55f && vehData.Velocity <= 0.5f
	                             && controls.ThrottleVal > 0) {
		VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
		CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
	}
	// Prevent reversing in gear >= 1.
	if (vehData.CurrGear > 0 && vehData.Velocity > -0.55f && vehData.Velocity <= 0.5f
	                            && controls.BrakeVal > 0) {
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
		controls.ButtonJustPressed(ScriptControls::ControllerControlType::Engine) ||
		controls.ButtonJustPressed(ScriptControls::KeyboardControlType::Engine) ||
		controls.ButtonJustPressed(ScriptControls::WheelControlType::Engine) ||
		controls.ThrottleVal > 0.98f && controls.ClutchVal > settings.ClutchCatchpoint) {
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, false, true);
	}

	if (controls.ButtonIn(ScriptControls::WheelControlType::Handbrake)) {
		//VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, 1.0f);
		showText(0.5, 0.5, 5.0, "E");
	}
	if (controls.ButtonIn(ScriptControls::WheelControlType::Horn)) {
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHorn, 1.0f);
	}
	if (controls.ButtonJustPressed(ScriptControls::WheelControlType::Lights)) {
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHeadlight, 1.0f);
	}
	if (controls.ButtonIn(ScriptControls::WheelControlType::LookBack)) {
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleLookBehind, 1.0f);
	}
	if (controls.ButtonJustPressed(ScriptControls::WheelControlType::Camera)) {
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlNextCamera, 1.0f);
	}
	if (controls.ButtonJustPressed(ScriptControls::WheelControlType::RadioNext)) {
		AUDIO::SET_RADIO_TO_STATION_INDEX(AUDIO::GET_PLAYER_RADIO_STATION_INDEX() + 1);
	}
	if (controls.ButtonJustPressed(ScriptControls::WheelControlType::RadioPrev)) {
		AUDIO::SET_RADIO_TO_STATION_INDEX(AUDIO::GET_PLAYER_RADIO_STATION_INDEX() - 1);
	}

	if (controls.ButtonJustPressed(ScriptControls::WheelControlType::IndicatorLeft)) {
		if (!blinkerLeft) {
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false); // L
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, true); // R
			blinkerLeft = true;
			blinkerRight = false;
			blinkerHazard = false;
		}
		else {
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
			blinkerLeft = false;
			blinkerRight = false;
			blinkerHazard = false;
		}
	}
	if (controls.ButtonJustPressed(ScriptControls::WheelControlType::IndicatorRight)) {
		if (!blinkerRight) {
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, true); // L
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false); // R
			blinkerLeft = false;
			blinkerRight = true;
			blinkerHazard = false;
		}
		else {
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
			blinkerLeft = false;
			blinkerRight = false;
			blinkerHazard = false;
		}
	}
	if (controls.ButtonJustPressed(ScriptControls::WheelControlType::IndicatorHazard)) {
		if (!blinkerHazard) {
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, true); // L
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, true); // R
			blinkerLeft = false;
			blinkerRight = false;
			blinkerHazard = true;
		}
		else {
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
			blinkerLeft = false;
			blinkerRight = false;
			blinkerHazard = false;
		}
	}
}

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

void updateLastInputDevice() {
	if (prevInput != controls.GetLastInputDevice(prevInput)) {
		prevInput = controls.GetLastInputDevice(prevInput);
		switch (prevInput) {
			case ScriptControls::Keyboard:
				showNotification("Switched to keyboard/mouse");
				break;
			case ScriptControls::Controller: // Controller
				if (settings.Hshifter) {
					showNotification("Switched to controller\nSequential re-initiated");
					settings.Hshifter = false;
					settings.Save();
				}
				else {
					showNotification("Switched to controller");
				}
				break;
			case ScriptControls::Wheel:
				//CONTROLS::DISABLE_ALL_CONTROL_ACTIONS
				showNotification("Switched to wheel");
				break;
			default:
				break;
		}
	}
}

void doWheelSteering() {
	if (prevInput == ScriptControls::InputDevices::Wheel) {
		ext.SetSteeringAngle1(vehicle, (controls.SteerVal - 32768) / (-32768.0f));
		int additionalOffset = 2560;
		float antiDeadzoned;
		antiDeadzoned = (controls.SteerVal - 32768) / 32768.0f;
		if (//logiSteeringWheelPos > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
			controls.SteerVal <= 0) {
			antiDeadzoned = (controls.SteerVal - 32768 - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE - additionalOffset) /
					(32768.0f + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE + additionalOffset);
		}
		if (//logiSteeringWheelPos > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
			controls.SteerVal > 0) {
			antiDeadzoned = (controls.SteerVal - 32768 + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE + additionalOffset) /
					(32768.0f + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE + additionalOffset);
		}
		// Gotta find a way to make this no-delay
		CONTROLS::_SET_CONTROL_NORMAL(27, ControlVehicleMoveLeftRight, antiDeadzoned);
		//VEHICLE::SET_VEHICLE_STEER_BIAS(vehicle, -steerVal_);
	}
}

void playWheelEffects(	float speed, Vector3 accelVals, Vector3 accelValsAvg, ScriptSettings& settings, bool airborne) {
	if (prevInput != ScriptControls::Wheel) {
		return;
	}

	if (!controls.WheelDI.IsConnected()) {
		return;
	}

	int constantForce = 100 * static_cast<int>(settings.FFPhysics * ((3 * accelValsAvg.x + 2 * accelVals.x)));
	std::stringstream forceDisplay;
	forceDisplay << "ConstForce: " << constantForce << std::endl;
	showText(0.85, 0.28, 0.4, forceDisplay.str().c_str());

	// targetSpeed in m/s
	// targetSpeed is the speed at which the damperForce is at minimum
	// damperForce is maximum at 0 and keeps decreasing
	float adjustRatio = static_cast<float>(settings.DamperMax) / static_cast<float>(settings.TargetSpeed);
	int damperForce = settings.DamperMax - static_cast<int>(speed * adjustRatio);
	if (damperForce < settings.DamperMin) {
		damperForce = settings.DamperMin;
	}

	// Holy batman readability = 0
	auto steerSpeed = controls.WheelDI.GetAxisSpeed(
		controls.WheelDI.StringToAxis(
			controls.WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Throttle)]
		)
	)/20;

	std::stringstream steerDisplay;
	steerDisplay << "SteerSpeed: " << steerSpeed << std::endl;
	showText(0.85, 0.24, 0.4, steerDisplay.str().c_str());

	if (airborne) {
		constantForce = 0;
		damperForce = settings.DamperMin;
	}

	int totalForce = static_cast<int>(steerSpeed * damperForce * 0.1) - constantForce;

	controls.WheelDI.SetConstantForce(totalForce);

	std::stringstream damperF;
	damperF << "damperF: " << damperForce << std::endl;
	showText(0.85, 0.32, 0.4, damperF.str().c_str());
}
