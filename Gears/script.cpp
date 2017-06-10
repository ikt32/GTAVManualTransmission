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
#include "Util/Paths.h"

#include "menu.h"
#include "menucontrols.h"

GameSound gearRattle("DAMAGED_TRUCK_IDLE", 0);

int soundID;
float stallingProgress = 0.0f;

std::string settingsGeneralFile;
std::string settingsWheelFile;
std::string settingsMenuFile;

NativeMenu::Menu menu;
NativeMenu::MenuControls menuControls;

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
extern std::vector<std::string> speedoTypes;

bool lookrightfirst = false;


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
		return;
	}

	vehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);

	if (!ENTITY::DOES_ENTITY_EXIST(vehicle) ||
		playerPed != VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1)) {
		return;
	}

	///////////////////////////////////////////////////////////////////////////
	//                           Update stuff
	///////////////////////////////////////////////////////////////////////////

	if (prevVehicle != vehicle) {
		vehData.Clear();
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
		prevVehicle = vehicle;
		initSteeringPatches();
	}

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
		showWheelInfo();
	}

	if (!settings.IsCorrectVersion()) {
		Color red;
		red.R = 255;
		red.G = 0;
		red.B = 0;
		red.A = 255;

		showText(0.05, 0.05, 1.0, settings.GetVersionError().c_str(), 0, red);
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
				controls.ButtonHeld(ScriptControls::WheelControlType::Toggle) ||
				controls.PrevInput == ScriptControls::Controller	&& controls.ButtonJustPressed(ScriptControls::LegacyControlType::Toggle) ) {
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
				playWheelEffectsPlane(settings, vehData);
			}
			return;
		}
		return;
	}
	
	///////////////////////////////////////////////////////////////////////////
	//                          Ground vehicle controls
	///////////////////////////////////////////////////////////////////////////

	if (controls.ButtonJustPressed(ScriptControls::KeyboardControlType::Toggle) ||
		controls.ButtonJustPressed(ScriptControls::WheelControlType::Toggle) ||
		controls.ButtonHeld(ScriptControls::ControllerControlType::Toggle) || 
		controls.PrevInput == ScriptControls::Controller	&& controls.ButtonHeld(ScriptControls::LegacyControlType::Toggle)) {
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
		controls.ButtonJustPressed(ScriptControls::WheelControlType::ToggleH) ||
		controls.ButtonHeld(ScriptControls::ControllerControlType::ToggleH) ||
		controls.PrevInput == ScriptControls::Controller	&& controls.ButtonHeld(ScriptControls::LegacyControlType::ToggleH)) {
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
	std::string gear = std::to_string(vehData.CurrGear);
	if (vehData.SimulatedNeutral) {
		gear = "N";
	}
	else if (vehData.CurrGear == 0) {
		gear = "R";
	}
	Color c;
	if (vehData.CurrGear == vehData.TopGear) {
		c.R = settings.GearTopColorR;
		c.G = settings.GearTopColorG;
		c.B = settings.GearTopColorB;
		c.A = 255;
	}
	else {
		c = solidWhite;
	}
	showText(settings.GearXpos, settings.GearYpos, settings.GearSize, gear.c_str(), settings.HUDFont, c, true);

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
	showText(settings.ShiftModeXpos, settings.ShiftModeYpos, settings.ShiftModeSize, shiftModeText, settings.HUDFont, solidWhite, true);
	
	// Speedometer using dashboard speed
	// PROBABLY not going over 999kph (smallest unit)
	if (settings.Speedo == "kph" ||
		settings.Speedo == "mph" ||
		settings.Speedo == "ms") {
		std::stringstream speedoFormat;

		float dashms = ext.GetDashSpeed(vehicle);
		if (!vehData.HasSpeedo && dashms > 0.1f) {
			vehData.HasSpeedo = true;
		}
		if (!vehData.HasSpeedo) {
			dashms = abs(vehData.Velocity);
		}

		if (settings.Speedo == "kph" ) {
			speedoFormat << std::setfill('0') << std::setw(3) << std::to_string(static_cast<int>(std::round(dashms * 3.6f)));
			if (settings.SpeedoShowUnit) speedoFormat << (settings.HUDFont == 2 ? " kph" : " km/h");
		}
		if (settings.Speedo == "mph" ) {
			speedoFormat << std::setfill('0') << std::setw(3) << std::to_string(static_cast<int>(std::round(dashms / 0.44704f)));
			if (settings.SpeedoShowUnit) speedoFormat << " mph";
		}
		if (settings.Speedo == "ms") {
			speedoFormat << std::setfill('0') << std::setw(3) << std::to_string(static_cast<int>(std::round(dashms)));
			if (settings.SpeedoShowUnit) speedoFormat << (settings.HUDFont == 2 ? " mps" : " m/s");;
		}
		showText(settings.SpeedoXpos, settings.SpeedoYpos, settings.SpeedoSize, speedoFormat.str().c_str(), settings.HUDFont, solidWhite, true);
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
	std::stringstream ssEnabled;
	std::stringstream ssRPM;
	std::stringstream ssCurrGear;
	std::stringstream ssNextGear;
	std::stringstream ssClutch;
	std::stringstream ssThrottle;
	std::stringstream ssTurbo;
	std::stringstream ssAddress;
	std::stringstream ssDashSpd;
	std::stringstream ssDbias;

	ssEnabled << "Mod:\t\t" << (settings.EnableManual ? "Enabled" : "Disabled");
	ssRPM		<< "RPM:\t\t" << std::setprecision(3) << vehData.Rpm;
	ssCurrGear	<< "CurrGear:\t" << vehData.CurrGear;
	ssNextGear	<< "NextGear:\t" << vehData.NextGear;
	ssClutch	<< "Clutch:\t\t" << std::setprecision(3) << vehData.Clutch;
	ssThrottle	<< "Throttle:\t" << std::setprecision(3) << vehData.Throttle;
	ssTurbo		<< "Turbo:\t\t" << std::setprecision(3) << vehData.Turbo;
	ssAddress	<< "Address:\t" << std::hex << reinterpret_cast<uint64_t>(vehData.Address);
	ssDashSpd	<< "Speedo:\t" << (vehData.HasSpeedo ? "Yes" : "No");
	ssDbias		<< "DBias:\t\t" << std::setprecision(3) << vehData.DriveBiasFront;

	showText(0.01f, 0.275f, 0.4f, ssEnabled.str().c_str(),	4, solidWhite, true);
	showText(0.01f, 0.300f, 0.4f, ssRPM.str().c_str(),		4, solidWhite, true);
	showText(0.01f, 0.325f, 0.4f, ssCurrGear.str().c_str(),	4, solidWhite, true);
	showText(0.01f, 0.350f, 0.4f, ssNextGear.str().c_str(),	4, solidWhite, true);
	showText(0.01f, 0.375f, 0.4f, ssClutch.str().c_str(),	4, solidWhite, true);
	showText(0.01f, 0.400f, 0.4f, ssThrottle.str().c_str(),	4, solidWhite, true);
	showText(0.01f, 0.425f, 0.4f, ssTurbo.str().c_str(),	4, solidWhite, true);
	showText(0.01f, 0.450f, 0.4f, ssAddress.str().c_str(),	4, solidWhite, true);
	showText(0.01f, 0.475f, 0.4f, ssDashSpd.str().c_str(),	4, solidWhite, true);
	showText(0.01f, 0.500f, 0.4f, ssDbias.str().c_str(),	4, solidWhite, true);

	std::stringstream ssThrottleInput;
	std::stringstream ssBrakeInput;
	std::stringstream ssClutchInput;
	std::stringstream ssHandbrakInput;

	ssThrottleInput << "Throttle:\t" << controls.ThrottleVal;
	ssBrakeInput	<< "Brake:\t\t" << controls.BrakeVal;
	ssClutchInput	<< "Clutch:\t\t" << controls.ClutchVal;
	ssHandbrakInput << "Handb:\t\t" << controls.HandbrakeVal;

	showText(0.85, 0.050, 0.4, ssThrottleInput.str().c_str(),	4, solidWhite, true);
	showText(0.85, 0.075, 0.4, ssBrakeInput.str().c_str(),		4, solidWhite, true);
	showText(0.85, 0.100, 0.4, ssClutchInput.str().c_str(),		4, solidWhite, true);
	showText(0.85, 0.125, 0.4, ssHandbrakInput.str().c_str(),	4, solidWhite, true);

	if (settings.EnableWheel) {
		std::stringstream dinputDisplay;
		dinputDisplay << "Wheel" << (controls.WheelControl.IsConnected(controls.SteerGUID) ? "" : " not") << " present";
		showText(0.85, 0.150, 0.4, dinputDisplay.str().c_str(), 4, solidWhite, true);
	}
}

void showDebugInfoWheel(ScriptSettings &settings, float effSteer, int damperForce, float steerSpeed, double GForce, float oversteer, float understeer) {
	std::stringstream SteerRaw;
	std::stringstream SteerNorm;
	std::stringstream steerDisplay;
	std::stringstream GForceDisplay;
	std::stringstream damperF;
	std::stringstream ssUnderSteer;
	std::stringstream ssOverSteer;
	
	SteerRaw		<< "SteerRaw:\t" << controls.SteerVal;
	SteerNorm		<< "SteerNorm:\t" << effSteer;
	steerDisplay	<< "SteerSpeed:\t" << steerSpeed;
	GForceDisplay	<< "GForceFinal:\t" <<
		std::setprecision(5) << (-GForce * 5000 * settings.PhysicsStrength * settings.FFGlobalMult);
	damperF			<< "DampForce:\t" << steerSpeed * damperForce * 0.1;
	ssUnderSteer	<< "Understeer:\t" << understeer;
	ssOverSteer		<< "Oversteer:\t" << oversteer;

	showText(0.85, 0.175, 0.4, SteerRaw.str().c_str(),		4, solidWhite, true);
	showText(0.85, 0.200, 0.4, SteerNorm.str().c_str(),		4, solidWhite, true);
	showText(0.85, 0.225, 0.4, steerDisplay.str().c_str(),	4, solidWhite, true);
	showText(0.85, 0.250, 0.4, GForceDisplay.str().c_str(),	4, solidWhite, true);
	showText(0.85, 0.275, 0.4, damperF.str().c_str(),		4, solidWhite, true);
	showText(0.85, 0.300, 0.4, ssUnderSteer.str().c_str(),	4, solidWhite, true);
	showText(0.85, 0.325, 0.4, ssOverSteer.str().c_str(),	4, solidWhite, true);
}

void showDebugInfo3D(Vector3 location, std::vector<std::string> textLines, Color backgroundColor = transparentGray) {
	float x, y;
	float height = 0.0125f;
	
	if (GRAPHICS::GET_SCREEN_COORD_FROM_WORLD_COORD(location.x, location.y, location.z, &x, &y)) {
		int i = 0;
		for (auto line : textLines) {
			showText(x, y + height * i, 0.2f, line.c_str());
			i++;
		}
		
		float szX = 0.060f;
		float szY = (height * i) + 0.02f;
		GRAPHICS::DRAW_RECT(x + 0.027f, y + (height * i)/2.0f, szX, szY,
							backgroundColor.R, backgroundColor.G, backgroundColor.B, backgroundColor.A);
	}
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
	settings.Read(&menuControls, &menu);
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
	vehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);
	resetSteeringMultiplier();
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

void resetSteeringMultiplier() {
	if (vehicle == 0) return;
	if (ext.GetSteeringMultiplier(vehicle) != 1.0f) {
		ext.SetSteeringMultiplier(vehicle, 1.0f);
	}
}

void toggleManual() {
	settings.EnableManual = !settings.EnableManual;
	settings.SaveGeneral();
	std::stringstream message;
	message << "Manual Transmission " <<
	           (settings.EnableManual ? "Enabled" : "Disabled");
	showNotification(message.str().c_str(), &prevNotification);
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

void initSteeringPatches() {
	if (controls.PrevInput != ScriptControls::Wheel && settings.LogiLEDs) {
		for (GUID guid : controls.WheelControl.GetGuids()) {
			controls.WheelControl.PlayLedsDInput(guid, 0.0, 0.5, 1.0);
		}
	}
	if (vehicle == 0) return;
	if (controls.PrevInput == ScriptControls::Wheel) {
		if (!MemoryPatcher::SteeringPatched && settings.PatchSteering) {
			MemoryPatcher::PatchSteeringCorrection();
		}
		ext.SetSteeringMultiplier(vehicle, settings.GameSteerMult);
	}
	else {
		if (MemoryPatcher::SteeringPatched && settings.PatchSteering && !settings.PatchSteeringAlways) {
			MemoryPatcher::RestoreSteeringCorrection();
		}
		resetSteeringMultiplier();
	}
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
				}
				else {
					showNotification("Switched to controller", &prevNotification);
				}
				break;
			case ScriptControls::Wheel:
				showNotification("Switched to wheel", &prevNotification);
				break;
		}
		initSteeringPatches();
	}
	if (controls.PrevInput == ScriptControls::Wheel) {
		CONTROLS::STOP_PAD_SHAKE(0);
	}
}

