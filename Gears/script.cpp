#define NOMINMAX
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "../../ScriptHookV_SDK/inc/natives.h"
#include "../../ScriptHookV_SDK/inc/enums.h"
#include "../../ScriptHookV_SDK/inc/main.h"
#include "../../ScriptHookV_SDK/inc/types.h"

#include "script.h"

#include "ScriptSettings.hpp"
#include "VehicleData.hpp"

#include "Input/ScriptControls.hpp"
#include "Memory/MemoryPatcher.hpp"
#include "Util/Logger.hpp"
#include "Util/Util.hpp"
#include "General.h"

#include "Menu/MenuClass.h"
#include "menu/Controls.h"
#include "Input/keyboard.h"

GameSound gearRattle("DAMAGED_TRUCK_IDLE", 0);

int soundID;

std::string settingsGeneralFile;
std::string settingsWheelFile;
std::string settingsMenuFile;

Menu menu;
MenuControls menuControls;

ScriptControls controls;
ScriptSettings settings(settingsGeneralFile, settingsWheelFile);

Player player;
Ped playerPed;
Vehicle vehicle;
Vehicle prevVehicle;
VehicleData vehData;
VehicleExtensions ext;

int prevNotification = 0;
int prevExtShift = 0;

int speedoIndex;
//todo: srsly unfuck pls

std::vector<std::string> speedoTypes = {
	"off",
	"kph",
	"mph",
	"ms"
};

enum ShiftModes {
	Sequential = 0,
	HPattern = 1,
	Automatic = 2
};

// FontName, fontID
std::vector<std::string> fonts {
	{ "Chalet London" },
	{ "Sign Painter" },
	{ "Slab Serif" },
	{ "Chalet Cologne" },
	{ "Pricedown" },
};

std::vector<int> fontIDs {
	0,
	1,
	2,
	4,
	7
};

std::vector<std::string> buttonConfTags{
	{ "SHIFT_UP" },
	{ "SHIFT_DOWN" },
	{ "ENGINE" },
	{ "HANDBRAKE" },
	{ "HORN" },
	{ "LIGHTS" },
	{ "LOOK_BACK" },
	{ "CHANGE_CAMERA" },
	{ "RADIO_NEXT" },
	{ "RADIO_PREVIOUS" },
	{ "INDICATOR_LEFT" },
	{ "INDICATOR_RIGHT" },
	{ "INDICATOR_HAZARD" },
	{ "TOGGLE_MOD" },
	{ "CHANGE_SHIFTMODE" },
};

std::vector<std::string> keyboardConfTags{
	{ "Toggle" },
	{ "ToggleH" },
	{ "ShiftUp" },
	{ "ShiftDown" },
	{ "Clutch" },
	{ "Engine" },
	{ "Throttle" },
	{ "Brake" },
	{ "HR" },
	{ "H1" },
	{ "H2" },
	{ "H3" },
	{ "H4" },
	{ "H5" },
	{ "H6" },
	{ "H7" },
	{ "HN" },
};

std::vector<std::string> controllerConfTags{
	{ "Toggle" },
	{ "ToggleShift" },
	{ "ShiftUp" },
	{ "ShiftDown" },
	{ "Clutch" },
	{ "Engine" },
	{ "Throttle" },
	{ "Brake" }
};

std::vector<std::string> keyboardConfTagsDetail{
	{ "Toggle mod on/off" },
	{ "Switch shift mode" },
	{ "Shift up" },
	{ "Shift down" },
	{ "Hold for clutch" },
	{ "Toggle engine on/off" },
	{ "Key used for throttle" },
	{ "Key used for brake" },
	{ "H-pattern gear R press" },
	{ "H-pattern gear 1 press" },
	{ "H-pattern gear 2 press" },
	{ "H-pattern gear 3 press" },
	{ "H-pattern gear 4 press" },
	{ "H-pattern gear 5 press" },
	{ "H-pattern gear 6 press" },
	{ "H-pattern gear 7 press" },
	{ "H-pattern Neutral" },
};

std::vector<std::string> controllerConfTagDetail{
	{ "Toggle mod usage: hold" },
	{ "Toggle shift usage: hold" },
	{ "Shift up usage: press" },
	{ "Shift down usage: press" },
	{ "Clutch usage: axis or button" },
	{ "Engine usage: press" },
	{ "Throttle: axis or button" },
	{ "Brake: axis or button" }
};
	

const std::string escapeKey = "BACKSPACE";
const std::string skipKey = "RIGHT";

