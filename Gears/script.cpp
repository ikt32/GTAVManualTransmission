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
#include "XboxController.hpp"

Logger logger("Gears.log");
ScriptControls controls;
ScriptSettings settings;
VehicleData vehData;
VehicleExtensions ext;
XboxController controller(1);

Vehicle vehicle;
Vehicle prevVehicle;
Hash model;
Player player;
Ped playerPed;

WORD buttonState;

bool runOnceRan = false;
bool patched = false;
bool patchedSpecial = false;
bool simpleBike;
int prevNotification = 0;

void showText(float x, float y, float scale, char * text);
void showNotification(char *message);
void showDebugInfo();
void reInit();
void toggleManual();
bool lastKeyboard();

void update() {
	if (controller.IsConnected()) {
		buttonState = controller.GetState().Gamepad.wButtons;
		controller.UpdateButtonChangeStates();
	}

	if (controls.IsKeyJustPressed(controls.Control[ScriptControls::KToggle], ScriptControls::KToggle)) {
		toggleManual();
	}

	player = PLAYER::PLAYER_ID();
	playerPed = PLAYER::PLAYER_PED_ID();

	if (!ENTITY::DOES_ENTITY_EXIST(playerPed) ||
		!PLAYER::IS_PLAYER_CONTROL_ON(player) || 
		ENTITY::IS_ENTITY_DEAD(playerPed) || 
		PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE))
		return;

	vehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);
	model = ENTITY::GET_ENTITY_MODEL(vehicle);
	prevVehicle = vehicle;

	if (!ENTITY::DOES_ENTITY_EXIST(vehicle) ||
		VEHICLE::IS_THIS_MODEL_A_BICYCLE(model) ||
		!(	VEHICLE::IS_THIS_MODEL_A_CAR(model) ||
			VEHICLE::IS_THIS_MODEL_A_BIKE(model) ||
			VEHICLE::IS_THIS_MODEL_A_QUADBIKE(model)
		))
		return;

	// Patch clutch on game start
	if (settings.EnableManual && !runOnceRan) {
		logger.Write("Patching functions on start");
		patched = MemoryPatcher::PatchInstructions();
		vehData.SimulatedNeutral = settings.DefaultNeutral;
		runOnceRan = true;
	}

	if (CONTROLS::IS_CONTROL_JUST_RELEASED(0, ControlEnter) || vehicle != prevVehicle) {
		reInit();
	}

	if (controller.WasButtonHeldForMs(controller.StringToButton(controls.ControlXbox[ScriptControls::CToggle]), buttonState, controls.CToggleTime) &&
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
	vehData.LockGear = (0xFFFF0000 & vehData.LockGears) >> 16;
	simpleBike = vehData.IsBike && settings.SimpleBike;
	vehData.LockTruck = vehData.IsTruck;

	if (lastKeyboard()) {
		controls.Rtvalf     = (controls.IsKeyPressed(controls.Control[ScriptControls::KThrottle]) ? 1.0f : 0.0f);
		controls.Ltvalf     = (controls.IsKeyPressed(controls.Control[ScriptControls::KBrake]) ? 1.0f : 0.0f);
		controls.Clutchvalf = (controls.IsKeyPressed(controls.Control[ScriptControls::KClutch]) ? 1.0f : 0.0f);
	}
	else {
		controls.Rtvalf     = controller.GetAnalogValue(controller.StringToButton(controls.ControlXbox[ScriptControls::CThrottle]), buttonState);
		controls.Ltvalf     = controller.GetAnalogValue(controller.StringToButton(controls.ControlXbox[ScriptControls::CBrake]), buttonState);
		controls.Clutchvalf = controller.GetAnalogValue(controller.StringToButton(controls.ControlXbox[ScriptControls::Clutch]), buttonState);
	}
	controls.Accelval = CONTROLS::GET_CONTROL_VALUE(0, ControlVehicleAccelerate);
	controls.Accelvalf = (controls.Accelval - 127) / 127.0f;

	// Put here to override last control readout for Clutchvalf
	if (vehData.SimulatedNeutral) {
		controls.Clutchvalf = 1.0f; // same difference in gameplay
	}

	// Override SimulatedNeutral on a bike
	if (simpleBike) {
		controls.Clutchvalf = 0.0f;
	}

	// Other scripts. 0 = nothing, 1 = Shift up, 2 = Shift down
	if (vehData.CurrGear > 1 && vehData.Rpm < 0.4f) {
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

	// Active whenever Manual is enabled from here

	if (playerPed != VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1)) {
		if (patchedSpecial) {
			logger.Write("Not a driver:");
			patchedSpecial = !MemoryPatcher::RestoreJustS_LOW();
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
			patchedSpecial = MemoryPatcher::PatchJustS_LOW();
		}
	}
	else {
		if (patchedSpecial) {
			patchedSpecial = !MemoryPatcher::RestoreJustS_LOW();
		}
	}

	// Simulated neutral gear
	if (controls.IsKeyJustPressed(controls.Control[ScriptControls::KEngageNeutral], ScriptControls::KEngageNeutral) ||
		(controller.WasButtonHeldForMs(controller.StringToButton(controls.ControlXbox[ScriptControls::ShiftDown]), buttonState, controls.CToggleTime) &&
			!lastKeyboard())) {
		vehData.SimulatedNeutral = !vehData.SimulatedNeutral;
		return; //cuz we don't wanna shift this loop! hacky af but *shrug*
	}

	if (settings.UITips && vehData.SimulatedNeutral && !simpleBike) {
		showText(0.95f, 0.90f, 1.4f, "N");
	}

	// BELOW THIS LINE THE FUNCTIONAL STUFF

	// Automatically engage first gear when stationary
	if (settings.AutoGear1) {
		functionAutoGear1();
	}

	// Reverse behavior
	// For bikes, do this automatically.
	// Also if the user chooses to.
	if (vehData.IsBike || settings.AutoReverse) {
		functionAutoReverse();
	}

	// New reverse: Reverse with throttle
	if (settings.RealReverse) {
		functionRealReverse();
	}
	// Reversing behavior: blocking
	if (!settings.RealReverse) {
		functionBlockingReverse();
	}

	// Limit truck speed per gear upon game wanting to shift, but we block it.
	if (vehData.IsTruck) {
		functionTruckLimiting();
	}

	// Since we know prevGear and the speed for that, let's simulate "engine" braking.
	if (settings.EngBrake) {
		functionEngBrake();
	}

	// Game wants to shift up. Triggered at high RPM, high speed.
	// Desired result: high RPM, same gear, no more accelerating
	// Result:	Is as desired. Speed may drop a bit because of game clutch.
	if (vehData.CurrGear > 0 &&
		(vehData.CurrGear < vehData.NextGear && vehData.Speed > 2.0f)) {
		ext.SetThrottle(vehicle, 1.2f);
		ext.SetCurrentRPM(vehicle, 1.07f);
	}

	// Emulate previous "shift down wanted" behavior.
	if (vehData.CurrGear > 1 && vehData.Rpm < 0.4f ) {
		VEHICLE::_SET_VEHICLE_ENGINE_TORQUE_MULTIPLIER(vehicle, vehData.Rpm * 2.5f);
		//VEHICLE::_SET_VEHICLE_ENGINE_POWER_MULTIPLIER(vehicle, vehData.Rpm * 2.5f);
	}

	// Game doesn't really support revving on disengaged clutch in any gear but 1
	// Simulate this
	if (vehData.CurrGear > 1 && controls.Clutchvalf > 0.4f && !vehData.LockTruck) {
		float revValue = vehData.Throttle;
		if (revValue > 0.2f) {
			ext.SetCurrentRPM(vehicle, revValue <= 0.99f ? revValue : 1.05f);
		}
	}

	// Engine damage
	if (settings.EngDamage) {
		functionEngDamage();
	}

	// Stalling
	if (settings.EngStall && !simpleBike) {
		functionEngStall();
	}
	
	// Simulate "catch point"
	// When the clutch "grabs" and the car starts moving without input
	// TODO Differentiate between diesel en gasoline?
	if (settings.ClutchCatching && !simpleBike) {
		functionClutchCatch();
	}

	// Manual shifting
	if (settings.Hshifter) {
		functionHShift();
	}
	else {
		functionSShift();
	}

	// Finally, update memory each loop
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
		"\nAddress: " << std::hex << vehData.Address <<
		"\nE: " << (settings.EnableManual ? "Y" : "N");
	const char *infoc = infos.str().c_str();
	showText(0.01f, 0.5f, 0.4f, (char *)infoc);
}