void updateSteeringMultiplier() {
	if (vehicle == 0) return;
	ext.SetSteeringMultiplier(vehicle, settings.GameSteerMult);
}

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Shifting
///////////////////////////////////////////////////////////////////////////////

void setShiftMode(int shiftMode) {
	gearRattle.Stop();
	if (shiftMode > 2 || shiftMode < 0)
		return;

	if (settings.ShiftMode == HPattern  && controls.PrevInput == ScriptControls::Controller) {
		settings.ShiftMode = Automatic;
	}

	if ((settings.ShiftMode == Automatic || settings.ShiftMode == Sequential) && vehData.CurrGear > 1) {
		vehData.SimulatedNeutral = false;
	}

	std::string mode = "Mode: ";
	// ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
	switch (settings.ShiftMode) {
		case Sequential: mode += "Sequential"; break;
		case HPattern: mode += "H-Pattern"; break;
		case Automatic: mode += "Automatic"; break;
	}
	showNotification(mode.c_str(), &prevNotification);
}

void cycleShiftMode() {
	settings.ShiftMode++;
	if (settings.ShiftMode > 2) {
		settings.ShiftMode = 0;
	}

	setShiftMode(settings.ShiftMode);
	settings.SaveGeneral();
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
		controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H7)) ||
		controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::HR))
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
	// TODO: Properly always block buttons?
	auto tapStateUp = controls.ButtonTapped(ScriptControls::ControllerControlType::ShiftUp);
	auto tapStateDown = controls.ButtonTapped(ScriptControls::ControllerControlType::ShiftDown);

	if (tapStateUp == XboxController::TapState::ButtonDown ||
		tapStateDown == XboxController::TapState::ButtonDown) {
		DisableActionControlsTick();
	}

	// Shift up
	if (controls.PrevInput == ScriptControls::Controller	&& tapStateUp == XboxController::TapState::Tapped ||
		controls.PrevInput == ScriptControls::Controller	&& controls.ButtonJustPressed(ScriptControls::LegacyControlType::ShiftUp) ||
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

	if (controls.PrevInput == ScriptControls::Controller	&& tapStateDown == XboxController::TapState::Tapped ||
		controls.PrevInput == ScriptControls::Controller	&& controls.ButtonJustPressed(ScriptControls::LegacyControlType::ShiftDown) || 
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
	auto tapStateUp = controls.ButtonTapped(ScriptControls::ControllerControlType::ShiftUp);
	auto tapStateDown = controls.ButtonTapped(ScriptControls::ControllerControlType::ShiftDown);

	if (tapStateUp == XboxController::TapState::ButtonDown ||
		tapStateDown == XboxController::TapState::ButtonDown) {
		DisableActionControlsTick();
	}

	// Shift up
	if (controls.PrevInput == ScriptControls::Controller	&& tapStateUp == XboxController::TapState::Tapped ||
		controls.PrevInput == ScriptControls::Controller	&& controls.ButtonJustPressed(ScriptControls::LegacyControlType::ShiftUp) ||
		controls.PrevInput == ScriptControls::Keyboard		&& controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ShiftUp) ||
		controls.PrevInput == ScriptControls::Wheel			&& controls.ButtonJustPressed(ScriptControls::WheelControlType::ShiftUp)) {
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

	if (controls.PrevInput == ScriptControls::Controller	&& tapStateDown == XboxController::TapState::Tapped ||
		controls.PrevInput == ScriptControls::Controller	&& controls.ButtonJustPressed(ScriptControls::LegacyControlType::ShiftDown) ||
		controls.PrevInput == ScriptControls::Keyboard		&& controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ShiftDown) ||
		controls.PrevInput == ScriptControls::Wheel			&& controls.ButtonJustPressed(ScriptControls::WheelControlType::ShiftDown)) {
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
	float lowSpeed = 2.5f;
	float idleThrottle = 0.33f;

	if (controls.ClutchVal < 1.0f - settings.ClutchCatchpoint) {
		// Automatic cars APPARENTLY need little/no brake pressure to stop
		if (settings.ShiftMode == Automatic && controls.BrakeVal > 0.1f || 
			vehData.Rpm > 0.25f && vehData.Speed >= lowSpeed) {
			return;
		}

		// Forward
		if (vehData.CurrGear > 0 && vehData.Speed < vehData.CurrGear * lowSpeed &&
		    controls.ThrottleVal < 0.25f && controls.BrakeVal < 0.95) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, idleThrottle);
			ext.SetCurrentRPM(vehicle, 0.21f);
		}

		// Reverse
		if (vehData.CurrGear == 0 &&
			controls.ThrottleVal < 0.25f && controls.BrakeVal < 0.95) {
			if (vehData.Velocity < -lowSpeed) {
				controls.ClutchVal = 1.0f;
			}
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, 0.26f);
			ext.SetCurrentRPM(vehicle, 0.21f);
		}
	}
}