void update() {
	///////////////////////////////////////////////////////////////////////////
	//                     Are we in a supported vehicle?
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

	if (!ENTITY::DOES_ENTITY_EXIST(vehicle) ||
		playerPed != VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1)) {
		reset();
		return;
	}

	///////////////////////////////////////////////////////////////////////////
	//                           Update stuff
	///////////////////////////////////////////////////////////////////////////

	if (prevVehicle != vehicle) {
		ext.ClearAddress();
		auto addr = ext.GetAddress(vehicle);
		if (settings.LogCar) {
			std::stringstream hexaddr;
			hexaddr << std::hex << static_cast<void*>(addr);
			logger.Write("DEBUG: New vehicle address: " + hexaddr.str());
		}
		if (vehData.NoClutch) {
			vehData.SimulatedNeutral = false;
		}
		else {
			vehData.SimulatedNeutral = settings.DefaultNeutral;
		}
		shiftTo(1, true);
	}
	prevVehicle = vehicle;

	vehData.UpdateValues(ext, vehicle);
	bool ignoreClutch = false;
	if (settings.ShiftMode == Automatic) {
		ignoreClutch = true;
	}
	if (vehData.Class == VehicleData::VehicleClass::Bike && settings.SimpleBike) {
		ignoreClutch = true;
	}
	// Clutch pedal serves as rudder control
	if (vehData.Class == VehicleData::VehicleClass::Plane) {
		ignoreClutch = false;
	}

	controls.UpdateValues(controls.PrevInput, ignoreClutch, false);

	if (settings.DisplayInfo) {
		showDebugInfo();
	}

	if (!settings.IsCorrectVersion()) {
		Color red;
		red.R = 255;
		red.G = 0;
		red.B = 0;
		red.A = 255;

		showText(0.05, 0.05, 1.0, CharAdapter(settings.GetVersionError().c_str()), 0, red);
	}

	///////////////////////////////////////////////////////////////////////////
	//                            Alt vehicle controls
	///////////////////////////////////////////////////////////////////////////
	if (vehData.Class != VehicleData::VehicleClass::Car &&
		vehData.Class != VehicleData::VehicleClass::Bike &&
		vehData.Class != VehicleData::VehicleClass::Quad) {
		// Alt Support

		updateLastInputDevice();

		if (settings.EnableWheel &&
			settings.AltControls &&
			controls.WheelControl.IsConnected(controls.SteerGUID) &&
			controls.PrevInput == ScriptControls::Wheel) {
			if (controls.ButtonJustPressed(ScriptControls::KeyboardControlType::Toggle) ||
				controls.ButtonHeld(ScriptControls::ControllerControlType::Toggle) ||
				controls.ButtonJustPressed(ScriptControls::WheelControlType::Toggle)) {
				reInit();
			}

			if (vehData.Class == VehicleData::VehicleClass::Boat) {
				handleVehicleButtons();
				handlePedalsDefault(controls.ThrottleVal, controls.BrakeVal);
				doWheelSteeringBoat();
				playWheelEffects(settings, vehData, false, true);
			}

			// Planes
			if (vehData.Class == VehicleData::VehicleClass::Plane) {
				doWheelSteeringPlane();
				playWheelEffects(settings, vehData, false, true);
			}
			return;
		}
		return;
	}
	
	///////////////////////////////////////////////////////////////////////////
	//                          Ground vehicle controls
	///////////////////////////////////////////////////////////////////////////

	if (controls.ButtonJustPressed(ScriptControls::KeyboardControlType::Toggle) ||
		controls.ButtonHeld(ScriptControls::ControllerControlType::Toggle) ||
		controls.ButtonJustPressed(ScriptControls::WheelControlType::Toggle)) {
		toggleManual();
	}

	if (!settings.EnableManual &&
		settings.WheelWithoutManual &&
		controls.WheelControl.IsConnected(controls.SteerGUID)) {

		updateLastInputDevice();
		handleVehicleButtons();
		handlePedalsDefault(controls.ThrottleVal, controls.BrakeVal);
		doWheelSteering();
		bool airborne = !VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle) &&
			ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(vehicle) > 1.25f;
		playWheelEffects(settings, vehData, airborne);
	}	

	vehData.LockGear = (0xFFFF0000 & vehData.LockGears) >> 16;
	
	if (settings.CrossScript) {
		crossScriptComms();
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

	if (controls.WheelControl.IsConnected(controls.SteerGUID)) {
		doWheelSteering();
		bool airborne = !VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle) &&
			ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(vehicle) > 1.25f;
		playWheelEffects(settings, vehData, airborne );
	}
	

	if (controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ToggleH) ||
		controls.ButtonHeld(ScriptControls::ControllerControlType::ToggleH) ||
		controls.ButtonJustPressed(ScriptControls::WheelControlType::ToggleH)) {
		cycleShiftMode();
	}

	if (settings.HUD) {
		showHUD();
	}

	///////////////////////////////////////////////////////////////////////////
	//                            Patching
	///////////////////////////////////////////////////////////////////////////
	if (MemoryPatcher::TotalPatched != MemoryPatcher::TotalToPatch && settings.EnableManual) {
		MemoryPatcher::PatchInstructions();
	}

	if (!MemoryPatcher::SteeringPatched && settings.PatchSteering && settings.PatchSteeringAlways) {
		MemoryPatcher::PatchSteeringCorrection();
	}
	///////////////////////////////////////////////////////////////////////////
	// Actual mod operations
	///////////////////////////////////////////////////////////////////////////
	
	// Hill-start effect, gravity and stuff
	// Courtesy of XMOD
	if (settings.HillBrakeWorkaround) {
		if (!controls.BrakeVal 
			&& vehData.Speed < 2.0f &&
			VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle))	{
			float clutchNeutral = vehData.SimulatedNeutral ? 1.0f : controls.ClutchVal;
			if (vehData.Pitch < 0 || clutchNeutral) {
				ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
					vehicle, 1, 0.0f, -1 * (vehData.Pitch / 150.0f) * 1.1f * clutchNeutral, 0.0f, true, true, true, true);
			}
			if (vehData.Pitch > 10.0f || clutchNeutral)
				ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
					vehicle, 1, 0.0f, -1 * (vehData.Pitch / 90.0f) * 0.35f * clutchNeutral, 0.0f, true, true, true, true);
		}
	}

	// Reverse behavior
	// For bikes, do this automatically.
	if (vehData.Class == VehicleData::VehicleClass::Bike && settings.SimpleBike) {
		if (controls.PrevInput == ScriptControls::InputDevices::Wheel) {
			handlePedalsDefault( controls.ThrottleVal, controls.BrakeVal );
		} else {
			functionAutoReverse();
		}
	}
	else {
		if (controls.PrevInput == ScriptControls::InputDevices::Wheel) {
			handlePedalsRealReverse( controls.ThrottleVal, controls.BrakeVal );
		} else {
			functionRealReverse();
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

	if (!vehData.SimulatedNeutral && 
		!(settings.SimpleBike && vehData.Class == VehicleData::VehicleClass::Bike) && 
		!vehData.NoClutch) {
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
		if (controls.PrevInput == ScriptControls::Wheel) {
			functionHShiftWheel();
		}
		if (controls.PrevInput == ScriptControls::Keyboard) {
			functionHShiftKeyboard();
		}
	}
	else if (settings.ShiftMode == 0){
		functionSShift();
		if (settings.AutoGear1) {
			functionAutoGear1();
		}
	}
	else { // Automatic
		functionAShift();
	}

	if (settings.AutoLookBack) {
		functionAutoLookback();
	}

	if (gearRattle.Active) {
		if (controls.ClutchVal > 1.0f - settings.ClutchCatchpoint ||
			!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
			gearRattle.Stop();
		}
	}

	// Finally, update memory each loop
	handleRPM();
	ext.SetGears(vehicle, vehData.LockGears);
}

///////////////////////////////////////////////////////////////////////////////
//                           Helper functions/tools
///////////////////////////////////////////////////////////////////////////////
void drawRPMIndicator(float x, float y, float width, float height, Color fg, Color bg, float rpm) {
	float bgpaddingx = 0.00f;
	float bgpaddingy = 0.01f;
	// background
	GRAPHICS::DRAW_RECT(x, y, width+bgpaddingx, height+bgpaddingy, bg.R, bg.G, bg.B, bg.A);

	// rpm thingy
	GRAPHICS::DRAW_RECT(x-width*0.5f+rpm*width*0.5f, y, width*rpm, height, fg.R, fg.G, fg.B, fg.A);
}

void showHUD() {

	// Gear number indication
	if (vehData.SimulatedNeutral) {
		showText(settings.GearXpos, settings.GearYpos, settings.GearSize, "N");
	}
	else if (vehData.CurrGear == 0) {
		showText(settings.GearXpos, settings.GearYpos, settings.GearSize, "R");
	}
	else {
		char * gear = CharAdapter(std::to_string(vehData.CurrGear).c_str());
		Color c;
		if (vehData.CurrGear == vehData.TopGear) {
			c.R = settings.GearTopColorR;
			c.G = settings.GearTopColorG;
			c.B = settings.GearTopColorB;
			c.A = 255;
		}
		else {
			c.R = 255;
			c.G = 255;
			c.B = 255;
			c.A = 255;
		}
		showText(settings.GearXpos, settings.GearYpos, settings.GearSize, gear, 0, c);
	}

	// Shift mode indicator
	char * shiftModeText;
	switch (settings.ShiftMode) {
	case Sequential: shiftModeText = "S";
		break;
	case HPattern: shiftModeText = "H";
		break;
	case Automatic: shiftModeText = "A";
		break;
	default: shiftModeText = "";
		break;
	}
	showText(settings.ShiftModeXpos, settings.ShiftModeYpos, settings.ShiftModeSize, shiftModeText);
	
	// Speedometer using dashboard speed
	if (settings.Speedo == "kph" ||
		settings.Speedo == "mph" ||
		settings.Speedo == "ms") {
		char * speedoText = "";
		std::stringstream speedoFormat;

		float dashms = ext.GetDashSpeed(vehicle);

		if (settings.Speedo == "kph" ) {
			speedoFormat << static_cast<int>(std::round(dashms * 3.6f));
			if (settings.SpeedoShowUnit) speedoFormat << " km/h";
		}
		if (settings.Speedo == "mph" ) {
			speedoFormat << static_cast<int>(std::round(dashms / 0.44704f));
			if (settings.SpeedoShowUnit) speedoFormat << " mph";
		}
		if (settings.Speedo == "ms") {
			speedoFormat << static_cast<int>(std::round(dashms));
			if (settings.SpeedoShowUnit) speedoFormat << " m/s";
		}
		speedoText = CharAdapter(speedoFormat.str().c_str());
		showText(settings.SpeedoXpos, settings.SpeedoYpos, settings.SpeedoSize, speedoText);
	}

	// RPM Indicator!
	if (settings.RPMIndicator) {
		Color background = {
			settings.RPMIndicatorBackgroundR,
			settings.RPMIndicatorBackgroundG,
			settings.RPMIndicatorBackgroundB,
			settings.RPMIndicatorBackgroundA
		};

		Color foreground = {
			settings.RPMIndicatorForegroundR,
			settings.RPMIndicatorForegroundG,
			settings.RPMIndicatorForegroundB,
			settings.RPMIndicatorForegroundA
		};

		Color rpmcolor = foreground;
		if (vehData.Rpm > settings.RPMIndicatorRedline) {
			Color redline = {
				settings.RPMIndicatorRedlineR,
				settings.RPMIndicatorRedlineG,
				settings.RPMIndicatorRedlineB,
				settings.RPMIndicatorRedlineA
			};
			rpmcolor = redline;
		}
		if (vehData.CurrGear < vehData.NextGear || vehData.TruckShiftUp) {
			Color rpmlimiter = {
				settings.RPMIndicatorRevlimitR,
				settings.RPMIndicatorRevlimitG,
				settings.RPMIndicatorRevlimitB,
				settings.RPMIndicatorRevlimitA
			};
			rpmcolor = rpmlimiter;
		}
		drawRPMIndicator(
			settings.RPMIndicatorXpos,
			settings.RPMIndicatorYpos,
			settings.RPMIndicatorWidth,
			settings.RPMIndicatorHeight,
			rpmcolor,
			background,
			vehData.Rpm
		);
	}
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
	ssAddress << "Address: " << std::hex << reinterpret_cast<uint64_t>(vehData.Address);
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

	/*std::stringstream ssClutchDisable;
	ssClutchDisable << (controls.ClutchDisable ? "Disabled:" : "");*/

	std::stringstream ssHandbrakInput;
	ssHandbrakInput << "HbrakeVal: " << controls.HandbrakeVal;

	showText(0.85, 0.050, 0.4, ssThrottleInput.str().c_str());
	showText(0.85, 0.075, 0.4, ssBrakeInput.str().c_str());
	showText(0.85, 0.100, 0.4, ssClutchInput.str().c_str());
	//showText(0.795, 0.100, 0.4, ssClutchDisable.str().c_str());
	showText(0.85, 0.125, 0.4, ssHandbrakInput.str().c_str());

	if (settings.EnableWheel) {
		std::stringstream dinputDisplay;
		dinputDisplay << "Wheel Avail: " << controls.WheelControl.IsConnected(controls.SteerGUID);
		showText(0.85, 0.150, 0.4, dinputDisplay.str().c_str());
	}
}

void showDebugInfoWheel(ScriptSettings &settings, float effSteer, int damperForce, float steerSpeed, double GForce, float oversteer, float understeer) {
	std::stringstream SteerRaw;
	SteerRaw << "SteerRaw: " << controls.SteerVal;
	showText(0.85, 0.175, 0.4, SteerRaw.str().c_str());

	std::stringstream SteerNorm;
	SteerNorm << "SteerNorm: " << effSteer;
	showText(0.85, 0.200, 0.4, SteerNorm.str().c_str());

	std::stringstream steerDisplay;
	steerDisplay << "SteerSpeed: " << steerSpeed;
	showText(0.85, 0.225, 0.4, steerDisplay.str().c_str());

	std::stringstream GForceDisplay;
	GForceDisplay << "GForceFinal: " <<
		std::setprecision(5) << (-GForce * 5000 * settings.PhysicsStrength * settings.FFGlobalMult);
	showText(0.85, 0.250, 0.4, GForceDisplay.str().c_str());

	std::stringstream damperF;
	damperF << "DampForce: " << steerSpeed * damperForce * 0.1;
	showText(0.85, 0.300, 0.4, damperF.str().c_str());

	std::stringstream ssUnderSteer;
	ssUnderSteer << "Understeer: " << understeer;
	showText(0.85, 0.325, 0.4, ssUnderSteer.str().c_str());

	std::stringstream ssOverSteer;
	ssOverSteer << "Oversteer: " << oversteer;
	showText(0.85, 0.350, 0.4, ssOverSteer.str().c_str());
}

// To expose some variables to other scripts
void crossScriptComms() {
	// FiveM "support": Just don't do anything if that version is detected.
	if (getGameVersion() == VER_1_0_505_2_NOSTEAM) return;

	// Current gear
	DECORATOR::DECOR_SET_INT(vehicle, "doe_elk", vehData.CurrGear);

	// Shift indicator: 0 = nothing, 1 = Shift up, 2 = Shift down
	if (vehData.CurrGear < vehData.NextGear || vehData.TruckShiftUp) {
		DECORATOR::DECOR_SET_INT(vehicle, "hunt_score", 1);
	}
	else if (vehData.CurrGear > 1 && vehData.Rpm < 0.4f) {
		DECORATOR::DECOR_SET_INT(vehicle, "hunt_score", 2);
	}
	else if (vehData.CurrGear == vehData.NextGear) {
		DECORATOR::DECOR_SET_INT(vehicle, "hunt_score", 0);
	}

	// Simulated Neutral
	if (vehData.SimulatedNeutral && settings.EnableManual) {
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
	settings.Read(&menuControls);
	// nasty but it should work enough...
	menu.LoadMenuTheme(std::wstring(settingsMenuFile.begin(), settingsMenuFile.end()).c_str());

	// lel we're not gonna exceed max_int anyway
	speedoIndex = static_cast<int>(std::find(speedoTypes.begin(), speedoTypes.end(), settings.Speedo) - speedoTypes.begin());
	if (speedoIndex >= speedoTypes.size()) {
		speedoIndex = 0;
	}

	logger.Write("Settings read");
	vehData.LockGears = 0x00010001;
	vehData.SimulatedNeutral = settings.DefaultNeutral;
	if (settings.EnableWheel) {
		controls.InitWheel(settings.EnableFFB);
		controls.CheckGUIDs(settings.reggdGuids);
	}
	controls.SteerGUID = controls.WheelAxesGUIDs[static_cast<int>(controls.SteerAxisType)];

	logger.Write("Initialization finished");
}

void reset() {
	gearRattle.Stop();
	prevVehicle = 0;
	if (MemoryPatcher::TotalPatched == MemoryPatcher::TotalToPatch) {
		MemoryPatcher::RestoreInstructions();
	}
	if (MemoryPatcher::SteeringPatched) {
		MemoryPatcher::RestoreSteeringCorrection();
	}
	if (settings.EnableFFB && controls.WheelControl.IsConnected(controls.SteerGUID)) {
		controls.WheelControl.SetConstantForce(controls.SteerGUID, 0);
	}
}

void toggleManual() {
	settings.EnableManual = !settings.EnableManual;
	settings.SaveGeneral();
	std::stringstream message;
	message << "Manual Transmission " <<
	           (settings.EnableManual ? "Enabled" : "Disabled");
	showNotification(CharAdapter(message.str().c_str()), &prevNotification);
	logger.Write(message.str());
	if (ENTITY::DOES_ENTITY_EXIST(vehicle)) {
		VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, false, true);
	}
	if (!settings.EnableManual) {
		reset();
	}
	reInit();
}

void updateLastInputDevice() {
	if (controls.PrevInput != controls.GetLastInputDevice(controls.PrevInput,settings.EnableWheel)) {
		controls.PrevInput = controls.GetLastInputDevice(controls.PrevInput, settings.EnableWheel);
		// ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
		switch (controls.PrevInput) {
			case ScriptControls::Keyboard:
				showNotification("Switched to keyboard/mouse", &prevNotification);
				break;
			case ScriptControls::Controller: // Controller
				if (settings.ShiftMode == HPattern) {
					showNotification("Switched to controller\nSequential re-initiated", &prevNotification);
					settings.ShiftMode = Sequential;
					//settings.Save();
				}
				else {
					showNotification("Switched to controller", &prevNotification);
				}
				break;
			case ScriptControls::Wheel:
				showNotification("Switched to wheel", &prevNotification);
				break;
		}
		if (controls.PrevInput != ScriptControls::Wheel && settings.LogiLEDs) {
			for (GUID guid : controls.WheelControl.GetGuids()) {
				controls.WheelControl.PlayLedsDInput(guid, 0.0, 0.5, 1.0);
			}
		}
	}
	if (controls.PrevInput == ScriptControls::Wheel) {
		CONTROLS::STOP_PAD_SHAKE(0);
		if (!MemoryPatcher::SteeringPatched && settings.PatchSteering) {
			MemoryPatcher::PatchSteeringCorrection();
		}
	} else {
		if (MemoryPatcher::SteeringPatched && settings.PatchSteering && !settings.PatchSteeringAlways) {
			MemoryPatcher::RestoreSteeringCorrection();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Shifting
///////////////////////////////////////////////////////////////////////////////

void setShiftMode(int shiftMode) {
	gearRattle.Stop();
	if (shiftMode > 2 || shiftMode < 0)
		return;

	if (settings.ShiftMode == HPattern  && (vehData.Class == VehicleData::VehicleClass::Bike || controls.PrevInput == ScriptControls::Controller)) {
		settings.ShiftMode = Automatic;
	}

	if ((settings.ShiftMode == Automatic || settings.ShiftMode == Sequential) && vehData.CurrGear > 1) {
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
	showNotification(CharAdapter(message.str().c_str()), &prevNotification);
}

void cycleShiftMode() {
	settings.ShiftMode++;
	if (settings.ShiftMode > 2) {
		settings.ShiftMode = 0;
	}

	setShiftMode(settings.ShiftMode);
	//settings.Save();
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
	// for engine braking, but we're not doing anything if that isn't enabled
	// Only increase the limiter speed
	if (vehData.Velocity > vehData.LockSpeeds[vehData.CurrGear]) {
		vehData.LockSpeeds[vehData.CurrGear] = vehData.Velocity;
	}
}
void functionHShiftTo(int i) {
	if (settings.ClutchShiftingH && !vehData.NoClutch) {
		if (controls.ClutchVal > 1.0f - settings.ClutchCatchpoint) {
			shiftTo(i, false);
			vehData.SimulatedNeutral = false;
			gearRattle.Stop();
		}
		else {
			gearRattle.Play(vehicle);
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
		gearRattle.Stop();
	}
}

void functionHShiftKeyboard() {
	// highest vehicle gear is 7th
	int clamp = 7;
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
	// highest vehicle gear is 7th
	int clamp = 7;
	if (vehData.TopGear <= clamp) {
		clamp = vehData.TopGear;
	}
	for (uint8_t i = 0; i <= clamp; i++) {
		if (controls.ButtonJustPressed(static_cast<ScriptControls::WheelControlType>(i))) {
			functionHShiftTo(i);
		}
	}
	// Bleh
	if (controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H1)) ||
		controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H2)) ||
		controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H3)) ||
		controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H4)) ||
		controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H5)) ||
		controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H6)) ||
		controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H7))
	) {
		if (settings.ClutchShiftingH &&
			settings.EngDamage &&
			!vehData.NoClutch) {
			if (controls.ClutchVal < 1.0 - settings.ClutchCatchpoint) {
				gearRattle.Play(vehicle);
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
	if (controls.PrevInput == ScriptControls::Controller	&& controls.ButtonJustPressed(ScriptControls::ControllerControlType::ShiftUp) ||
		controls.PrevInput == ScriptControls::Keyboard		&& controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ShiftUp) ||
		controls.PrevInput == ScriptControls::Wheel			&& controls.ButtonJustPressed(ScriptControls::WheelControlType::ShiftUp)) {
		if (vehData.NoClutch) {
			if (vehData.CurrGear < vehData.TopGear) {
				shiftTo(vehData.LockGear + 1, true);
			}
			return;
		}

		// Shift block /w clutch shifting for seq.
		if (settings.ClutchShiftingS && 
			controls.ClutchVal < 1.0f - settings.ClutchCatchpoint) {
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

	if (controls.PrevInput == ScriptControls::Controller	&& controls.ButtonJustPressed(ScriptControls::ControllerControlType::ShiftDown) ||
		controls.PrevInput == ScriptControls::Keyboard		&& controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ShiftDown) ||
		controls.PrevInput == ScriptControls::Wheel			&& controls.ButtonJustPressed(ScriptControls::WheelControlType::ShiftDown)) {
		if (vehData.NoClutch) {
			if (vehData.CurrGear > 0) {
				shiftTo(vehData.LockGear - 1, true);
			}
			return;
		}

		// Shift block /w clutch shifting for seq.
		if (settings.ClutchShiftingS &&
			controls.ClutchVal < 1.0f - settings.ClutchCatchpoint) {
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

void functionAShift() { // Automatic
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
		vehData.SimulatedNeutral = false;
	}

	// Shift down
	if ((vehData.CurrGear > 1 && vehData.Rpm < 0.4f) ||
		(vehData.CurrGear > 1 && vehData.Rpm < 0.5f) && vehData.Throttle > 0.95f) {
		shiftTo(vehData.CurrGear - 1, true);
		vehData.NextGear = vehData.CurrGear - 1;
		vehData.SimulatedNeutral = false;
	}
}

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Gearbox features
///////////////////////////////////////////////////////////////////////////////

void functionClutchCatch() {
	if (controls.ClutchVal < 1.0f - settings.ClutchCatchpoint) {
		// Automatic cars APPARENTLY need little/no brake pressure to stop
		if (settings.ShiftMode == Automatic && controls.BrakeVal > 0.1f || vehData.Rpm > 0.3f) {
			return;
		}
		// todo - clutchval changes acceleration factor/speed factor
		// Forward
		if (vehData.CurrGear > 0 && vehData.Velocity < vehData.CurrGear * 2.2f &&
		    controls.ThrottleVal < 0.25f && controls.BrakeVal < 0.95) {
			if (VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle)) {
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, 0.27f);
			}
			else {
				if (vehData.Rpm < 0.3f) {
					CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, 0.27f);
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
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, 0.27f);
			}
			else if (vehData.Rpm < 0.3f) {
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, 0.27f);
			}
		}
	}
}