void reInit() {
	settings.Read(&controls);
	vehData.LockGears = 0x00010001;
	vehData.SimulatedNeutral = settings.DefaultNeutral;
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
		patchedSpecial = !MemoryPatcher::RestoreJustS_LOW();
	}
	if (!runOnceRan)
		runOnceRan = true;
	settings.Save();
	reInit();
}

bool lastKeyboard() {
	return CONTROLS::_GET_LAST_INPUT_METHOD(2) == TRUE;
}

void functionHShift() {
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

void functionSShift() {
	// Shift up
	if (controller.IsButtonJustReleased(controller.StringToButton(controls.ControlXbox[ScriptControls::ShiftUp]), buttonState) ||
		controls.IsKeyJustPressed(controls.Control[ScriptControls::KShiftUp], ScriptControls::KShiftUp)) {
		if (vehData.CurrGear < vehData.TopGear) {
			if (controls.Clutchvalf < 0.1f) {
				controls.Clutchvalf = 1.0f;
			}
			ext.SetThrottle(vehicle, 0.0f);
			vehData.LockGears = vehData.LockGear + 1 | ((vehData.LockGear + 1) << 16);
			vehData.LockTruck = false;
			vehData.PrevGear = vehData.CurrGear;
			vehData.LockSpeeds[vehData.CurrGear] = vehData.Velocity;
		}
	}

	// Shift down
	if (controller.IsButtonJustReleased(controller.StringToButton(controls.ControlXbox[ScriptControls::ShiftDown]), buttonState) ||
		controls.IsKeyJustPressed(controls.Control[ScriptControls::KShiftDown], ScriptControls::KShiftDown)) {
		if (vehData.CurrGear > 0) {
			if (controls.Clutchvalf < 0.1f) {
				controls.Clutchvalf = 1.0f;
			}
			ext.SetThrottle(vehicle, 0.0f);
			vehData.LockGears = vehData.LockGear - 1 | ((vehData.LockGear - 1) << 16);
			vehData.LockTruck = false;
			vehData.PrevGear = vehData.CurrGear;
			vehData.LockSpeeds[vehData.CurrGear] = vehData.Velocity;
		}
	}

}

void functionClutchCatch() {
	if (vehData.Clutch >= 0.2f &&
		((vehData.Speed < vehData.CurrGear * 2.2f) || (vehData.CurrGear == 0))) {
		if (vehData.Throttle < 0.25f) {
			if (vehData.CurrGear > 0) {
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, 0.37f);
			}
			else if (vehData.Velocity > -2.2f && controls.Ltvalf < 0.1f) {
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, 0.30f);
			}
		}
	}
}