float getAverage(std::vector<float> values) {
	float total = 0.0f;
	for (auto value : values) {
		total += value;
	}
	return total / values.size();
}

std::vector<bool> getWheelLockups(Vehicle handle) {
	std::vector<bool> lockups;
	float velocity = ENTITY::GET_ENTITY_VELOCITY(vehicle).y;
	auto wheelsSpeed = ext.GetWheelRotationSpeeds(vehicle);
	for (auto wheelSpeed : wheelsSpeed) {
		if (abs(velocity) > 0.01f && wheelSpeed == 0.0f)
			lockups.push_back(true);
		else
			lockups.push_back(false);
	}
	return lockups;
}

void showWheelInfo() {
	auto numWheels = ext.GetNumWheels(vehicle);
	auto wheelsSpeed = ext.GetTyreSpeeds(vehicle);
	auto wheelsCompr = ext.GetWheelCompressions(vehicle);
	auto wheelsHealt = ext.GetWheelHealths(vehicle);
	auto wheelsContactCoords = ext.GetWheelLastContactCoords(vehicle);
	auto wheelsOnGround = ext.GetWheelsOnGround(vehicle);
	auto wheelCoords = ext.GetWheelCoords(vehicle, ENTITY::GET_ENTITY_COORDS(vehicle, true), ENTITY::GET_ENTITY_ROTATION(vehicle, 0), ENTITY::GET_ENTITY_FORWARD_VECTOR(vehicle));
	auto wheelLockups = getWheelLockups(vehicle);

	for (int i = 0; i < numWheels; i++) {
		float wheelSpeed = wheelsSpeed[i];
		float wheelCompr = wheelsCompr[i];
		float wheelHealt = wheelsHealt[i];
		Color c = wheelLockups[i] ? solidOrange : transparentGray;
		c = wheelsOnGround[i] ? c : solidRed;
		showDebugInfo3D(wheelCoords[i], {
			"Speed: " + std::to_string(wheelSpeed),
			"Compress: " + std::to_string(wheelCompr),
			"Health: " + std::to_string(wheelHealt), },
			c );
		GRAPHICS::DRAW_LINE(wheelCoords[i].x, wheelCoords[i].y, wheelCoords[i].z, 
							wheelCoords[i].x, wheelCoords[i].y, wheelCoords[i].z + 1.0f + 2.5f * wheelCompr, 255, 0, 0, 255);
	}
}