void functionEngStall() {
	if (controls.ClutchVal < 1.0f - settings.StallingThreshold &&
		vehData.Rpm < 0.21f &&
		((vehData.Speed < vehData.CurrGear * 1.4f) || (vehData.CurrGear == 0 && vehData.Speed < 1.0f)) &&
		VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) &&
		VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle)) {
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
		gearRattle.Stop();
	}
}

void functionEngDamage() {
	if (vehData.Rpm > 0.98f &&
		vehData.ControlAccelerate > 0.99f) {
		VEHICLE::SET_VEHICLE_ENGINE_HEALTH(
			vehicle, VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) - (settings.RPMDamage));
	}
}

void functionEngBrake() {
	// Braking
	if (vehData.LockSpeeds[vehData.CurrGear] > 1.0f &&
		vehData.CurrGear != 0 && vehData.CurrGear != vehData.TopGear &&
		vehData.Velocity > vehData.LockSpeeds[vehData.CurrGear]+2.0f &&
		vehData.Rpm >= 1.00) {
		float brakeForce = -0.20f * (1.0f - controls.ClutchVal);
		ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(vehicle, 1, 0.0f, brakeForce, 0.0f, true, true, true, true);
	}
}

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Gearbox control
///////////////////////////////////////////////////////////////////////////////

void fakeRev() {
	float timeStep = SYSTEM::TIMESTEP();
	float accelRatio = 2.5f * timeStep;
	float rpmValTemp = (vehData.PrevRpm > vehData.Rpm ? (vehData.PrevRpm - vehData.Rpm) : 0.0f);
	if (vehData.CurrGear == 1) {
		rpmValTemp *= 2.0f;
	}
	float rpmVal = vehData.Rpm + // Base value
		rpmValTemp + // Keep it constant
		controls.ThrottleVal * accelRatio; // Addition value, depends on delta T
	//showText(0.4, 0.4, 2.0, "FakeRev");
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
		if (controls.ClutchVal > 0.4f && 
			vehData.ControlAccelerate > 0.05f &&
		    !vehData.SimulatedNeutral && 
			// The next statement is a workaround for rolling back + brake + gear > 1 because it shouldn't rev then.
			// Also because we're checking on the game Control accel value and not the pedal position
			// TODO: Might wanna re-write with control.AccelVal instead of vehData.ControlAccelerate?
			!(vehData.Velocity < 0.0 && controls.BrakeVal > 0.1f && vehData.ControlAccelerate > 0.05f)) {
			fakeRev();
			ext.SetThrottle(vehicle, vehData.ControlAccelerate);
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
	if (vehData.SimulatedNeutral || controls.ClutchVal > 1.0 - settings.ClutchCatchpoint) {
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
	vehData.UpdateRpm();
	ext.SetClutch(vehicle, finalClutch);
}

void functionTruckLimiting() {
	// Save speed @ shift
	if (vehData.CurrGear < vehData.NextGear) {
		if (vehData.PrevGear <= vehData.CurrGear ||
			vehData.Velocity <= vehData.LockSpeed ||
			vehData.LockSpeed < 0.01f) {
			vehData.LockSpeed = vehData.Velocity;
		}
	}

	// Update speed
	if (vehData.CurrGear < vehData.NextGear) {
		vehData.LockSpeed = vehData.Velocity;
		vehData.LockTruck = true;
	}

	// Limit
	if ((vehData.Velocity > vehData.LockSpeed && vehData.LockTruck) ||
		(vehData.Velocity > vehData.LockSpeed && vehData.PrevGear > vehData.CurrGear)) {
		controls.ClutchVal = 1.0f;
		vehData.TruckShiftUp = true;
	}
	else {
		vehData.TruckShiftUp = false;
	}
}

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Reverse/Pedal handling
///////////////////////////////////////////////////////////////////////////////

void functionRealReverse() {
	// Forward gear
	// Desired: Only brake
	if (vehData.CurrGear > 0) {
		// LT behavior when stopped: Just brake
		if (controls.BrakeVal > 0.01f && controls.ThrottleVal < controls.BrakeVal &&
		    vehData.Velocity < 0.5f && vehData.Velocity >= -0.5f) { // < 0.5 so reverse never triggers
			//showText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Stop");
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			ext.SetThrottleP(vehicle, 0.1f);
			ext.SetBrakeP(vehicle, 1.0f);
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
		}
		// LT behavior when rolling back: Brake
		if (controls.BrakeVal > 0.01f && controls.ThrottleVal < controls.BrakeVal &&
		    vehData.Velocity < -0.5f) {
			//showText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Rollback");
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, controls.BrakeVal);
			ext.SetThrottle(vehicle, 0.0f);
			ext.SetThrottleP(vehicle, 0.1f);
			ext.SetBrakeP(vehicle, 1.0f);
		}
		// RT behavior when rolling back: Burnout
		if (controls.ThrottleVal > 0.5f && vehData.Velocity < -1.0f) {
			//showText(0.3, 0.3, 0.5, "functionRealReverse: Throttle @ Rollback");
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, controls.ThrottleVal);
			if (controls.BrakeVal < 0.1f) {
				VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, false);
			}
		}
	}
	// Reverse gear
	// Desired: RT reverses, LT brakes
	if (vehData.CurrGear == 0) {
		// Enables reverse lights
		ext.SetThrottleP(vehicle, -0.1f);
		// RT behavior
		int throttleAndSomeBrake = 0;
		if (controls.ThrottleVal > 0.01f && controls.ThrottleVal > controls.BrakeVal) {
			throttleAndSomeBrake++;
			//showText(0.3, 0.3, 0.5, "functionRealReverse: Throttle @ Active Reverse");
			
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, controls.ThrottleVal);
		}
		// LT behavior when reversing
		if (controls.BrakeVal > 0.01f &&
			vehData.Velocity <= -0.5f) {
			throttleAndSomeBrake++;
			//showText(0.3, 0.35, 0.5, "functionRealReverse: Brake @ Reverse");

			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, controls.BrakeVal);
		}
		// Throttle > brake  && BrakeVal > 0.1f
		if (throttleAndSomeBrake >= 2) {
			//showText(0.3, 0.4, 0.5, "functionRealReverse: Weird combo + rev it");

			CONTROLS::ENABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, controls.BrakeVal);
			fakeRev();
		}

		// LT behavior when forward
		if (controls.BrakeVal > 0.01f && controls.ThrottleVal <= controls.BrakeVal &&
			vehData.Velocity > 0.1f) {
			//showText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Rollforwrd");

			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, controls.BrakeVal);

			//CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			ext.SetBrakeP(vehicle, 1.0f);
		}

		// LT behavior when still
		if (controls.BrakeVal > 0.01f && controls.ThrottleVal <= controls.BrakeVal &&
		    vehData.Velocity > -0.5f && vehData.Velocity <= 0.1f) {
			//showText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Stopped");

			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
			ext.SetBrakeP(vehicle, 1.0f);
		}
	}
}

