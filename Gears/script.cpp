#include "script.h"

#include <string>
#include <sstream>
#include <iomanip>
#include <array>

#include "ScriptSettings.hpp"
#include "ScriptControls.hpp"
#include "Logger.hpp"
#include "VehicleData.hpp"
#include "MemoryPatcher.hpp"

Logger logger("Gears.log");

ScriptControls controls;
ScriptSettings settings;
VehicleData vehData;

Vehicle vehicle;
Vehicle prevVehicle;
Hash model;
VehicleExtensions ext;
Player player;
Ped playerPed;

bool runOnceRan = false;
bool patched = false;
bool patchedSpecial = false;
int prevNotification = 0;

void showText(float x, float y, float scale, char * text) {
	UI::SET_TEXT_FONT(0);
	UI::SET_TEXT_SCALE(scale, scale);
	UI::SET_TEXT_COLOUR(255, 255, 255, 255);
	UI::SET_TEXT_WRAP(0.0, 1.0);
	UI::SET_TEXT_CENTRE(0);
	UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
	UI::SET_TEXT_EDGE(1, 0, 0, 0, 205);
	UI::_SET_TEXT_ENTRY("STRING");
	UI::_ADD_TEXT_COMPONENT_STRING(text);
	UI::_DRAW_TEXT(x, y);
}

void showNotification(char *message) {
	if (prevNotification)
		UI::_REMOVE_NOTIFICATION(prevNotification);
	UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
	UI::_ADD_TEXT_COMPONENT_STRING(message);
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
		//"\nV:" << std::setprecision(3) << velocity <<
		"\nAddress: " << std::hex << vehData.Address <<
		"\nE: " << (settings.EnableManual ? "Y" : "N");
	const char *infoc = infos.str().c_str();
	showText(0.01f, 0.5f, 0.4f, (char *)infoc);
}

void reInit() {
	settings.Read(&controls);
	vehData.LockGears = 0x00010001;
}

void toggleManual() {
	settings.EnableManual = !settings.EnableManual;
	std::stringstream message;
	message << "Manual Transmission " <<
		(settings.EnableManual ? "Enabled" : "Disabled");
	showNotification((char *)message.str().c_str());
	logger.Write((char *)message.str().c_str());
	if (ENTITY::DOES_ENTITY_EXIST(vehicle)) {
		VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, false, true);
	}
	if (!settings.EnableManual) {
		patched = !MemoryPatcher::RestoreInstructions();
	}
	if (!runOnceRan)
		runOnceRan = true;

	settings.Save();
	settings.Read(&controls);
}

bool lastKeyboard() {
	return CONTROLS::_GET_LAST_INPUT_METHOD(2) == TRUE;
}

