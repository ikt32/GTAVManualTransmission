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
#include "Util.hpp"

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
int prevNotification = 0;
ScriptControls::InputDevices prevInput;

float prevRpm;
int prevExtShift = 0;

// This gonna be refactored into vehData or LogiInput somehow
bool blinkerLeft = false;
bool blinkerRight = false;
bool blinkerHazard = false;
int blinkerTicks = 0;

bool truckShiftUp = false;

enum Shifter {
	Sequential = 0,
	HPattern = 1,
	Automatic = 2
};

void update() {
	///////////////////////////////////////////////////////////////////////////
	//                           Gathering data
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

	vehData.UpdateValues(ext, vehicle);
	vehData.LockGear = (0xFFFF0000 & vehData.LockGears) >> 16;

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

	controls.UpdateValues(prevInput, settings.ShiftMode == Automatic);

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

	if (!settings.EnableManual &&
		settings.WheelWithoutManual &&
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
		!VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle) && ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(vehicle) > 1.25f);
	}

	if (!settings.EnableManual) {
		return;
	}
	

	///////////////////////////////////////////////////////////////////////////
	//          Active whenever Manual is enabled from here
	//						UI stuff and whatever
	///////////////////////////////////////////////////////////////////////////
	updateLastInputDevice();
	handleVehicleButtons();

	if (controls.WheelDI.IsConnected()) {
		doWheelSteering();
		playWheelEffects(
			vehData.Speed,
			vehData.getAccelerationVectors(ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true)),
			vehData.getAccelerationVectorsAverage(),
			settings,
			!VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle) && ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(vehicle) > 1.25f);
	}
	

	if (controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ToggleH) ||
		controls.ButtonJustPressed(ScriptControls::WheelControlType::ToggleH)) {
		cycleShiftMode();
	}

	if (settings.UITips) {
		if (vehData.SimulatedNeutral) {
			showText(settings.UITips_X,
			         settings.UITips_Y,
			         settings.UITips_Size,
			         "N");
		}
		else if (vehData.CurrGear == 0 && !settings.UITips_OnlyNeutral) {
			showText(settings.UITips_X,
					 settings.UITips_Y,
					 settings.UITips_Size,
					 "R");
		}
		else if (!settings.UITips_OnlyNeutral) {
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
	
	// Hill-start effect, gravity and stuff
	// Courtesy of XMOD
	if (settings.HillBrakeWorkaround) {
		if (vehData.CurrGear > 0 && controls.ThrottleVal < 0.2 && !controls.BrakeVal && vehData.Speed < 2.0f)
		{
			if (vehData.Pitch < 0 || controls.ClutchVal)
				ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
					vehicle, 1, 0.0f, -1 * (vehData.Pitch / 150.0f) * 1.1f, 0.0f, true, true, true, true);

			if (vehData.Pitch > 10.0f || controls.ClutchVal)
				ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
					vehicle, 1, 0.0f, -1 * (vehData.Pitch / 90.0f) * 0.35f, 0.0f, true, true, true, true);
		}
	}

	// Reverse behavior
	// For bikes, do this automatically.
	if (vehData.IsBike && settings.SimpleBike) {
		functionAutoReverse();
		handlePedalsDefault(
			controls.ThrottleVal,
			controls.BrakeVal);
	}
	else {
		functionRealReverse();
		if (prevInput == ScriptControls::InputDevices::Wheel) {
			handlePedalsRealReverse(
				controls.ThrottleVal,
				controls.BrakeVal);
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
	if (settings.EngDamage &&
		!vehData.NoClutch) {
		functionEngDamage();
	}

	if (!vehData.SimulatedNeutral && !(settings.SimpleBike && vehData.IsBike) && !vehData.NoClutch) {
		// Stalling
		if (settings.EngStall && settings.ShiftMode != Automatic) {
			functionEngStall();
		}

		// Simulate "catch point"
		// When the clutch "grabs" and the car starts moving without input
		if (settings.ClutchCatching) {
			functionClutchCatch();
		}
	}

	// Manual shifting
	if (settings.ShiftMode == 1) {
		functionHShiftWheel();
		functionHShiftKeyboard();
	}
	else if (settings.ShiftMode == 0){
		functionSShift();
	}
	else { // Automatic
		functionAShift();
	}

	if (settings.AutoLookBack) {
		functionAutoLookback();
	}

	// Finally, update memory each loop
	handleRPM();
	ext.SetGears(vehicle, vehData.LockGears);
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
	std::stringstream ssRPM;
	ssRPM << "RPM: " << std::setprecision(3) << vehData.Rpm;
	showText(0.01f, 0.300f, 0.4f, ssRPM.str().c_str());

	std::stringstream ssCurrGear;
	ssCurrGear << "CurrGear: " << vehData.CurrGear;
	showText(0.01f, 0.325f, 0.4f, ssCurrGear.str().c_str());

	std::stringstream ssNextGear;
	ssNextGear << "NextGear: " << vehData.NextGear;
	showText(0.01f, 0.350f, 0.4f, ssNextGear.str().c_str());

	std::stringstream ssClutch;
	ssClutch << "Clutch: " << std::setprecision(3) << vehData.Clutch;
	showText(0.01f, 0.375f, 0.4f, ssClutch.str().c_str());

	std::stringstream ssThrottle;
	ssThrottle << "Throttle: " << std::setprecision(3) << vehData.Throttle;
	showText(0.01f, 0.400f, 0.4f, ssThrottle.str().c_str());

	std::stringstream ssTurbo;
	ssTurbo << "Turbo: " << std::setprecision(3) << vehData.Turbo;
	showText(0.01f, 0.425f, 0.4f, ssTurbo.str().c_str());

	std::stringstream ssAddress;
	ssAddress << "Address: " << std::hex << vehData.Address;
	showText(0.01f, 0.450f, 0.4f, ssAddress.str().c_str());

	std::stringstream ssEnabled;
	ssEnabled << "Mod " << (settings.EnableManual ? "Enabled" : "Disabled");
	showText(0.01f, 0.475f, 0.4f, ssEnabled.str().c_str());

	/*std::stringstream ssWheelAddr;
	ssWheelAddr << "WheelAddress: " << std::hex << ext.GetWheelsPtr(vehicle);
	showText(0.01f, 0.500, 0.4f, ssWheelAddr.str().c_str());*/

	std::stringstream ssThrottleInput;
	ssThrottleInput << "ThrottleVal: " << controls.ThrottleVal;

	std::stringstream ssBrakeInput;
	ssBrakeInput << "Brake Value: " << controls.BrakeVal;

	std::stringstream ssClutchInput;
	ssClutchInput << "ClutchValue: " << controls.ClutchVal;

	std::stringstream ssClutchDisable;
	ssClutchDisable << (controls.ClutchDisable ? "Disabled:" : "");

	showText(0.85, 0.050, 0.4, ssThrottleInput.str().c_str());
	showText(0.85, 0.075, 0.4, ssBrakeInput.str().c_str());
	showText(0.85, 0.100, 0.4, ssClutchInput.str().c_str());
	showText(0.795, 0.100, 0.4, ssClutchDisable.str().c_str());

	if (settings.WheelEnabled) {
		std::stringstream dinputDisplay;
		dinputDisplay << "Wheel Avail: " << controls.WheelDI.IsConnected();
		showText(0.85, 0.150, 0.4, dinputDisplay.str().c_str());
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

	// External shifting
	int currExtShift = DECORATOR::DECOR_GET_INT(vehicle, "hunt_chal_weapon");
	if (prevExtShift != currExtShift && currExtShift > 0) {
		// 1 Seq, 2 H, 3 Auto
		setShiftMode(currExtShift - 1);
	}
	prevExtShift = currExtShift;
}

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Mod control
///////////////////////////////////////////////////////////////////////////////

void reInit() {
	settings.Read(&controls);
	vehData.LockGears = 0x00010001;
	vehData.SimulatedNeutral = settings.DefaultNeutral;
	if (settings.WheelEnabled) {
		controls.InitWheel();
	}
}

void reset() {
	prevVehicle = 0;
	if (patched) {
		patched = !MemoryPatcher::RestoreInstructions();
	}
	if (controls.WheelDI.IsConnected()) {
		controls.WheelDI.SetConstantForce(0);
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

void updateLastInputDevice() {
	if (prevInput != controls.GetLastInputDevice(prevInput)) {
		prevInput = controls.GetLastInputDevice(prevInput);
		// ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
		switch (prevInput) {
			case ScriptControls::Keyboard:
				showNotification("Switched to keyboard/mouse");
				break;
			case ScriptControls::Controller: // Controller
				if (settings.ShiftMode == 1) {
					showNotification("Switched to controller\nSequential re-initiated");
					settings.ShiftMode = 0;
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
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Shifting
///////////////////////////////////////////////////////////////////////////////

void setShiftMode(int shiftMode) {
	if (shiftMode > 2 || shiftMode < 0)
		return;

	if (settings.ShiftMode == HPattern  && vehData.IsBike) {
		settings.ShiftMode = Automatic;
	}

	if (settings.ShiftMode == Sequential && vehData.CurrGear > 1) {
		vehData.SimulatedNeutral = false;
	}

	std::stringstream message;
	std::string mode;
	// ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
	switch (settings.ShiftMode) {
		case Sequential: mode = "Sequential";
			break;
		case HPattern: mode = "H-Pattern";
			break;
		case Automatic: mode = "Automatic";
			break;
	}
	message << "Mode: " << mode;
	showNotification(const_cast<char *>(message.str().c_str()));
}

void cycleShiftMode() {
	settings.ShiftMode++;
	if (settings.ShiftMode > 2) {
		settings.ShiftMode = 0;
	}

	setShiftMode(settings.ShiftMode);
	settings.Save();
}

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
			if (settings.EngDamage &&
				!vehData.NoClutch) {
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

void functionAShift() {
	// Shift up
	if (controls.ButtonJustPressed(ScriptControls::ControllerControlType::ShiftUp) ||
		controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ShiftUp) ||
		controls.ButtonJustPressed(ScriptControls::WheelControlType::ShiftUp)) {
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
	}

	// Shift down

	if (controls.ButtonJustPressed(ScriptControls::ControllerControlType::ShiftDown) ||
		controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ShiftDown) ||
		controls.ButtonJustPressed(ScriptControls::WheelControlType::ShiftDown)) {
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
	}

	// Shift up
	if (vehData.CurrGear > 0 &&
		(vehData.CurrGear < vehData.NextGear && vehData.Speed > 2.0f)) {
		shiftTo(vehData.CurrGear + 1, true);
	}

	// Shift down
	if ((vehData.CurrGear > 1 && vehData.Rpm < 0.4f) ||
		(vehData.CurrGear > 1 && vehData.Rpm < 0.5f) && vehData.Throttle > 0.95f) {
		shiftTo(vehData.CurrGear - 1, true);
	}
}

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Gearbox features
///////////////////////////////////////////////////////////////////////////////

void functionClutchCatch() {
	if (controls.ClutchVal < 1.0f - settings.ClutchCatchpoint) {
		// Forward
		if (vehData.CurrGear > 0 && vehData.Velocity < vehData.CurrGear * 2.2f &&
		    controls.ThrottleVal < 0.25f && controls.BrakeVal < 0.95) {
			if (VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle)) {
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, 0.37f);
			}
			else {
				if (vehData.Rpm < 0.3f) {
					CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, 0.28f);
				} else {
					ext.SetCurrentRPM(vehicle, 0.28f);
				}
			}
		}

		// Reverse
		if (vehData.CurrGear == 0 &&
			controls.ThrottleVal < 0.25f && controls.BrakeVal < 0.95) {
			if (vehData.Velocity < -2.2f) {
				controls.ClutchVal = 1.0f;
				//CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, 0.20f);
				//return;
			}
			if (VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle)) {
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, 0.37f);
			}
			else if (vehData.Rpm < 0.3f) {
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, 0.28f);
			}
		}
	}
}

void functionEngStall() {
	if (controls.ClutchVal < 1.0f - settings.StallingThreshold &&
		vehData.Rpm < 0.21f &&
		((vehData.Speed < vehData.CurrGear * 1.4f) || (vehData.CurrGear == 0 && vehData.Speed < 1.0f)) &&
		VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
	}
}

void functionEngDamage() {
	if (vehData.Rpm > 0.98f &&
		controls.AccelValGTAf > 0.99f) {
		VEHICLE::SET_VEHICLE_ENGINE_HEALTH(
			vehicle, VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) - (settings.RPMDamage));
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
		float brakeForce = -0.20f * (1.0f - controls.ClutchVal) * vehData.Rpm;
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
		// Don't artificially lower this any more
		//VEHICLE::_SET_VEHICLE_ENGINE_TORQUE_MULTIPLIER(vehicle, vehData.Rpm * 2.5f);
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
//                   Mod functions: Reverse/Pedal handling
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
		if (controls.ThrottleVal > 0.01f && controls.ThrottleVal > controls.BrakeVal) {
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, controls.ThrottleVal);
		}
		// LT behavior when still
		if (controls.BrakeVal > 0.01f && controls.ThrottleVal <= controls.BrakeVal &&
		                                vehData.Velocity > -0.55f && vehData.Velocity <= 0.5f) {
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
		}
		else {
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
		}
		// LT behavior when reversing
		if (controls.BrakeVal > 0.01f &&
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

void functionAutoReverse() {
	// Go forward
	if (CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) && 
		!CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake) &&
	    vehData.Velocity > -1.0f &&
	    vehData.CurrGear == 0) {
		vehData.LockGears = 0x00010001;
	}

	// Reverse
	if (CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake) && 
		!CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) &&
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
	if  (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) &&
		(controls.ButtonJustPressed(ScriptControls::ControllerControlType::Engine) ||
		controls.ButtonJustPressed(ScriptControls::KeyboardControlType::Engine) ||
		controls.ButtonJustPressed(ScriptControls::WheelControlType::Engine) ||
		controls.ThrottleVal > 0.98f && controls.ClutchVal > settings.ClutchCatchpoint)) {
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, false, true);
	}
	if  (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) &&
		(controls.ButtonJustPressed(ScriptControls::ControllerControlType::Engine) ||
		controls.ButtonJustPressed(ScriptControls::KeyboardControlType::Engine) ||
		controls.ButtonJustPressed(ScriptControls::WheelControlType::Engine))) {
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
	}

	if (!controls.WheelDI.IsConnected() ||
		controls.WheelDI.NoFeedback ||
		prevInput != ScriptControls::Wheel) {
		return;
	}

	if (controls.ButtonIn(ScriptControls::WheelControlType::Handbrake)) {
		//VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, 1.0f);
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
			blinkerTicks = 1;
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false); // L
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, true); // R
			blinkerLeft = true;
			blinkerRight = false;
			blinkerHazard = false;
		}
		else {
			blinkerTicks = 0;
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
			blinkerLeft = false;
			blinkerRight = false;
			blinkerHazard = false;
		}
	}
	if (controls.ButtonJustPressed(ScriptControls::WheelControlType::IndicatorRight)) {
		if (!blinkerRight) {
			blinkerTicks = 1;
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, true); // L
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false); // R
			blinkerLeft = false;
			blinkerRight = true;
			blinkerHazard = false;
		}
		else {
			blinkerTicks = 0;
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
			blinkerLeft = false;
			blinkerRight = false;
			blinkerHazard = false;
		}
	}

	float centerPos = (controls.SteerLeft + controls.SteerRight) / 2;
	float wheelCenterDeviation = controls.SteerVal - centerPos;

	if (blinkerTicks == 1 && abs(wheelCenterDeviation/ centerPos) > 0.2f)
	{
		blinkerTicks = 2;
	}

	if (blinkerTicks == 2 && abs(wheelCenterDeviation / centerPos) < 0.1f)
	{
		blinkerTicks = 0;
		VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
		VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
		blinkerLeft = false;
		blinkerRight = false;
		blinkerHazard = false;
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

///////////////////////////////////////////////////////////////////////////////
//                    Wheel input and force feedback
///////////////////////////////////////////////////////////////////////////////

void doWheelSteering() {
	if (prevInput == ScriptControls::InputDevices::Wheel) {
		ext.SetSteeringInputAngle(vehicle, (controls.SteerVal - 32768) / (-32768.0f));
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
	if (!controls.WheelDI.IsConnected() || controls.WheelDI.NoFeedback || prevInput != ScriptControls::Wheel) {
		return;
	}

	// Macro FFB effects on the car body
	int constantForce = -100 * static_cast<int>(settings.FFPhysics * ((3 * accelValsAvg.x + 2 * accelVals.x)));
	
	// targetSpeed is the speed at which the damperForce is at minimum
	// damperForce is maximum at speed 0 and decreases with speed increase
	float adjustRatio = static_cast<float>(settings.DamperMax) / static_cast<float>(settings.TargetSpeed);
	int damperForce = settings.DamperMax - static_cast<int>(speed * adjustRatio);

	// Acceleration also affects damper force
	damperForce -= static_cast<int>(adjustRatio * accelValsAvg.y * std::copysignf(1.0, vehData.Velocity));
	
	if (damperForce < settings.DamperMin) {
		damperForce = settings.DamperMin;
	}

	// steerSpeed is to dampen the steering wheel
	auto steerSpeed = controls.WheelDI.GetAxisSpeed(
		controls.WheelDI.StringToAxis(
			controls.WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Steer)]
		)
	)/20;

	// Centering
	int centerPos = (controls.SteerLeft + controls.SteerRight)/2;
	float divisor = (controls.SteerRight - controls.SteerLeft) / 10000.0f;
	int wheelCenterDeviation = controls.SteerVal - centerPos;
	int centerForce = static_cast<int>((wheelCenterDeviation / divisor) * speed * 0.25) +
					  static_cast<int>((wheelCenterDeviation / divisor) * std::abs(accelValsAvg.y) * 0.25);


	// start oversteer detect
	float oversteer = 0.0f;

	Vector3 rel_vector = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true);
	
	float angle = acos(rel_vector.y / speed)* 180.0f / 3.14159265f;
	if (isnan(angle))
		angle = 0.0;

	if (angle > 10 && vehData.Velocity > 1.0f) {
		oversteer = angle/90.0f;
	}
	// end oversteer detect

	// begin understeer detect
	float understeer = 0.0f;
	if (abs(vehData.SteeringAngle*std::sqrt(vehData.Velocity)) > abs(vehData.RotationVelocity.z) &&
		vehData.Velocity > 0.1f) {
		understeer = abs(vehData.SteeringAngle*sqrt(vehData.Velocity) - vehData.RotationVelocity.z);
	}
	// end understeer detect
	
	if (oversteer > 0.05f) {
		understeer = 0.0f;
	}
	if (understeer > 1.0f) {
		understeer = 1.0f;
	}

	if (oversteer > 1.0f) {
		oversteer = 1.0f;
	}

	float gripSpeedThreshold = 4.0f;

	// On understeering conditions, lower "grippy" feel
	if (understeer > 0.01f && vehData.Velocity > gripSpeedThreshold) {
		centerForce = static_cast<int>((1.0f - understeer) * centerForce);
	}

	// On oversteering conditions, let physics dictate the feel
	if (oversteer > 0.01f && vehData.Velocity > gripSpeedThreshold) {
		centerForce = 0;
		damperForce = settings.DamperMin + static_cast<int>(oversteer * (settings.DamperMax - settings.DamperMin));
	}

	if (vehData.Velocity < -0.1f) {
		centerForce = 0;
		damperForce = settings.DamperMin;
	}

	// Detail feel / suspension compression based
	float compSpeedTotal = 0.0f;
	if (!vehData.IsBike) {
		auto compSpeed = vehData.GetWheelCompressionSpeeds();

		// left should pull left, right should pull right
		compSpeedTotal = -compSpeed[0] + compSpeed[1];
	}

	// Cancel all effects except dampening
	if (airborne) {
		constantForce = 0;
		centerForce = 0;
		damperForce = settings.DamperMin;
	}

	int totalForce = static_cast<int>(steerSpeed * damperForce * 0.1) + 
		constantForce +
		static_cast<int>(settings.CenterStrength * centerForce) +
		static_cast<int>(10.0f * settings.DetailStrength * compSpeedTotal);

	controls.WheelDI.SetConstantForce(totalForce);

	if (settings.Debug) {
		std::stringstream SteerValue;
		SteerValue << "SteerValue: " << controls.SteerVal;
		showText(0.85, 0.175, 0.4, SteerValue.str().c_str());

		std::stringstream steerDisplay;
		steerDisplay << "SteerSpeed: " << steerSpeed;
		showText(0.85, 0.200, 0.4, steerDisplay.str().c_str());
		
		std::stringstream forceDisplay;
		forceDisplay << "ConstForce: " << constantForce;
		showText(0.85, 0.225, 0.4, forceDisplay.str().c_str());

		std::stringstream damperF;
		damperF << "DampForce: " << damperForce;
		showText(0.85, 0.250, 0.4, damperF.str().c_str());

		std::stringstream centerDisplay;
		centerDisplay << "CenterForce: " << centerForce;
		showText(0.85, 0.275, 0.4, centerDisplay.str().c_str());
		
		std::stringstream ssUnderSteer;
		ssUnderSteer << "Understeer: " << understeer;
		showText(0.85, 0.300, 0.4, ssUnderSteer.str().c_str());

		std::stringstream ssOverSteer;
		ssOverSteer << "Oversteer: " << oversteer;
		showText(0.85, 0.325, 0.4, ssOverSteer.str().c_str());
	}
}

///////////////////////////////////////////////////////////////////////////////
//                             Misc features
///////////////////////////////////////////////////////////////////////////////

void functionAutoLookback() {
	if (vehData.CurrGear == 0) {
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleLookBehind, 1.0f);
	}
}


///////////////////////////////////////////////////////////////////////////////
//                              Script entry
///////////////////////////////////////////////////////////////////////////////

void main() {
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