// Forward gear: Throttle accelerates, Brake brakes (exclusive)
// Reverse gear: Throttle reverses, Brake brakes (exclusive)
void handlePedalsRealReverse(float wheelThrottleVal, float wheelBrakeVal) {
	float speedThreshold = 0.5f;
	float reverseThreshold = 1.0f;

	if (vehData.CurrGear > 0) {
		// Going forward
		if (vehData.Velocity > speedThreshold) {
			//showText(0.3, 0.0, 1.0, "We are going forward");
			// Throttle Pedal normal
			if (wheelThrottleVal > 0.01f) {
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, wheelThrottleVal);
			}
			// Brake Pedal normal
			if (wheelBrakeVal > 0.01f) {
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, wheelBrakeVal);
			}
		}

		// Standing still
		if (vehData.Velocity < speedThreshold && vehData.Velocity >= -speedThreshold) {
			//showText(0.3, 0.0, 1.0, "We are stopped");

			if (wheelThrottleVal > 0.01f) {
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, wheelThrottleVal);
			}

			if (wheelBrakeVal > 0.01f) {
				ext.SetThrottleP(vehicle, 0.1f);
				ext.SetBrakeP(vehicle, 1.0f);
				VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			}
		}

		// Rolling back
		if (vehData.Velocity < -speedThreshold) {
			bool brakelights = false;
			// Just brake
			if (wheelThrottleVal <= 0.01f && wheelBrakeVal > 0.01f) {
				//showText(0.3, 0.0, 1.0, "We should brake");
				//showText(0.3, 0.05, 1.0, ("Brake pressure:" + std::to_string(wheelBrakeVal)).c_str());
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, wheelBrakeVal);
				ext.SetThrottleP(vehicle, 0.1f);
				ext.SetBrakeP(vehicle, 1.0f);
				brakelights = true;
			}
			
			if (wheelThrottleVal > 0.01f && controls.ClutchVal < settings.ClutchCatchpoint) {
				//showText(0.3, 0.0, 1.0, "We should burnout");
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, wheelThrottleVal);
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, wheelThrottleVal);
			}

			if (wheelThrottleVal > 0.01f && controls.ClutchVal > settings.ClutchCatchpoint) {
				if (wheelBrakeVal > 0.01f) {
					//showText(0.3, 0.0, 1.0, "We should rev and brake");
					//showText(0.3, 0.05, 1.0, ("Brake pressure:" + std::to_string(wheelBrakeVal)).c_str() );
					CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, wheelBrakeVal);
					ext.SetThrottleP(vehicle, 0.1f);
					ext.SetBrakeP(vehicle, 1.0f);
					brakelights = true;
					fakeRev();
				}
				else {
					//showText(0.3, 0.0, 1.0, "We should rev and do nothing");
					ext.SetThrottleP(vehicle, wheelThrottleVal); 
					fakeRev();
				}
			}
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, brakelights);
		}
	}

	if (vehData.CurrGear == 0) {
		// Enables reverse lights
		ext.SetThrottleP(vehicle, -0.1f);
		
		// We're reversing
		if (vehData.Velocity < -speedThreshold) {
			//showText(0.3, 0.0, 1.0, "We are reversing");
			// Throttle Pedal Reverse
			if (wheelThrottleVal > 0.01f) {
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, wheelThrottleVal);
			}
			// Brake Pedal Reverse
			if (wheelBrakeVal > 0.01f) {
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, wheelBrakeVal);
				ext.SetThrottleP(vehicle, -wheelBrakeVal);
				ext.SetBrakeP(vehicle, 1.0f);
			}
		}

		// Standing still
		if (vehData.Velocity < speedThreshold && vehData.Velocity >= -speedThreshold) {
			//showText(0.3, 0.0, 1.0, "We are stopped");

			if (wheelThrottleVal > 0.01f) {
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, wheelThrottleVal);
			}

			if (wheelBrakeVal > 0.01f) {
				ext.SetThrottleP(vehicle, -wheelBrakeVal);
				ext.SetBrakeP(vehicle, 1.0f);
				VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			}
		}

		// We're rolling forwards
		if (vehData.Velocity > speedThreshold) {
			//showText(0.3, 0.0, 1.0, "We are rolling forwards");
			//bool brakelights = false;

			if (vehData.Velocity > reverseThreshold) {
				gearRattle.Play(vehicle);
				shiftTo(1, false);
				vehData.SimulatedNeutral = true;
				
				if (controls.ClutchVal > settings.ClutchCatchpoint) {
					CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, 1.0f);
				}
				if (settings.EngDamage) {
					VEHICLE::SET_VEHICLE_ENGINE_HEALTH(
						vehicle,
						VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) - settings.MisshiftDamage * 2);
				}
				//showNotification("Woops", nullptr);
			}

			//VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, brakelights);
		}
	}
}

// Pedals behave like RT/LT
void handlePedalsDefault(float wheelThrottleVal, float wheelBrakeVal) {
	if (wheelThrottleVal > 0.01f) {
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, wheelThrottleVal);
	}
	if (wheelBrakeVal > 0.01f) {
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
		settings.ThrottleStart && controls.ThrottleVal > 0.98f && controls.ClutchVal > settings.ClutchCatchpoint)) {
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, false, true);
	}
	if  (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) &&
		((controls.ButtonJustPressed(ScriptControls::ControllerControlType::Engine) && settings.ToggleEngine) ||
		controls.ButtonJustPressed(ScriptControls::KeyboardControlType::Engine) ||
		controls.ButtonJustPressed(ScriptControls::WheelControlType::Engine))) {
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
	}

	if (!controls.WheelControl.IsConnected(controls.SteerGUID) ||
		controls.PrevInput != ScriptControls::Wheel) {
		return;
	}

	if (controls.HandbrakeVal > 0.1f) {
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, controls.HandbrakeVal);
	}
	if (controls.ButtonIn(ScriptControls::WheelControlType::Handbrake)) {
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


	if (controls.ButtonHeld(ScriptControls::WheelControlType::RadioPrev) ||
		controls.ButtonHeld(ScriptControls::WheelControlType::RadioNext)) {
		if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() != RadioOff) {
			vehData.RadioStationIndex = AUDIO::GET_PLAYER_RADIO_STATION_INDEX();
		}
		AUDIO::SET_VEH_RADIO_STATION(vehicle, "OFF");
		return;
	}
	if (controls.ButtonReleased(ScriptControls::WheelControlType::RadioNext)) {
		if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() == RadioOff) {
			AUDIO::SET_RADIO_TO_STATION_INDEX(vehData.RadioStationIndex);
			return;
		}
		AUDIO::_0xFF266D1D0EB1195D(); // Next radio station
	}
	if (controls.ButtonReleased(ScriptControls::WheelControlType::RadioPrev)) {
		if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() == RadioOff) {
			AUDIO::SET_RADIO_TO_STATION_INDEX(vehData.RadioStationIndex);
			return;
		}
		AUDIO::_0xDD6BCF9E94425DF9(); // Prev radio station
	}


	if (controls.ButtonJustPressed(ScriptControls::WheelControlType::IndicatorLeft)) {
		if (!vehData.BlinkerLeft) {
			vehData.BlinkerTicks = 1;
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false); // L
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, true); // R
			vehData.BlinkerLeft = true;
			vehData.BlinkerRight = false;
			vehData.BlinkerHazard = false;
		}
		else {
			vehData.BlinkerTicks = 0;
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
			vehData.BlinkerLeft = false;
			vehData.BlinkerRight = false;
			vehData.BlinkerHazard = false;
		}
	}
	if (controls.ButtonJustPressed(ScriptControls::WheelControlType::IndicatorRight)) {
		if (!vehData.BlinkerRight) {
			vehData.BlinkerTicks = 1;
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, true); // L
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false); // R
			vehData.BlinkerLeft = false;
			vehData.BlinkerRight = true;
			vehData.BlinkerHazard = false;
		}
		else {
			vehData.BlinkerTicks = 0;
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
			vehData.BlinkerLeft = false;
			vehData.BlinkerRight = false;
			vehData.BlinkerHazard = false;
		}
	}

	float centerPos = 0.5f;//static_cast<float>(controls.SteerLeft + controls.SteerRight) / 2;
	float wheelCenterDeviation = controls.SteerVal - centerPos;

	if (vehData.BlinkerTicks == 1 && abs(wheelCenterDeviation/ centerPos) > 0.2f)
	{
		vehData.BlinkerTicks = 2;
	}

	if (vehData.BlinkerTicks == 2 && abs(wheelCenterDeviation / centerPos) < 0.1f)
	{
		vehData.BlinkerTicks = 0;
		VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
		VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
		vehData.BlinkerLeft = false;
		vehData.BlinkerRight = false;
		vehData.BlinkerHazard = false;
	}

	if (controls.ButtonJustPressed(ScriptControls::WheelControlType::IndicatorHazard)) {
		if (!vehData.BlinkerHazard) {
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, true); // L
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, true); // R
			vehData.BlinkerLeft = false;
			vehData.BlinkerRight = false;
			vehData.BlinkerHazard = true;
		}
		else {
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
			VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
			vehData.BlinkerLeft = false;
			vehData.BlinkerRight = false;
			vehData.BlinkerHazard = false;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//                    Wheel input and force feedback
///////////////////////////////////////////////////////////////////////////////

void doWheelSteering() {
	if (controls.PrevInput == ScriptControls::InputDevices::Wheel) {
		float steerMult;
		if (vehData.Class == VehicleData::VehicleClass::Bike || vehData.Class == VehicleData::VehicleClass::Quad)
			steerMult = settings.SteerAngleMax / settings.SteerAngleBike;
		else if (vehData.Class == VehicleData::VehicleClass::Car)
			steerMult = settings.SteerAngleMax / settings.SteerAngleCar;
		else {
			steerMult = settings.SteerAngleMax / settings.SteerAngleAlt;
		}

		float effSteer = steerMult * 2.0f * (controls.SteerVal - 0.5f);

		ext.SetSteeringInputAngle(vehicle, -effSteer);
		CONTROLS::_SET_CONTROL_NORMAL(27, ControlVehicleMoveLeftRight, effSteer);
	}
}

void doWheelSteeringBoat() {
	float steerMult = settings.SteerAngleMax / settings.SteerAngleAlt;
	float effSteer = steerMult * 2.0f * (controls.SteerVal - 0.5f);

	ext.SetSteeringInputAngle(vehicle, -effSteer);
	CONTROLS::_SET_CONTROL_NORMAL(27, ControlVehicleMoveLeftRight, effSteer);
}

void doWheelSteeringPlane() {

	if (controls.ButtonIn(ScriptControls::WheelControlType::H1))
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleFlyThrottleUp, 0.33f);

	if (controls.ButtonIn(ScriptControls::WheelControlType::H3))
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleFlyThrottleUp, 0.66f);

	if (controls.ButtonIn(ScriptControls::WheelControlType::H5))
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleFlyThrottleUp, 1.0f);

	if (controls.ButtonIn(ScriptControls::WheelControlType::H2))
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleFlyThrottleDown, 0.33f);

	if (controls.ButtonIn(ScriptControls::WheelControlType::H4))
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleFlyThrottleDown, 0.66f);

	if (controls.ButtonIn(ScriptControls::WheelControlType::H6))
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleFlyThrottleDown, 1.0f);

	float steerMult = settings.SteerAngleMax / settings.SteerAngleAlt;
	float effSteer = steerMult * 2.0f * (controls.SteerVal - 0.5f);


	if (controls.ButtonIn(ScriptControls::WheelControlType::ShiftUp))
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleFlyPitchUpDown, 1.0f);

	if (controls.ButtonIn(ScriptControls::WheelControlType::ShiftDown))
		CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleFlyPitchUpDown, -1.0f);

	CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleFlyYawLeft, controls.ClutchVal);
	CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleFlyYawRight, controls.ThrottleVal);

	//////////////////////////////////////////////////////////////////////////////

	float antiDeadzoned = effSteer;
	if (effSteer < -0.02) {
		antiDeadzoned = effSteer - 0.20f;
	}
	if (effSteer > 0.02) {
		antiDeadzoned = effSteer + 0.20f;
	}
	CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleFlyRollLeftRight, antiDeadzoned);
}