void update() {
	if (controls.IsKeyJustPressed(controls.Control[ScriptControls::Toggle], ScriptControls::Toggle)) {
		toggleManual();
	}

	player = PLAYER::PLAYER_ID();
	playerPed = PLAYER::PLAYER_PED_ID();

	// check if player ped exists and control is on (e.g. not in a cutscene)
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed) || !PLAYER::IS_PLAYER_CONTROL_ON(player))
		return;

	// check for player ped death and player arrest
	if (ENTITY::IS_ENTITY_DEAD(playerPed) || PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE))
		return;

	vehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);
	model = ENTITY::GET_ENTITY_MODEL(vehicle);
	if (!ENTITY::DOES_ENTITY_EXIST(vehicle))
		return;

	if (VEHICLE::IS_THIS_MODEL_A_BICYCLE(model) ||
		!(VEHICLE::IS_THIS_MODEL_A_CAR(model)   ||
		VEHICLE::IS_THIS_MODEL_A_BIKE(model)    ||
		VEHICLE::IS_THIS_MODEL_A_QUADBIKE(model) ) )
		return;

	// Patch clutch on game start
	if (settings.EnableManual && !runOnceRan) {
		logger.Write("Patching functions on start");
		patched = MemoryPatcher::PatchInstructions();
		runOnceRan = true;
	}

	if (CONTROLS::IS_CONTROL_JUST_RELEASED(0, ControlEnter) || vehicle != prevVehicle) {
		reInit();
	}
	prevVehicle = vehicle;

	if (controls.WasControlPressedForMs(controls.Control[ScriptControls::CToggle], controls.CToggleTime) &&
		!lastKeyboard()) {
		toggleManual();
	}

	if (controls.IsKeyJustPressed(controls.Control[ScriptControls::ToggleH], ScriptControls::ToggleH)) {
		settings.Hshifter = !settings.Hshifter;
		std::stringstream message;
		message << "Mode: " <<
			(settings.Hshifter ? "H-Shifter" : "Sequential");
		showNotification((char *)message.str().c_str());
	}

	vehData.ReadMemData(ext, vehicle);

	if (lastKeyboard()) {
		controls.Rtvalf     = (controls.IsKeyPressed(controls.Control[ScriptControls::KThrottle]) ? 1.0f : 0.0f);
		controls.Ltvalf     = (controls.IsKeyPressed(controls.Control[ScriptControls::KBrake]) ? 1.0f : 0.0f);
		controls.Clutchvalf = (controls.IsKeyPressed(controls.Control[ScriptControls::KClutch]) ? 1.0f : 0.0f);
	}
	else {
		controls.Rtvalf     = (CONTROLS::GET_CONTROL_VALUE(0, controls.Control[ScriptControls::CThrottle]) - 127) / 127.0f;
		controls.Ltvalf     = (CONTROLS::GET_CONTROL_VALUE(0, controls.Control[ScriptControls::CBrake]) - 127) / 127.0f;
		controls.Clutchvalf = (CONTROLS::GET_CONTROL_VALUE(0, controls.Control[ScriptControls::Clutch]) - 127) / 127.0f;
	}

	controls.Accelval = CONTROLS::GET_CONTROL_VALUE(0, ControlVehicleAccelerate);
	controls.Accelvalf = (controls.Accelval - 127) / 127.0f;

	// Other scripts. 0 = nothing, 1 = Shift up, 2 = Shift down
	if (vehData.CurrGear > 1 && vehData.Rpm < 0.4f) {//vehData.CurrGear > vehData.NextGear) {
		DECORATOR::DECOR_SET_INT(vehicle, "hunt_score", 2);
	}
	else if (vehData.CurrGear == vehData.NextGear) {
		DECORATOR::DECOR_SET_INT(vehicle, "hunt_score", 0);
	}
	else if (vehData.CurrGear < vehData.NextGear) {
		DECORATOR::DECOR_SET_INT(vehicle, "hunt_score", 1);
	}


	if (settings.Debug) {
		showDebugInfo();
	}

	if (!settings.EnableManual ||
		(!VEHICLE::IS_VEHICLE_DRIVEABLE(vehicle, false) &&
			VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) < -100.0f))
		return;

	// check if player ped is driver
	if (playerPed != VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1)) {
		if (patched) {
			logger.Write("Not a driver:");
			patched = !MemoryPatcher::RestoreInstructions();
		}
		return;
	}

	if (!patched && settings.EnableManual) {
		logger.Write("Re-patching functions");
		patched = MemoryPatcher::PatchInstructions();
	}

	// Special case for clutch used by all vehicles
	// Only patch if user desires to have their clutch @ 0
	if (controls.Clutchvalf > 0.9) {
		if (!patchedSpecial) {
			//logger.Write("Patching ClutchSpecial");
			patchedSpecial = MemoryPatcher::PatchJustS_LOW();
		}
	}
	else {
		if (patchedSpecial) {
			//logger.Write("Restoring ClutchSpecial");
			patchedSpecial = !MemoryPatcher::RestoreJustS_LOW();
		}
	}

	// Automatically engage first gear when stationary
	if (settings.AutoGear1 &&
		vehData.Throttle < 0.1f && vehData.Speed < 0.1f && vehData.CurrGear > 1) {
		vehData.LockGears = 0x00010001;
	}

	// Reverse behavior
	// For bikes, do this automatically.
	// Also if the user chooses to.
	if (vehData.IsBike ||
		(settings.OldReverse && settings.AutoReverse)) {
		if (vehData.CurrGear == 0 && CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate)
			&& !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake) && vehData.Speed < 2.0f) {
			vehData.LockGears = 0x00010001;
		}
		else if (vehData.CurrGear > 0 && CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake)
			&& !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) && vehData.Speed < 2.0f) {
			vehData.LockGears = 0x00000000;
		}
	}
	// Reversing behavior: blocking
	else if (settings.OldReverse) {
		// Block and reverse with brake in reverse gear

		//Park/Reverse. Gear 0. Prevent going forward in gear 0.
		if (vehData.CurrGear == 0
			&& vehData.Throttle > 0) {
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
		}
		// Forward gears. Prevent reversing.
		else if (vehData.CurrGear > 0
			&& vehData.Throttle < 0) {
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
		}
		else {
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
		}
	}
	// New reverse: Reverse with throttle
	else {
		// Case forward gear
		// Desired: Only brake
		if (vehData.CurrGear > 0) {
			// LT behavior when still
			//if (ltvalf > 0.0f && rtvalf < ltvalf &&
			//	velocity >= -0.1f && velocity <= 0.5f) {
			if (controls.Ltvalf > 0.0f && controls.Rtvalf < controls.Ltvalf &&
				vehData.Velocity <= 0.5f && vehData.Velocity >= -0.1f) {
				CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
				ext.SetThrottleP(vehicle, 0.0f);
				VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
				VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
			}
			else {
				VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
			}
			// LT behavior when rolling back
			if (controls.Ltvalf > 0.0f && controls.Rtvalf < controls.Ltvalf &&
				vehData.Velocity < -0.1f) {
				VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
				CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, controls.Ltvalf);
				ext.SetThrottle(vehicle, 0.0f);
				ext.SetThrottleP(vehicle, 0.1f);
			}
		}
		// Case reverse gear
		// Desired: RT reverses, LT brakes
		if (vehData.CurrGear == 0) {
			ext.SetThrottleP(vehicle, -0.1f);
			// RT behavior
			if (controls.Rtvalf > 0.0f && controls.Rtvalf > controls.Ltvalf) {
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
			if (controls.Ltvalf > 0.0f &&
				vehData.Velocity <= -0.5f) {
				CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, controls.Ltvalf);
			}
		}
	}

	// Since trucks don't stop accelerating and don't disengage the clutch at currGear < nextGear
	// Save the speed this happened at until next shift up and don't let truck go faster by doing
	// the clutch thing by ourselves instead.
	if (vehData.CurrGear < vehData.NextGear) {
		if (vehData.IsTruck) {
			vehData.LockSpeed = vehData.Velocity;
		}
		else if (vehData.PrevGear <= vehData.CurrGear || vehData.Velocity <= vehData.LockSpeed || vehData.LockSpeed < 0.01f) {
			vehData.LockSpeed = vehData.Velocity;
		}
		vehData.LockTruck = vehData.IsTruck;
	}

	// Disengage the clutch when cap is reached to cap speed
	if (vehData.IsTruck) {
		if ((vehData.Velocity > vehData.LockSpeed && vehData.LockTruck) ||
			(vehData.Velocity > vehData.LockSpeed && vehData.PrevGear > vehData.CurrGear)) {
			controls.Clutchvalf = 1.0f;
			VEHICLE::_SET_VEHICLE_ENGINE_TORQUE_MULTIPLIER(vehicle, 0.0f);
			VEHICLE::_SET_VEHICLE_ENGINE_POWER_MULTIPLIER(vehicle, 0.0f);
			ext.SetThrottle(vehicle, 0.0f);
			DECORATOR::DECOR_SET_INT(vehicle, "hunt_score", 1); // Uglyyyyy
		}
	}

	// Since we know prevGear and the speed for that, let's simulate "engine" braking.
	if (settings.EngBrake && vehData.CurrGear > 0)
	{
		// Only when free rolling at high speeds
		if (vehData.Velocity > vehData.LockSpeeds.at(vehData.CurrGear) &&
			controls.Rtvalf < 0.99 && vehData.Rpm > 0.9) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, (1.0f - controls.Clutchvalf) * 0.5f * vehData.Rpm);
			if (controls.Ltvalf < 0.1f) {
				VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, false);
			}
			else {
				VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			}
		}
		if (controls.Ltvalf < 0.1f) {
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, false);
		}
	}

	// Game wants to shift up. Triggered at high RPM, high speed.
	// Desired result: high RPM, same gear, no more accelerating
	// Result:	Is as desired. Speed may drop a bit because of game clutch.
	if (vehData.CurrGear > 0 &&
		(vehData.CurrGear < vehData.NextGear && vehData.Speed > 2.0f)) {
		ext.SetThrottle(vehicle, 1.2f);
		ext.SetCurrentRPM(vehicle, 1.07f);
	}

	// Game wants to shift down. Usually triggered when user accelerates
	// while in a too high gear.
	// Desired result: low RPM or stall. Same gear. Low torque.
	// Result:	Patching the clutch ops results in above
	// New result: Additional patching made this superfluous.
	/*
	if (vehData.CurrGear > vehData.NextGear) {
		VEHICLE::_SET_VEHICLE_ENGINE_TORQUE_MULTIPLIER(vehicle, vehData.Rpm);
		// RPM mismatch on transition currGear > nextGear to currGear == nextGear?
	}
	*/

	// Emulate previous "shift down wanted" behavior.
	if (vehData.CurrGear > 1 && vehData.Rpm < 0.4f ) {
		VEHICLE::_SET_VEHICLE_ENGINE_TORQUE_MULTIPLIER(vehicle, vehData.Rpm * 2.5f);
	}

	// Engine damage
	if (settings.EngDamage &&
		vehData.Rpm > 0.98f && controls.Accelvalf > 0.99f) {
		VEHICLE::SET_VEHICLE_ENGINE_HEALTH(vehicle, VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) - (0.2f*controls.Accelvalf));
		if (VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) < 0.0f) {
			VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
		}
	}

	// Stalling
	//if (settings.EngStall && vehData.CurrGear > 2 &&
	//	vehData.Rpm < 0.25f && vehData.Speed < 5.0f && vehData.Clutch > 0.33f) {
	//	VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
	//}

	if (!VEHICLE::_IS_VEHICLE_ENGINE_ON(vehicle) &&
		(CONTROLS::IS_CONTROL_JUST_PRESSED(0, controls.Control[ScriptControls::Engine]) ||
		controls.IsKeyJustPressed(controls.Control[ScriptControls::KEngine], ScriptControls::KEngine))) {
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, false, true);
	}

	// Game doesn't really support revving on disengaged clutch in any gear but 1
	// Simulate this
	//if ()


	// Simulate "catch point"
	// Default behavior - No catch point exists. Car just idles happily with
	// fully engaged clutch. We wanna have it, like this
	// Clutch - 0.0 -> Free engine rev
	// Clutch - 0.2 -> Catch point
	// Clutch - 0.2 to 0.4 -> Starts rolling??
	// Clutch > 0.4 -> Stalling unless rolling in any gear OR hi RPM (> 0.5)
	if (vehData.Clutch >= 0.2f && vehData.Speed < vehData.CurrGear * 2.2f) {
		showText(0.5f, 0.5f, 0.5f, "ROLL");
		if (vehData.Throttle < 0.25f) {
			if (vehData.CurrGear > 0) {
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, 0.37f);
			}
			else {
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, 0.37f);
			}
		}
	}

	// Stalling
	if (vehData.Clutch > 0.57f && vehData.Speed < vehData.CurrGear * 1.4f && vehData.Rpm < 0.35f) {
		showText(0.5f, 0.5f, 0.5f, "DIE");
		if (VEHICLE::_IS_VEHICLE_ENGINE_ON(vehicle)) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, 0.5f);
			VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
		}
	}


	if (vehData.Velocity > 0 && vehData.CurrGear == 0 && controls.Clutchvalf < 0.3f) {
		VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
	}
	if (vehData.Velocity < 0 && vehData.CurrGear > 0 && controls.Clutchvalf < 0.3f) {
		VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
	}

	// sequential or h?
	if (!settings.Hshifter) {
		// Shift up
		if ((CONTROLS::IS_CONTROL_JUST_PRESSED(0, controls.Control[ScriptControls::ShiftUp]) && !lastKeyboard()) ||
			controls.IsKeyJustPressed(controls.Control[ScriptControls::KShiftUp], ScriptControls::KShiftUp)) {
			if (vehData.CurrGear < vehData.TopGear) {
				if (controls.Clutchvalf < 0.1f) {
					controls.Clutchvalf = 1.0f;
				}
				ext.SetThrottle(vehicle, 0.0f);
				vehData.LockGears = vehData.CurrGear + 1 | ((vehData.CurrGear + 1) << 16);
				vehData.LockTruck = false;
				vehData.PrevGear = vehData.CurrGear;
				vehData.LockSpeeds[vehData.CurrGear] = vehData.Velocity;
			}
		}

		// Shift down
		if ((CONTROLS::IS_CONTROL_JUST_PRESSED(0, controls.Control[ScriptControls::ShiftDown]) && !lastKeyboard()) ||
			controls.IsKeyJustPressed(controls.Control[ScriptControls::KShiftDown], ScriptControls::KShiftDown)) {
			if (vehData.CurrGear > 0) {
				if (controls.Clutchvalf < 0.1f) {
					controls.Clutchvalf = 1.0f;
				}
				ext.SetThrottle(vehicle, 0.0f);
				vehData.LockGears = vehData.CurrGear - 1 | ((vehData.CurrGear - 1) << 16);
				vehData.LockTruck = false;
				vehData.PrevGear = vehData.CurrGear;
				vehData.LockSpeeds[vehData.CurrGear] = vehData.Velocity;
			}
		}
	}
	else
	{
		// All keys checking
		for (uint8_t i = 0; i <= vehData.TopGear; i++) {
			if (i > ScriptControls::H8) // this shit is just silly can I rly do dis?
				i = ScriptControls::H8; // holy shit bad, bad, hacky idea
			if (controls.IsKeyJustPressed(controls.Control[i], (ScriptControls::ControlType)i)) {
				if (controls.Clutchvalf < 0.1f) {
					controls.Clutchvalf = 1.0f;
				}
				ext.SetThrottle(vehicle, 0.0f);
				vehData.LockGears = i | (i << 16);
				vehData.LockTruck = false;
				vehData.PrevGear = vehData.CurrGear;
				vehData.LockSpeeds[vehData.CurrGear] = vehData.Velocity;
			}
		}
	}

	// Clutch override working now
	ext.SetClutch(vehicle, 1.0f - controls.Clutchvalf);
	ext.SetGears(vehicle, vehData.LockGears);
}

void main() {
	settings.Read(&controls);
	while (true) {
		update();
		WAIT(0);
	}
}

void ScriptMain() {
	srand(GetTickCount());
	main();
}