void functionEngStall() {
	if (vehData.Clutch > 0.65f &&
		((vehData.Speed < vehData.CurrGear * 1.4f) || (vehData.CurrGear == 0 && vehData.Speed < 1.0f))
		&& vehData.Rpm < 0.27f) {
		if (VEHICLE::_IS_VEHICLE_ENGINE_ON(vehicle)) {
			// Maybe add "jerk" on sudden stall?
			VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
		}
	}
	if (!VEHICLE::_IS_VEHICLE_ENGINE_ON(vehicle) &&
		(controller.IsButtonJustPressed(controller.StringToButton(controls.ControlXbox[ScriptControls::Engine]), buttonState) ||
			controls.IsKeyJustPressed(controls.Control[ScriptControls::KEngine], ScriptControls::KEngine))) {
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, false, true);
	}
}

void functionEngDamage() {
	if ( vehData.Rpm > 0.98f && controls.Accelvalf > 0.99f) {
		VEHICLE::SET_VEHICLE_ENGINE_HEALTH(vehicle, VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) - (0.2f*controls.Accelvalf));
	}
}

void functionEngBrake() {
	// Save speed @ shift
	if (vehData.CurrGear < vehData.NextGear) {
		if (vehData.PrevGear <= vehData.CurrGear || vehData.Velocity <= vehData.LockSpeed || vehData.LockSpeed < 0.01f) {
			vehData.LockSpeed = vehData.Velocity;
		}
	}
	// Braking
	if (vehData.CurrGear > 0 && vehData.Velocity > vehData.LockSpeeds.at(vehData.CurrGear) &&
		controls.Rtvalf < 0.1 && vehData.Rpm > 0.80) {
		float brakeForce = -0.1f * (1.0f - controls.Clutchvalf) * vehData.Rpm;
		ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(vehicle, 1, 0.0f, brakeForce, 0.0f, true, true, true, true);
	}
}

void functionRevBehavior() {

}

void functionTruckLimiting() {
	// Save speed
	if (vehData.CurrGear < vehData.NextGear) {
		vehData.LockSpeed = vehData.Velocity;
	}

	// Limit
	if ((vehData.Velocity > vehData.LockSpeed && vehData.LockTruck) ||
		(vehData.Velocity > vehData.LockSpeed && vehData.PrevGear > vehData.CurrGear)) {
		controls.Clutchvalf = 1.0f;
		VEHICLE::_SET_VEHICLE_ENGINE_TORQUE_MULTIPLIER(vehicle, 0.0f);
		VEHICLE::_SET_VEHICLE_ENGINE_POWER_MULTIPLIER(vehicle, 0.0f);
		ext.SetThrottle(vehicle, 0.0f);
		DECORATOR::DECOR_SET_INT(vehicle, "hunt_score", 1); // Uglyyyyy
	}
}

// Still a bit huge but OH WELL
void functionRealReverse() {
	// Forward gear
	// Desired: Only brake
	if (vehData.CurrGear > 0) {
		// LT behavior when still
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
	// Reverse gear
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

void functionBlockingReverse() {
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

void functionAutoReverse() {
	if (vehData.CurrGear == 0 && CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate)
		&& !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake) && vehData.Speed < 2.0f) {
		vehData.LockGears = 0x00010001;
	}
	else if (vehData.CurrGear > 0 && CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake)
		&& !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) && vehData.Speed < 2.0f) {
		vehData.LockGears = 0x00000000;
	}
}

void functionAutoGear1() {
	if (vehData.Throttle < 0.1f && vehData.Speed < 0.1f && vehData.CurrGear > 1) {
		vehData.LockGears = 0x00010001;
	}
}