void functionEngStall() {
	float lowSpeed = 2.4f;
	float stallingRateDivisor = 3500000.0f; // lower = faster stalling
	float timeStep = SYSTEM::TIMESTEP() * 100.0f;
	
	// Since we don't have a convenient way to get the drivetrain [yet]
	// (I really do not want to look into handling...), we can just use the
	// average of all wheels for now.
	// TODO: Get drivetrain from handling data

	auto wheelSpeeds = ext.GetTyreSpeeds(vehicle);
	std::vector<float> wheelsToConsider;

	// TODO: this is ugly and dumb. Fix. 

	//bikes? huh
	if (ext.GetNumWheels(vehicle) == 2) {
		// fwd
		if (vehData.DriveBiasFront == 1.0f) {
			wheelsToConsider.push_back(wheelSpeeds[0]);
		}
		// rwd
		else if (vehData.DriveBiasFront == 0.0f) {
			wheelsToConsider.push_back(wheelSpeeds[1]);
		}
		// awd
		else {
			wheelsToConsider = wheelSpeeds;
		}
	}
	// normal vehicles
	else if (ext.GetNumWheels(vehicle) == 4) {
		// fwd
		if (vehData.DriveBiasFront == 1.0f) {
			wheelsToConsider.push_back(wheelSpeeds[0]);
			wheelsToConsider.push_back(wheelSpeeds[1]);
		}
		// rwd
		else if (vehData.DriveBiasFront == 0.0f) {
			wheelsToConsider.push_back(wheelSpeeds[2]);
			wheelsToConsider.push_back(wheelSpeeds[3]);
		}
		// awd
		else {
			wheelsToConsider = wheelSpeeds;
		}
	}
	// offroad? hmmmm
	else if (ext.GetNumWheels(vehicle) == 6) {
		// fwd
		if (vehData.DriveBiasFront == 1.0f) {
			wheelsToConsider.push_back(wheelSpeeds[0]);
			wheelsToConsider.push_back(wheelSpeeds[1]);
		}
		// rwd
		else if (vehData.DriveBiasFront == 0.0f) {
			wheelsToConsider.push_back(wheelSpeeds[2]);
			wheelsToConsider.push_back(wheelSpeeds[3]);
			wheelsToConsider.push_back(wheelSpeeds[4]);
			wheelsToConsider.push_back(wheelSpeeds[5]);
		}
		// awd
		else {
			wheelsToConsider = wheelSpeeds;
		}
	}
	// trikes 'n stuff, dunno :(
	else {
		wheelsToConsider = wheelSpeeds;
	}



	float avgWheelSpeed = abs(getAverage(wheelsToConsider));
	if (vehData.Clutch > 0.2f && // 1.0 = fully engaged, 0.1 = fully disengaged
		controls.ClutchVal < 1.0f - settings.StallingThreshold &&
		vehData.Rpm < 0.25f &&
		((avgWheelSpeed < vehData.CurrGear * lowSpeed) || (vehData.CurrGear == 0 && avgWheelSpeed < lowSpeed)) &&
		VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
		stallingProgress += (rand() % 1000) / (stallingRateDivisor * (controls.ThrottleVal+0.001f) * timeStep);
		if (stallingProgress > 1.0f) {
			VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
			gearRattle.Stop();
			stallingProgress = 0.0f;
			if (settings.DisplayInfo)
				showNotification("Your car has stalled.");
		}
	}
	else {
		if (stallingProgress > 0.0f) {
			stallingProgress -= (rand() % 1000) / (stallingRateDivisor * (controls.ThrottleVal + 0.001f) * timeStep);
		}
		else {
			stallingProgress = 0.0f;
		}
	}
	//showText(0.1, 0.1, 1.0, ("Stall progress:" + std::to_string(stallingProgress)).c_str(), 0, solidWhite, true);
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
			//showText(0.1, 0.1, 0.5, "we have clutch");
		}
	}
	vehData.UpdateRpm();
	ext.SetClutch(vehicle, finalClutch);
	//showText(0.1, 0.15, 0.5, ("clutch set: " + std::to_string(finalClutch)).c_str());
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
			
			if (wheelThrottleVal > 0.01f && controls.ClutchVal < settings.ClutchCatchpoint && !vehData.SimulatedNeutral) {
				//showText(0.3, 0.0, 1.0, "We should burnout");
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, wheelThrottleVal);
				CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, wheelThrottleVal);
			}

			if (wheelThrottleVal > 0.01f && (controls.ClutchVal > settings.ClutchCatchpoint || vehData.SimulatedNeutral)) {
				if (wheelBrakeVal > 0.01f) {
					//showText(0.3, 0.0, 1.0, "We should rev and brake");
					//showText(0.3, 0.05, 1.0, ("Brake pressure:" + std::to_string(wheelBrakeVal)).c_str() );
					CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, wheelBrakeVal);
					ext.SetThrottleP(vehicle, 0.1f);
					ext.SetBrakeP(vehicle, 1.0f);
					brakelights = true;
					fakeRev();
				}
				else if (controls.ClutchVal > 0.9 || vehData.SimulatedNeutral) {
					//showText(0.3, 0.0, 1.0, "We should rev and do nothing");
					ext.SetThrottleP(vehicle, wheelThrottleVal); 
					fakeRev();
				} else {
					//showText(0.3, 0.0, 1.0, "We should rev and apply throttle");
					CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, wheelThrottleVal);
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
		(controls.PrevInput == ScriptControls::Controller	&& controls.ButtonJustPressed(ScriptControls::ControllerControlType::Engine) ||
		 controls.PrevInput == ScriptControls::Controller	&& controls.ButtonJustPressed(ScriptControls::LegacyControlType::Engine) ||
		 controls.PrevInput == ScriptControls::Keyboard		&& controls.ButtonJustPressed(ScriptControls::KeyboardControlType::Engine) ||
		 controls.PrevInput == ScriptControls::Wheel		&& controls.ButtonJustPressed(ScriptControls::WheelControlType::Engine) ||
		settings.ThrottleStart && controls.ThrottleVal > 0.98f && controls.ClutchVal > settings.ClutchCatchpoint)) {
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, false, true);
	}
	if  (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) &&
		((controls.PrevInput == ScriptControls::Controller	&& controls.ButtonJustPressed(ScriptControls::ControllerControlType::Engine) && settings.ToggleEngine) ||
		 (controls.PrevInput == ScriptControls::Controller	&& controls.ButtonJustPressed(ScriptControls::LegacyControlType::Engine) && settings.ToggleEngine) ||
		 controls.PrevInput == ScriptControls::Keyboard		&& controls.ButtonJustPressed(ScriptControls::KeyboardControlType::Engine) ||
		 controls.PrevInput == ScriptControls::Wheel		&& controls.ButtonJustPressed(ScriptControls::WheelControlType::Engine))) {
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
	
	// who was first?
	if (controls.ButtonIn(ScriptControls::WheelControlType::LookRight) &&
		controls.ButtonJustPressed(ScriptControls::WheelControlType::LookLeft)) {
		lookrightfirst = true;
	}

	if (controls.ButtonIn(ScriptControls::WheelControlType::LookLeft) &&
		controls.ButtonIn(ScriptControls::WheelControlType::LookRight)) {
		CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(lookrightfirst ? -180.0f : 180.0f);
	}
	else if (controls.ButtonIn(ScriptControls::WheelControlType::LookLeft)) {
		CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(90);
	}
	else if (controls.ButtonIn(ScriptControls::WheelControlType::LookRight)) {
		CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(-90);
	}
	if (controls.ButtonReleased(ScriptControls::WheelControlType::LookLeft) && !(controls.ButtonIn(ScriptControls::WheelControlType::LookRight)) ||
		controls.ButtonReleased(ScriptControls::WheelControlType::LookRight) && !(controls.ButtonIn(ScriptControls::WheelControlType::LookLeft))) {
		CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(0);
	}
	if (controls.ButtonReleased(ScriptControls::WheelControlType::LookLeft)  ||
		controls.ButtonReleased(ScriptControls::WheelControlType::LookRight)) {
		lookrightfirst = false;
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

// I have never flown a plane so I have no idea how this should feel
// Also I don't think planes are flown with a steering wheel but I could be wrong
void playWheelEffectsPlane(ScriptSettings& settings, VehicleData& vehData) {
	auto steerAxis = controls.WheelControl.StringToAxis(controls.WheelAxes[static_cast<int>(controls.SteerAxisType)]);

	if (!controls.WheelControl.IsConnected(controls.SteerGUID) ||
		controls.PrevInput != ScriptControls::Wheel ||
		!settings.EnableFFB) {
		return;
	}

	vehData.getAccelerationVectors(vehData.V3Velocities);
	Vector3 accelValsAvg = vehData.getAccelerationVectorsAverage();

	float steerMult = settings.SteerAngleMax / settings.SteerAngleAlt;
	
	// Probably 0 when centered so just make a loop around dis
	float effSteer = steerMult * 2.0f * (controls.SteerVal - 0.5f);

	int damperForce = settings.DamperMin;

	// steerSpeed is to dampen the steering wheel
	auto steerSpeed = controls.WheelControl.GetAxisSpeed(steerAxis, controls.SteerGUID) / 20;

	// We're a fucking plane so there are no fake G-Forces
	// Just forces over the control surfaces so just re-center @ speed?
	float centerForce = effSteer * vehData.Velocity;

	if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
		damperForce *= 2;
	}

	int totalForce =
		static_cast<int>(centerForce * 100 * settings.PhysicsStrength * settings.FFGlobalMult) + // 2G = max force, Koenigsegg One:1 only does 1.7g!
		static_cast<int>(steerSpeed * damperForce * 0.1);

	// Soft lock
	if (effSteer > 1.0f) {
		totalForce = static_cast<int>((effSteer - 1.0f) * 100000) + totalForce;
		if (effSteer > 1.1f) {
			totalForce = 10000;
		}
	}
	else if (effSteer < -1.0f) {
		totalForce = static_cast<int>((-effSteer - 1.0f) * -100000) + totalForce;
		if (effSteer < -1.1f) {
			totalForce = -10000;
		}
	}
	controls.WheelControl.SetConstantForce(controls.SteerGUID, totalForce);

	if (settings.DisplayInfo) {
		showDebugInfoWheel(settings, effSteer, damperForce, steerSpeed, centerForce, 0.0f, 0.0f);
	}
}

void playWheelEffects(ScriptSettings& settings, VehicleData& vehData, bool airborne, bool ignoreSpeed) {
	if (!settings.EnableFFB ||
		controls.PrevInput != ScriptControls::Wheel ||
		!controls.WheelControl.IsConnected(controls.SteerGUID)) {
		return;
	}

	if (settings.LogiLEDs) {
		controls.WheelControl.PlayLedsDInput(controls.SteerGUID, vehData.Rpm, 0.45, 0.95);
	}

	vehData.getAccelerationVectors(vehData.V3Velocities);
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
	damperForce -= static_cast<int>(adjustRatio * accelValsAvg.y);
	
	if (damperForce < settings.DamperMin) {
		damperForce = settings.DamperMin;
	}

	// steerSpeed is to dampen the steering wheel
	auto steerAxis = controls.WheelControl.StringToAxis(controls.WheelAxes[static_cast<int>(controls.SteerAxisType)]);
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

	if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
		damperForce *= 2;
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
//                              Script entry
///////////////////////////////////////////////////////////////////////////////

void main() {
	logger.Write("Script started");

	if (!controls.WheelControl.PreInit()) {
		logger.Write("DirectInput failed to initialize");
	}

	settingsGeneralFile = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + mtDir + "\\settings_general.ini";
	settingsWheelFile = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + mtDir + "\\settings_wheel.ini";
	settingsMenuFile = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + mtDir + "\\settings_menu.ini";
	
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