void playWheelEffects(ScriptSettings& settings, VehicleData& vehData, bool airborne, bool ignoreSpeed) {
	auto steerAxis = controls.WheelControl.StringToAxis(controls.WheelAxes[static_cast<int>(controls.SteerAxisType)]);

	if (!controls.WheelControl.IsConnected(controls.SteerGUID) ||
		controls.PrevInput != ScriptControls::Wheel ||
		!settings.EnableFFB) {
		return;
	}

	if (settings.LogiLEDs) {
		controls.WheelControl.PlayLedsDInput(controls.SteerGUID, vehData.Rpm, 0.45, 0.95);
	}

	Vector3 accelVals = vehData.getAccelerationVectors(vehData.V3Velocities);
	Vector3 accelValsAvg = vehData.getAccelerationVectorsAverage();

	float steerMult;
	if (vehData.Class == VehicleData::VehicleClass::Bike || vehData.Class == VehicleData::VehicleClass::Quad)
		steerMult = settings.SteerAngleMax / settings.SteerAngleBike;
	else if (vehData.Class == VehicleData::VehicleClass::Car)
		steerMult = settings.SteerAngleMax / settings.SteerAngleCar;
	else {
		steerMult = settings.SteerAngleMax / settings.SteerAngleAlt;
	}
	float effSteer = steerMult * 2.0f * (controls.SteerVal - 0.5f);
	
	// targetSpeed is the speed at which the damperForce is at minimum
	// damperForce is maximum at speed 0 and decreases with speed increase
	float adjustRatio = static_cast<float>(settings.DamperMax) / static_cast<float>(settings.TargetSpeed);
	int damperForce = settings.DamperMax - static_cast<int>(vehData.Speed * adjustRatio);

	// Acceleration also affects damper force
	damperForce -= static_cast<int>(adjustRatio * accelValsAvg.y * std::copysignf(1.0, vehData.Velocity));
	
	if (damperForce < settings.DamperMin) {
		damperForce = settings.DamperMin;
	}

	// steerSpeed is to dampen the steering wheel
	auto steerSpeed = controls.WheelControl.GetAxisSpeed(steerAxis, controls.SteerGUID) / 20;

	/*                    a                                        v^2
	 * Because G Force = ---- and a = v * omega, verified with a = ---   using a speedo and 
	 *                   9.81                                       r    r = ypg205t16a length
	 * No need to crappily emulate centering any more.
	 * Do: Find something for self-aligning torque
	 */
	double GForce = (vehData.RotationVelocity.z * vehData.Velocity) / 9.81f;

	// Oversteer  - when rear  wheels are outside the turning radius
	// Understeer - when front wheels are more angled than turning radius
	// Neutral    - both front rear   are tracking true

	// start oversteer detect
	float oversteer = 0.0f;
	
	float angle = acos(vehData.Velocity / vehData.Speed)* 180.0f / 3.14159265f;
	if (isnan(angle))
		angle = 0.0;

	if (angle > 10 && vehData.Velocity > 1.0f) {
		oversteer = std::min(1.0f, angle/90.0f);
	}
	// end oversteer detect

	// begin understeer detect
	float understeer = 0.0f;
	if (abs(vehData.SteeringAngle*std::sqrt(vehData.Velocity)) > abs(vehData.RotationVelocity.z) &&
		vehData.Velocity > 0.1f) {
		understeer = std::min(1.0f, abs(vehData.SteeringAngle*sqrt(vehData.Velocity) - vehData.RotationVelocity.z));
	}
	// end understeer detect

	// On understeering conditions, lower "grippy" feel
	GForce = std::min(1.0f, std::max(0.0f, 1.0f - understeer + oversteer)) * GForce;

	// Simulate caster instability
	//if (vehData.Velocity < -0.1f) {
	//	GForce = -GForce;
	//}

	// Detail feel / suspension compression based
	float compSpeedTotal = 0.0f;
	auto compSpeed = vehData.GetWheelCompressionSpeeds();

	// More than 2 wheels! Trikes should be ok, etc.
	if (ext.GetNumWheels(vehicle) > 2) {
		// left should pull left, right should pull right
		compSpeedTotal = -compSpeed[0] + compSpeed[1];
	}

	// Cancel all effects except dampening
	if (airborne) {
		GForce = 0.0;
		damperForce = settings.DamperMin;
	}

	if (ignoreSpeed) {
		damperForce = settings.DamperMin;
	}

	if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
		damperForce *= 4;
	}

	if (vehData.Class == VehicleData::VehicleClass::Car || vehData.Class == VehicleData::VehicleClass::Quad) {
		if (VEHICLE::IS_VEHICLE_TYRE_BURST(vehicle, 0, true) && VEHICLE::IS_VEHICLE_TYRE_BURST(vehicle, 1, true)) {
			GForce = GForce * 0.1;
			damperForce = settings.DamperMin;
		}
	}
	if (vehData.Class == VehicleData::VehicleClass::Bike) {
		if (VEHICLE::IS_VEHICLE_TYRE_BURST(vehicle, 0, true)) {
			GForce = GForce * 0.1;
			damperForce = settings.DamperMin;
		}
	}

	int totalForce = 
		static_cast<int>(-GForce * 5000 * settings.PhysicsStrength * settings.FFGlobalMult) + // 2G = max force, Koenigsegg One:1 only does 1.7g!
		static_cast<int>(1000.0f * settings.DetailStrength * compSpeedTotal * settings.FFGlobalMult) +
		static_cast<int>(steerSpeed * damperForce * 0.1) +
		0;

	// Soft lock
	if (effSteer > 1.0f) {
		totalForce = static_cast<int>((effSteer - 1.0f) * 100000) + totalForce;
		if (effSteer > 1.1f) {
			totalForce = 10000;
		}
	} else if (effSteer < -1.0f) {
		totalForce = static_cast<int>((-effSteer - 1.0f) * -100000) + totalForce;
		if (effSteer < -1.1f) {
			totalForce = -10000;
		}
	}
	controls.WheelControl.SetConstantForce(controls.SteerGUID, totalForce);

	if (settings.DisplayInfo) {
		showDebugInfoWheel(settings, effSteer, damperForce, steerSpeed, GForce, oversteer, understeer);
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

void functionAutoGear1() {
	if (vehData.Throttle < 0.1f && vehData.Speed < 0.1f && vehData.CurrGear > 1) {
		vehData.LockGears = 0x00010001;
	}
}

///////////////////////////////////////////////////////////////////////////////
//                             Menu stuff
///////////////////////////////////////////////////////////////////////////////
void menuInit() {
	
}

void menuClose() {
	settings.SaveGeneral();
	settings.SaveWheel();
	settings.SaveController(&controls);
	menu.SaveMenuTheme(std::wstring(settingsMenuFile.begin(), settingsMenuFile.end()).c_str());
}

void update_menu() {
	menu.CheckKeys(&menuControls, std::bind(menuInit), std::bind(menuClose));

	/* Yes hello I am root */
	if (menu.CurrentMenu("mainmenu")) {
		menu.Title("Manual Transmission");
		bool tempEnableRead = settings.EnableManual;
		if (menu.BoolOption("Enable manual transmission", &tempEnableRead)) { toggleManual(); } 
		
		int shiftModeTemp = settings.ShiftMode;
		std::vector<std::string> gearboxModes = { 
			"Sequential",
			"H-pattern",
			"Automatic"
		};
		menu.StringArray("Gearbox", gearboxModes, &shiftModeTemp);
		if (shiftModeTemp != settings.ShiftMode) {
			settings.ShiftMode = shiftModeTemp;
			setShiftMode(shiftModeTemp);
		}

		menu.MenuOption("Mod options", "optionsmenu");
		menu.MenuOption("Controls", "controlsmenu");
		menu.MenuOption("Wheel Options", "wheelmenu");
		menu.MenuOption("HUD Options", "hudmenu");
		menu.MenuOption("Menu Options", "menumenu"); 
		menu.MenuOption("Debug", "debugmenu");

		int activeIndex = 0;
		std::string activeInputName;
		switch (controls.PrevInput) {
			case ScriptControls::Keyboard:
				activeInputName = "Keyboard";
				break;
			case ScriptControls::Controller:
				activeInputName = "Controller";
				break;
			case ScriptControls::Wheel:
				activeInputName = "Wheel";
				break;
		}
		std::vector<std::string> active = { activeInputName };
		menu.StringArray("Active input", active, &activeIndex);

		int versionIndex = 0;
		std::vector<std::string> version = { DISPLAY_VERSION };
		menu.StringArray("Version", version, &versionIndex);
	}

	/* Yes hello I am root - 1 */
	if (menu.CurrentMenu("optionsmenu")) {
		menu.Title("Mod options");
		if (menu.BoolOption("Simple Bike", &settings.SimpleBike)) {}
		if (menu.BoolOption("Engine Damage", &settings.EngDamage)) {}
		if (menu.BoolOption("Engine Stalling", &settings.EngStall)) {}
		if (menu.BoolOption("Engine Braking", &settings.EngBrake)) {}
		if (menu.BoolOption("Clutch Grabbing", &settings.ClutchCatching)) {}
		if (menu.BoolOption("Clutch Shift (S)", &settings.ClutchShiftingS)) {}
		if (menu.BoolOption("Clutch Shift (H)", &settings.ClutchShiftingH)) {}
		if (menu.BoolOption("Default Neutral", &settings.DefaultNeutral)) {}
		if (menu.FloatOption("Clutch bite point", &settings.ClutchCatchpoint, 0.0f, 1.0f, 0.05f)) {}
		if (menu.FloatOption("Stalling threshold", &settings.StallingThreshold, 0.0f, 1.0f, 0.05f)) {}
		if (menu.FloatOption("RPM Damage", &settings.RPMDamage, 0.0f, 10.0f, 0.05f)) {}
		if (menu.IntOption("Misshift Damage", &settings.MisshiftDamage, 0, 100, 5)) {}
		if (menu.BoolOption("Hill gravity workaround", &settings.HillBrakeWorkaround)) {}
		if (menu.BoolOption("Auto gear 1", &settings.AutoGear1)) {}
		if (menu.BoolOption("Auto look back", &settings.AutoLookBack)) {}
		if (menu.BoolOption("Clutch + throttle start", &settings.ThrottleStart)) {}
	}

	/* Yes hello I am root - 1 */
	if (menu.CurrentMenu("controlsmenu")) {
		menu.Title("Controls");
		
		menu.MenuOption("Controller", "controllermenu");
		menu.MenuOption("Keyboard", "keyboardmenu");
	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("controllermenu")) {
		menu.Title("Controller controls");
		
		if (menu.BoolOption("Engine button turns off too", &settings.ToggleEngine)) {}
		if (menu.IntOption("Long press time (ms)", &controls.CToggleTime, 100, 5000, 100)) {}

		// wtf ikt
		float currTriggerValue = controls.GetXboxTrigger();
		float prevTriggerValue = currTriggerValue;
		if (menu.FloatOption("Trigger value", &currTriggerValue, 0.25, 1.0, 0.05)) {}
		if (currTriggerValue != prevTriggerValue) {
			controls.SetXboxTrigger(currTriggerValue);
		}

		std::vector<std::string> controllerInfo;
		controllerInfo.push_back("Press RIGHT to clear key");
		controllerInfo.push_back("Press RETURN to configure button");
		controllerInfo.push_back("");

		auto it = 0;
		for (auto confTag : controllerConfTags) {
			controllerInfo.back() = controllerConfTagDetail.at(it);
			controllerInfo.push_back("Assigned to " + controls.ConfTagController2Value(confTag));
			if (menu.OptionPlus(CharAdapter(("Assign " + confTag).c_str()), controllerInfo, nullptr, std::bind(clearControllerButton, confTag), nullptr)) {
				bool result = configControllerButton(confTag);
				//showNotification(result ? (confTag + " saved").c_str() : ("Cancelled " + confTag + " assignment").c_str(), &prevNotification);
				if (!result) showNotification(("Cancelled " + confTag + " assignment").c_str(), &prevNotification);
				WAIT(1000);
			}
			it++;
			controllerInfo.pop_back();
		}
	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("keyboardmenu")) {
		menu.Title("Keyboard controls");

		std::vector<std::string> keyboardInfo;
		keyboardInfo.push_back("Press RIGHT to clear key");
		keyboardInfo.push_back("Press RETURN to configure key");
		keyboardInfo.push_back("");

		int it = 0;
		for (auto confTag : keyboardConfTags) {
			keyboardInfo.back() = keyboardConfTagsDetail.at(it);
			keyboardInfo.push_back("Assigned to " + key2str(controls.ConfTagKB2key(confTag)));
			if (menu.OptionPlus(CharAdapter(("Assign " + confTag).c_str()), keyboardInfo, nullptr, std::bind(clearKeyboardKey, confTag), nullptr)) {
				bool result = configKeyboardKey(confTag);
				if (!result) showNotification(("Cancelled " + confTag + " assignment").c_str(), &prevNotification);
				WAIT(1000);
			}
			it++;
			keyboardInfo.pop_back();
		}
	}

	/* Yes hello I am root - 1 */
	if (menu.CurrentMenu("wheelmenu")) {
		menu.Title("Wheel options");
		if (menu.BoolOption("Enable wheel", &settings.EnableWheel)) { settings.SaveWheel(); }
		if (menu.BoolOption("Enable wheel without MT", &settings.WheelWithoutManual)) { settings.SaveWheel(); }
		if (menu.BoolOption("Enable wheel for boats & planes", &settings.AltControls)) { settings.SaveWheel(); }
		if (menu.BoolOption("Patch steering", &settings.PatchSteering)) { settings.SaveWheel(); }
		if (menu.BoolOption("Patch steering for all inputs", &settings.PatchSteeringAlways)) { settings.SaveWheel(); }
		if (menu.BoolOption("Logitech LEDs (can crash!)", &settings.LogiLEDs)) { settings.SaveWheel(); }
		menu.MenuOption("Force feedback options", "forcefeedbackmenu");
		menu.MenuOption("Steering wheel setup", "axesmenu");
		menu.MenuOption("Steering wheel angles", "anglemenu");
		menu.MenuOption("Steering wheel buttons", "buttonsmenu");
		
		std::vector<std::string> hpatInfo;
		hpatInfo.push_back("Press RIGHT to clear H-pattern shifter");
		hpatInfo.push_back("Active gear:");
		if (controls.ButtonIn(ScriptControls::WheelControlType::HR)) hpatInfo.push_back("Reverse");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H1)) hpatInfo.push_back("Gear 1");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H2)) hpatInfo.push_back("Gear 2");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H3)) hpatInfo.push_back("Gear 3");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H4)) hpatInfo.push_back("Gear 4");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H5)) hpatInfo.push_back("Gear 5");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H6)) hpatInfo.push_back("Gear 6");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H7)) hpatInfo.push_back("Gear 7");

		if (menu.OptionPlus("H-pattern shifter", hpatInfo, nullptr, std::bind(clearHShifter), nullptr)) {
			bool result = configHPattern();
			showNotification(result ? "H-pattern shifter saved" : "Cancelled H-pattern shifter setup", &prevNotification);
		}
	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("anglemenu")) {
		menu.Title("Wheel angles");

		if (menu.FloatOption("Physical degrees", &settings.SteerAngleMax, 180.0, 1080.0, 60.0)) {
			if (settings.SteerAngleCar > settings.SteerAngleMax) { settings.SteerAngleCar = settings.SteerAngleMax; }
			if (settings.SteerAngleBike > settings.SteerAngleMax) { settings.SteerAngleBike = settings.SteerAngleMax; }
			if (settings.SteerAngleAlt > settings.SteerAngleMax) { settings.SteerAngleAlt = settings.SteerAngleMax; }
		}
		menu.FloatOption("Car soft lock", &settings.SteerAngleCar, 180.0, settings.SteerAngleMax, 60.0);
		menu.FloatOption("Bike soft lock", &settings.SteerAngleBike, 180.0, settings.SteerAngleMax, 60.0);
		menu.FloatOption("Boat/Plane soft lock", &settings.SteerAngleAlt, 180.0, settings.SteerAngleMax, 60.0);

	}

	
	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("axesmenu")) {
		menu.Title("Configure axes");
		
		std::vector<std::string> info = {
			"Press RIGHT to clear this axis" ,
			"Steer    : " + std::to_string(controls.SteerVal),
			"Throttle : " + std::to_string(controls.ThrottleVal),
			"Brake    : " + std::to_string(controls.BrakeVal),
			"Clutch   : " + std::to_string(controls.ClutchVal),
			"Handbrake: " + std::to_string(controls.HandbrakeVal),
		};

		if (menu.OptionPlus("Calibrate steering", info, nullptr, std::bind(clearAxis, "STEER"), nullptr)) {
			bool result = configAxis("STEER");
			showNotification(result ? "Steering axis saved" : "Cancelled steering axis calibration", &prevNotification);
		}
		if (menu.OptionPlus("Calibrate throttle", info, nullptr, std::bind(clearAxis, "THROTTLE"), nullptr)) {
			bool result = configAxis("THROTTLE");
			showNotification(result ? "Throttle axis saved" : "Cancelled throttle axis calibration", &prevNotification);
		}
		if (menu.OptionPlus("Calibrate brake", info, nullptr, std::bind(clearAxis, "BRAKES"), nullptr)) {
			bool result = configAxis("BRAKES");
			showNotification(result ? "Brake axis saved" : "Cancelled brake axis calibration", &prevNotification);
		}
		if (menu.OptionPlus("Calibrate clutch", info, nullptr, std::bind(clearAxis, "CLUTCH"), nullptr)) {
			bool result = configAxis("CLUTCH");
			showNotification(result ? "Clutch axis saved" : "Cancelled clutch axis calibration", &prevNotification);
		}
		if (menu.OptionPlus("Calibrate handbrake", info, nullptr, std::bind(clearAxis, "HANDBRAKE_ANALOG"), nullptr)) {
			bool result = configAxis("HANDBRAKE_ANALOG");
			showNotification(result ? "Handbrake axis saved" : "Cancelled handbrake axis calibration", &prevNotification);
		}
	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("forcefeedbackmenu")) {
		menu.Title("Force feedback");

		menu.BoolOption("Enable", &settings.EnableFFB);
		menu.FloatOption("Global multiplier", &settings.FFGlobalMult, 0.0f, 10.0f, 1.0f);
		menu.IntOption("Damper Max (low speed)", &settings.DamperMax, 0, 200, 1);
		menu.IntOption("Damper Min (high speed)", &settings.DamperMin, 0, 200, 1);
		menu.FloatOption("Damper Min speed (m/s)", &settings.TargetSpeed, 0.0f, 40.0f, 0.2f);
		menu.FloatOption("Physics strength", &settings.PhysicsStrength, 0.0f, 10.0f, 0.1f);
		menu.FloatOption("Detail strength", &settings.DetailStrength, 0.0f, 10.0f, 0.1f);
	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("buttonsmenu")) {
		menu.Title("Configure buttons");
		std::vector<std::string> info;
		info.push_back("Press RIGHT to clear this button");
		info.push_back("Active buttons:");
		if (controls.ButtonIn(ScriptControls::WheelControlType::HR)) info.push_back("Gear R");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H1)) info.push_back("Gear 1");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H2)) info.push_back("Gear 2");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H3)) info.push_back("Gear 3");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H4)) info.push_back("Gear 4");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H5)) info.push_back("Gear 5");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H6)) info.push_back("Gear 6");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H7)) info.push_back("Gear 7");
		if (controls.ButtonIn(ScriptControls::WheelControlType::ShiftUp))	info.push_back("ShiftUp");
		if (controls.ButtonIn(ScriptControls::WheelControlType::ShiftDown)) info.push_back("ShiftDown");
		if (controls.ButtonIn(ScriptControls::WheelControlType::Engine))	info.push_back("Engine");
		if (controls.ButtonIn(ScriptControls::WheelControlType::Handbrake)) info.push_back("Handbrake");
		if (controls.ButtonIn(ScriptControls::WheelControlType::Horn))		info.push_back("Horn");
		if (controls.ButtonIn(ScriptControls::WheelControlType::Lights))	info.push_back("Lights");
		if (controls.ButtonIn(ScriptControls::WheelControlType::LookBack))	info.push_back("LookBack");
		if (controls.ButtonIn(ScriptControls::WheelControlType::Camera))	info.push_back("Camera");
		if (controls.ButtonIn(ScriptControls::WheelControlType::RadioNext)) info.push_back("RadioNext");
		if (controls.ButtonIn(ScriptControls::WheelControlType::RadioPrev)) info.push_back("RadioPrev");
		if (controls.ButtonIn(ScriptControls::WheelControlType::IndicatorLeft))		info.push_back("IndicatorLeft");
		if (controls.ButtonIn(ScriptControls::WheelControlType::IndicatorRight))	info.push_back("IndicatorRight");
		if (controls.ButtonIn(ScriptControls::WheelControlType::IndicatorHazard))	info.push_back("IndicatorHazard");
		if (controls.ButtonIn(ScriptControls::WheelControlType::Toggle))	info.push_back("ToggleMod");
		if (controls.ButtonIn(ScriptControls::WheelControlType::ToggleH))	info.push_back("ChangeShiftMode");

		for (auto confTag : buttonConfTags) {
			if (menu.OptionPlus(CharAdapter(("Assign " + confTag).c_str()), info, nullptr, std::bind(clearButton, confTag), nullptr)) {
				bool result = configButton(confTag);
				showNotification(result ? (confTag + " saved").c_str() : ("Cancelled " + confTag + " assignment").c_str(), &prevNotification);
			}
		}
	}
	
	/* Yes hello I am root - 1 */
	if (menu.CurrentMenu("hudmenu")) {
		menu.Title("HUD Options");

		menu.BoolOption("Enable", &settings.HUD);
		menu.MenuOption("Gear and shift mode", "geardisplaymenu");
		menu.MenuOption("Speedometer", "speedodisplaymenu");
		menu.MenuOption("RPM Gauge", "rpmdisplaymenu");

	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("geardisplaymenu")) {
		menu.Title("Gear options");

		// prolly gear section
		menu.FloatOption("Gear X", &settings.GearXpos, 0.0f, 1.0f, 0.005f);
		menu.FloatOption("Gear Y", &settings.GearYpos, 0.0f, 1.0f, 0.005f);
		menu.FloatOption("Gear Size", &settings.GearSize, 0.0f, 3.0f, 0.05f);
		menu.IntOption("Gear Top Color Red", &settings.GearTopColorR, 0, 255);
		menu.IntOption("Gear Top Color Green", &settings.GearTopColorG, 0, 255);
		menu.IntOption("Gear Top Color Blue", &settings.GearTopColorB, 0, 255);

		menu.FloatOption("Shift Mode X", &settings.ShiftModeXpos, 0.0f, 1.0f, 0.005f);
		menu.FloatOption("Shift Mode Y", &settings.ShiftModeYpos, 0.0f, 1.0f, 0.005f);
		menu.FloatOption("Shift Mode Size", &settings.ShiftModeSize, 0.0f, 3.0f, 0.05f);

	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("speedodisplaymenu")) {
		menu.Title("Speedometer options");
		// prolly speedo section
		ptrdiff_t oldPos = std::find(speedoTypes.begin(), speedoTypes.end(), settings.Speedo) - speedoTypes.begin();
		menu.StringArray("Speedometer", speedoTypes, &speedoIndex);
		if (speedoIndex != oldPos) {
			settings.Speedo = speedoTypes.at(speedoIndex);
		}
		menu.BoolOption("Show units", &settings.SpeedoShowUnit);

		menu.FloatOption("Speedometer X", &settings.SpeedoXpos, 0.0f, 1.0f, 0.005f);
		menu.FloatOption("Speedometer Y", &settings.SpeedoYpos, 0.0f, 1.0f, 0.005f);
		menu.FloatOption("Speedometer Size", &settings.SpeedoSize, 0.0f, 3.0f, 0.05f);

	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("rpmdisplaymenu")) {
		menu.Title("RPM Gauge options");
		// prolly RPM section
		menu.BoolOption("RPM Gauge", &settings.RPMIndicator);
		menu.FloatOption("RPM Redline", &settings.RPMIndicatorRedline, 0.0f, 1.0f, 0.01f);

		menu.FloatOption("RPM X", &settings.RPMIndicatorXpos, 0.0f, 1.0f, 0.0025f);
		menu.FloatOption("RPM Y", &settings.RPMIndicatorYpos, 0.0f, 1.0f, 0.0025f);
		menu.FloatOption("RPM Width", &settings.RPMIndicatorWidth, 0.0f, 1.0f, 0.0025f);
		menu.FloatOption("RPM Height", &settings.RPMIndicatorHeight, 0.0f, 1.0f, 0.0025f);

		menu.IntOption("RPM Background Red	", &settings.RPMIndicatorBackgroundR, 0, 255);
		menu.IntOption("RPM Background Green", &settings.RPMIndicatorBackgroundG, 0, 255);
		menu.IntOption("RPM Background Blue	", &settings.RPMIndicatorBackgroundB, 0, 255);
		menu.IntOption("RPM Background Alpha", &settings.RPMIndicatorBackgroundA, 0, 255);
																			
		menu.IntOption("RPM Foreground Red	", &settings.RPMIndicatorForegroundR, 0, 255);
		menu.IntOption("RPM Foreground Green", &settings.RPMIndicatorForegroundG, 0, 255);
		menu.IntOption("RPM Foreground Blue	", &settings.RPMIndicatorForegroundB, 0, 255);
		menu.IntOption("RPM Foreground Alpha", &settings.RPMIndicatorForegroundA, 0, 255);

		menu.IntOption("RPM Redline Red		", &settings.RPMIndicatorRedlineR, 0, 255);
		menu.IntOption("RPM Redline Green	", &settings.RPMIndicatorRedlineG, 0, 255);
		menu.IntOption("RPM Redline Blue	", &settings.RPMIndicatorRedlineB, 0, 255);
		menu.IntOption("RPM Redline Alpha	", &settings.RPMIndicatorRedlineA, 0, 255);

		menu.IntOption("RPM Revlimit Red	", &settings.RPMIndicatorRevlimitR, 0, 255);
		menu.IntOption("RPM Revlimit Green	", &settings.RPMIndicatorRevlimitG, 0, 255);
		menu.IntOption("RPM Revlimit Blue	", &settings.RPMIndicatorRevlimitB, 0, 255);
		menu.IntOption("RPM Revlimit Alpha	", &settings.RPMIndicatorRevlimitA, 0, 255);
	}

	/* Yes hello I am root - 1 */
	if (menu.CurrentMenu("menumenu")) {
		menu.Title("Theme Options");
		
		menu.MenuOption("Title Text", "settings_theme_titletext");
		menu.MenuOption("Title Background", "settings_theme_titlerect");
		menu.MenuOption("Highlighted", "settings_theme_scroller");
		menu.MenuOption("Options Text", "settings_theme_options");
		menu.MenuOption("Options Background", "settings_theme_optionsrect");
	}

	if (menu.CurrentMenu("settings_theme_titletext")) {
		menu.Title("Title Text");
		
		int fontIndex = std::find(fontIDs.begin(), fontIDs.end(), menu.titleFont) - fontIDs.begin();
		int oldIndex = fontIndex;
		menu.StringArray("Font: ", fonts, &fontIndex);
		if (fontIndex != oldIndex) {
			menu.titleFont = fontIDs.at(fontIndex);
		}

		menu.IntOption("Red: ", &menu.titleText.r, 0, 255);
		menu.IntOption("Green: ", &menu.titleText.g, 0, 255);
		menu.IntOption("Blue: ", &menu.titleText.b, 0, 255);
		menu.IntOption("Alpha: ", &menu.titleText.a, 0, 255);
	}
	if (menu.CurrentMenu("settings_theme_titlerect")) {
		menu.Title("Title Background");

		menu.IntOption("Red: ", &menu.titleRect.r, 0, 255);
		menu.IntOption("Green: ", &menu.titleRect.g, 0, 255);
		menu.IntOption("Blue: ", &menu.titleRect.b, 0, 255);
		menu.IntOption("Alpha: ", &menu.titleRect.a, 0, 255);
	}
	if (menu.CurrentMenu("settings_theme_scroller")) {
		menu.Title("Highlighted");

		menu.IntOption("Red: ", &menu.scroller.r, 0, 255);
		menu.IntOption("Green: ", &menu.scroller.g, 0, 255);
		menu.IntOption("Blue: ", &menu.scroller.b, 0, 255);
		menu.IntOption("Alpha: ", &menu.scroller.a, 0, 255);
	}	
	if (menu.CurrentMenu("settings_theme_options")) {
		menu.Title("Options Text");

		int fontIndex = std::find(fontIDs.begin(), fontIDs.end(), menu.optionsFont) - fontIDs.begin();
		int oldIndex = fontIndex;
		menu.StringArray("Font: ", fonts, &fontIndex);
		if (fontIndex != oldIndex) {
			menu.optionsFont = fontIDs.at(fontIndex);
		}
		
		menu.IntOption("Red: ", &menu.options.r, 0, 255);
		menu.IntOption("Green: ", &menu.options.g, 0, 255);
		menu.IntOption("Blue: ", &menu.options.b, 0, 255);
		menu.IntOption("Alpha: ", &menu.options.a, 0, 255);
	}	
	if (menu.CurrentMenu("settings_theme_optionsrect")) {
		menu.Title("Options Background");
		
		menu.IntOption("Red: ", &menu.optionsrect.r, 0, 255);
		menu.IntOption("Green: ", &menu.optionsrect.g, 0, 255);
		menu.IntOption("Blue: ", &menu.optionsrect.b, 0, 255);
		menu.IntOption("Alpha: ", &menu.optionsrect.a, 0, 255);
	}

	/* Yes hello I am root - 1 */
	if (menu.CurrentMenu("debugmenu")) {
		menu.Title("Debug settings");
		if (menu.BoolOption("Display info", &settings.DisplayInfo)) {}
		if (menu.BoolOption("Log car address", &settings.LogCar)) {}
		if (menu.BoolOption("Expose script variables", &settings.CrossScript)) {}
	}


	menu.EndMenu();
}


///////////////////////////////////////////////////////////////////////////////
//                              Config things
///////////////////////////////////////////////////////////////////////////////
bool getConfigAxisWithValues(std::vector<std::tuple<GUID, std::string, int>> startStates, std::tuple<GUID, std::string> &selectedDevice, int hyst, bool &positive, int &startValue_) {
	for (auto guid : controls.WheelControl.GetGuids()) {
		for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
			for (auto startState : startStates) {
				std::string axisName = controls.WheelControl.DIAxisHelper[i];
				int axisValue = controls.WheelControl.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), guid);
				int startValue = std::get<2>(startState);
				if (std::get<0>(startState) == guid &&
					std::get<1>(startState) == axisName) {
					startValue_ = startValue;
					if (axisValue > startValue + hyst) { // 0 (min) - 65535 (max)
						selectedDevice = std::tuple<GUID, std::string>(guid, axisName);
						positive = true;
						return true;
					}
					if (axisValue < startValue - hyst) { // 65535 (min) - 0 (max)
						selectedDevice = std::tuple<GUID, std::string>(guid, axisName);
						positive = false;
						return true;
					}
				}
			}
		}
	}
	return false;
}

void saveAxis(const std::string &confTag, std::tuple<GUID, std::string> selectedDevice, int min, int max) {
	std::wstring wDevName = controls.WheelControl.FindEntryFromGUID(std::get<0>(selectedDevice))->diDeviceInstance.tszInstanceName;
	std::string devName = std::string(wDevName.begin(), wDevName.end());
	auto index = settings.SteeringAppendDevice(std::get<0>(selectedDevice), devName);
	settings.SteeringSaveAxis(confTag, index, std::get<1>(selectedDevice), min, max);
	if (confTag == "STEER") {
		settings.SteeringSaveFFBAxis(confTag, index, std::get<1>(selectedDevice));
	}
	settings.Read(&controls);
}

void saveButton(int button, std::string confTag, GUID devGUID) {
	std::wstring wDevName = controls.WheelControl.FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
	std::string devName = std::string(wDevName.begin(), wDevName.end()).c_str();
	auto index = settings.SteeringAppendDevice(devGUID, devName.c_str());
	settings.SteeringSaveButton(confTag, index, button);
	settings.Read(&controls);
}

void saveKeyboardKey(std::string confTag, std::string key) {
	settings.KeyboardSaveKey(confTag, key);
	settings.Read(&controls);
	showNotification(("Saved key " + confTag + ": " + key).c_str(), &prevNotification);
}

void saveControllerButton(std::string confTag, std::string button) {
	settings.ControllerSaveButton(confTag, button);
	settings.Read(&controls);
	showNotification(("Saved button " + confTag + ": " + button).c_str(), &prevNotification);
}

void saveHShifter(std::string confTag, GUID devGUID, std::array<int, numGears> buttonArray, std::string devName) {
	auto index = settings.SteeringAppendDevice(devGUID, devName);
	settings.SteeringSaveHShifter(confTag, index, buttonArray.data());
	settings.Read(&controls);
}

void clearAxis(std::string confTag) {
	settings.SteeringSaveAxis(confTag, -1, "", 0, 0);
	settings.Read(&controls);
	showNotification(("Cleared axis " + confTag).c_str(), &prevNotification);
}

void clearButton(std::string confTag) {
	settings.SteeringSaveButton(confTag, -1, -1);
	settings.Read(&controls);
	showNotification(("Cleared button " + confTag).c_str(), &prevNotification);
}

void clearKeyboardKey(std::string confTag) {
	settings.KeyboardSaveKey(confTag, "UNKNOWN");
	settings.Read(&controls);
	showNotification(("Cleared key " + confTag).c_str(), &prevNotification);
}

void clearControllerButton(std::string confTag) {
	settings.ControllerSaveButton(confTag, "UNKNOWN");
	settings.Read(&controls);
	showNotification(("Cleared button " + confTag).c_str(), &prevNotification);
}

void clearHShifter() {
	int empty[numGears] = {};
	for (int i = 0; i < numGears; i++) {
		empty[i] = -1;
	}
	settings.SteeringSaveHShifter("SHIFTER", -1, empty);
	settings.Read(&controls);
	showNotification("Cleared H-pattern shifter", &prevNotification);
}

bool configAxis(std::string str) {
	
	std::string confTag = str;
	std::string additionalInfo = "Press " + escapeKey + " to exit.";

	if (str == "STEER") {
		additionalInfo += " Steer right to register axis.";
	} else if (str == "HANDBRAKE_ANALOG") {
		additionalInfo += " Fully pull and set back handbrake to register axis.";
	} else {
		additionalInfo += " Fully press and release the " + confTag + " pedal to register axis.";
	}

	controls.UpdateValues(ScriptControls::InputDevices::Wheel, false, true);
	// Save current state
	std::vector<std::tuple<GUID, std::string, int>> startStates;
	for (auto guid : controls.WheelControl.GetGuids()) {
		for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
			std::string axisName = controls.WheelControl.DIAxisHelper[i];
			int axisValue = controls.WheelControl.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), guid);
			startStates.push_back(std::tuple<GUID, std::string, int>(guid, axisName, axisValue));
		}
	}

	std::tuple<GUID, std::string> selectedDevice;

	// Ignore inputs before selection if they moved less than hyst
	int hyst = 65536 / 8;

	// To track direction of physical <-> digital value.
	bool positive = true;

	int startValue;
	int endValue;

	// going down!
	while (true) {
		if (IsKeyJustUp(str2key(escapeKey))) {
			return false;
		}
		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false, true);
		if (getConfigAxisWithValues(startStates, selectedDevice, hyst, positive, startValue)) {
			break;
		}
		showSubtitle(additionalInfo);
		WAIT(0);
	}

	// I'll just assume no manufacturer uses inane non-full range steering wheel values
	if (confTag == "STEER") {
		int min = (positive ? 0 : 65535);
		int max = (positive ? 65535 : 0);
		saveAxis(confTag, selectedDevice, min, max);
		return true;
	}

	int prevAxisValue = controls.WheelControl.GetAxisValue(controls.WheelControl.StringToAxis(std::get<1>(selectedDevice)), std::get<0>(selectedDevice));
	std::wstring wDevName = controls.WheelControl.FindEntryFromGUID(std::get<0>(selectedDevice))->diDeviceInstance.tszInstanceName;
	std::string selectedDevName = std::string(wDevName.begin(), wDevName.end()).c_str();
	std::string selectedAxis = std::get<1>(selectedDevice);
	GUID selectedGUID = std::get<0>(selectedDevice);

	// and up again!
	while (true) {
		if (IsKeyJustUp(str2key(escapeKey))) {
			return false;
		}
		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false, true);

		int axisValue = controls.WheelControl.GetAxisValue(controls.WheelControl.StringToAxis(selectedAxis), selectedGUID);

		if (positive && axisValue < prevAxisValue) {
			endValue = prevAxisValue;
			break;
		}

		if (!positive && axisValue > prevAxisValue) {
			endValue = prevAxisValue;
			break;
		}

		prevAxisValue = axisValue;

		showSubtitle(additionalInfo);
		WAIT(0);
	}

	int min = startValue;
	int max = endValue;
	saveAxis(confTag, selectedDevice, min, max);
	return true;
}

bool configButton(std::string str) {
	int buttonsActive = 0;

	std::array<int, 8> directions = {
		WheelDirectInput::POV::N,
		WheelDirectInput::POV::NE,
		WheelDirectInput::POV::E,
		WheelDirectInput::POV::SE,
		WheelDirectInput::POV::S,
		WheelDirectInput::POV::SW,
		WheelDirectInput::POV::W,
		WheelDirectInput::POV::NW,
	};

	std::string confTag = str;
	std::string additionalInfo = "Press " + escapeKey + " to exit.";
	additionalInfo += " Press a button to set " + confTag + ".";

	controls.UpdateValues(ScriptControls::InputDevices::Wheel, false, true);
	
	for (auto guid : controls.WheelControl.GetGuids()) {
		for (int i = 0; i < 255; i++) {
			if (controls.WheelControl.IsButtonPressed(i, guid)) {
				buttonsActive++;
			}
		}
		for (auto d : directions) {
			if (controls.WheelControl.IsButtonPressed(d, guid)) {
				buttonsActive++;
			}
		}
	}

	if (buttonsActive > 0) {
		showSubtitle("One or more buttons had been pressed on start. Stop pressing and try again.");
		return false;
	}

	while (true) {
		if (IsKeyJustUp(str2key(escapeKey))) {
			return false;
		}
		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false, true);

		for (auto guid : controls.WheelControl.GetGuids()) {
			for (int i = 0; i < 255; i++) {
				if (controls.WheelControl.IsButtonPressed(i, guid)) {
					saveButton(i, confTag, guid);
					return true;
				}
			}

			//POV hat shit
			std::string directionsStr = "?";
			for (auto d : directions) {
				if (controls.WheelControl.IsButtonPressed(d, guid)) {
					if (d == WheelDirectInput::N) directionsStr = "N ";
					if (d == WheelDirectInput::NE) directionsStr = "NE";
					if (d == WheelDirectInput::E) directionsStr = " E";
					if (d == WheelDirectInput::SE) directionsStr = "SE";
					if (d == WheelDirectInput::S) directionsStr = "S ";
					if (d == WheelDirectInput::SW) directionsStr = "SW";
					if (d == WheelDirectInput::W) directionsStr = " W";
					if (d == WheelDirectInput::NW) directionsStr = "NW";
					saveButton(d, confTag, guid);
					return true;
				}
			}
		}
		WAIT(0);
	}
}

bool configHPattern() {
	int buttonsActive = 0;

	std::array<int, 8> directions = {
		WheelDirectInput::POV::N,
		WheelDirectInput::POV::NE,
		WheelDirectInput::POV::E,
		WheelDirectInput::POV::SE,
		WheelDirectInput::POV::S,
		WheelDirectInput::POV::SW,
		WheelDirectInput::POV::W,
		WheelDirectInput::POV::NW,
	};

	std::string confTag = "SHIFTER";
	std::string additionalInfo = "Press " + escapeKey + " to exit. Press " + skipKey + " to skip gear.";

	controls.UpdateValues(ScriptControls::InputDevices::Wheel, false, true);

	for (auto guid : controls.WheelControl.GetGuids()) {
		for (int i = 0; i < 255; i++) {
			if (controls.WheelControl.IsButtonPressed(i, guid)) {
				buttonsActive++;
			}
		}
	}

	if (buttonsActive > 0) {
		showSubtitle("One or more buttons had been pressed on start. Stop pressing and try again.");
		return false;
	}
	////////////////////////////////////////////////////////////////////////////////////////////////

	GUID devGUID = {};
	std::array<int, numGears> buttonArray; // There are gears 1-7 + R
	std::fill(buttonArray.begin(), buttonArray.end(), -1);
	std::string devName;

	int progress = 0;

	while (true) {
		if (IsKeyJustUp(str2key(escapeKey))) {
			return false;
		}
		if (IsKeyJustUp(str2key(skipKey))) {
			progress++;
		}

		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false, true);

		for (auto guid : controls.WheelControl.GetGuids()) {
			for (int i = 0; i < 255; i++) {
				if (controls.WheelControl.IsButtonPressed(i, guid) &&
					// only find unregistered buttons
					std::find(std::begin(buttonArray), std::end(buttonArray), i) == std::end(buttonArray)) {
					// also save device info when just started
					if (progress == 0) {
						devGUID = guid;
						std::wstring wDevName = controls.WheelControl.FindEntryFromGUID(guid)->diDeviceInstance.tszInstanceName;
						devName = std::string(wDevName.begin(), wDevName.end()).c_str();
						buttonArray[progress] = i;
						progress++;
					}
					// only accept same device onwards
					else if (guid == devGUID) {
						buttonArray[progress] = i;
						progress++;
					}
				}
			}
		}


		if (progress > 7) {
			break;
		}
		std::string gearDisplay;
		switch (progress) {
			case 0: gearDisplay = "Reverse"; break;
			case 1: gearDisplay = "1st gear"; break;
			case 2: gearDisplay = "2nd gear"; break;
			case 3: gearDisplay = "3rd gear"; break;
			case 4: case 5: case 6:
			case 7: gearDisplay = std::to_string(progress) + "th gear"; break;
		}
		showSubtitle("Shift into " + gearDisplay + ". " + additionalInfo);
		WAIT(0);
	}
	//save H-pattern doesn't need its own func because it's called once! :-)
	saveHShifter(confTag, devGUID, buttonArray, devName);
	return true;
}

bool isMenuControl(int control) {
	if (control == menuControls.ControlKeys[MenuControls::ControlType::MenuKey] ||
		control == menuControls.ControlKeys[MenuControls::ControlType::MenuSelect] ||
		control == menuControls.ControlKeys[MenuControls::ControlType::MenuCancel]) {
		return true;
	}

	return false;
}

bool configKeyboardKey(std::string confTag) {
	std::string additionalInfo = "Press " + escapeKey + " to exit.";
	while (true) {
		if (IsKeyJustUp(str2key(escapeKey))) {
			return false;
		}
		auto keymap = createKeyMap();
		for (auto k : keymap) {
			if (isMenuControl(k.second))
				continue;
			if (IsKeyJustUp(k.second)) {
				saveKeyboardKey(confTag, k.first);
				return true;
			}
		}

		// Key limits from keyboard.cpp
		// a-z
		for (int i = 0x30; i <= 0x39; i++) {
			if (isMenuControl(i))
				continue;
			if (IsKeyJustUp(i)) {
				std::string str = "";
				str = static_cast<char>(i);
				saveKeyboardKey(confTag, str);
				return true;
			}
		}
		// A-Z
		for (int i = 0x41; i <= 0x5A; i++) {
			if (isMenuControl(i))
				continue;
			if (IsKeyJustUp(i)) {
				std::string str = "";
				str = static_cast<char>(i);
				saveKeyboardKey(confTag, str);
				return true;
			}
		}

		showSubtitle("Press " + confTag + ". Menu controls can't be chosen." + additionalInfo);
		WAIT(0);
	}
}

bool configControllerButton(std::string confTag) {
	std::string additionalInfo = "Press " + escapeKey + " to exit.";
	XboxController* rawController = controls.GetRawController();
	if (rawController == nullptr)
		return false;

	while (true) {
		if (IsKeyJustUp(str2key(escapeKey))) {
			return false;
		}
		controls.UpdateValues(ScriptControls::InputDevices::Controller, false, true);

		for (std::string buttonHelper : rawController->XboxButtonsHelper) {
			auto button = rawController->StringToButton(buttonHelper);
			if (rawController->IsButtonJustPressed(button, controls.GetButtonState())) {
				saveControllerButton(confTag, buttonHelper);
				return true;
			}
		}
		showSubtitle("Press " + confTag + ". " + additionalInfo);
		WAIT(0);
	}
}


///////////////////////////////////////////////////////////////////////////////
//                              Script entry
///////////////////////////////////////////////////////////////////////////////

void main() {
	settingsGeneralFile = Util::GetModuleFolder(Util::GetOurModuleHandle()) + mtDir + "\\settings_general.ini";
	settingsWheelFile = Util::GetModuleFolder(Util::GetOurModuleHandle()) + mtDir + "\\settings_wheel.ini";
	settingsMenuFile = Util::GetModuleFolder(Util::GetOurModuleHandle()) + mtDir + "\\settings_menu.ini";
	
	settings.SetFiles(settingsGeneralFile, settingsWheelFile);
	settings.SetMenuFile(settingsMenuFile);

	logger.Write("Loading " + settingsGeneralFile);
	logger.Write("Loading " + settingsWheelFile);
	logger.Write("Loading " + settingsMenuFile);

	reInit();
	while (true) {
		update();
		update_menu();
		WAIT(0);
	}
}

void ScriptMain() {
	srand(GetTickCount());
	main();
}
