#include "script.h"
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include "VehicleExtensions.hpp"

enum ControlType {
	HR = 0,
	H1,
	H2,
	H3,
	H4,
	H5,
	H6,
	H7,
	H8,
	ShiftUp,
	ShiftDown,
	Clutch,
	Toggle,
	KShiftUp,
	KShiftDown,
	KClutch,
	CThrottle,
	CBrake,
	KThrottle,
	KBrake,
	SIZE_OF_ARRAY
};

Vehicle vehicle;
VehExt::VehicleExtensions ext;
Player player;
Ped playerPed;

bool enableManual = true;
bool autoGear1 = false;
bool autoReverse = false;
bool oldReverse = false;
bool debug = false;
int hshifter;
int controls[SIZE_OF_ARRAY];
bool controlCurr[SIZE_OF_ARRAY];
bool controlPrev[SIZE_OF_ARRAY];


bool isBike;
//uint64_t address;
uint32_t gears;
float rpm;
float clutch;
float throttle;
float turbo;
float speed;
float velocity;
uint32_t currGear;
uint32_t nextGear;
uint32_t lockGears = 0x00010001;

float ltvalf = 0.0f;
float rtvalf = 0.0f;

void readSettings() {
	enableManual = (GetPrivateProfileInt(L"MAIN", L"DefaultEnable", 1, L"./Gears.ini") == 1);
	autoGear1 = (GetPrivateProfileInt(L"MAIN", L"AutoGear1", 0, L"./Gears.ini") == 1);
	autoReverse = (GetPrivateProfileInt(L"MAIN", L"AutoReverse", 0, L"./Gears.ini") == 1);
	oldReverse = (GetPrivateProfileInt(L"MAIN", L"OldReverse", 0, L"./Gears.ini") == 1);
	controls[Toggle] = GetPrivateProfileInt(L"MAIN", L"Toggle", VK_OEM_5, L"./Gears.ini");

	controls[ShiftUp]	= GetPrivateProfileInt(L"CONTROLS", L"ShiftUp",		ControlFrontendAccept,	L"./Gears.ini");
	controls[ShiftDown] = GetPrivateProfileInt(L"CONTROLS", L"ShiftDown",	ControlFrontendX,		L"./Gears.ini");
	controls[Clutch]	= GetPrivateProfileInt(L"CONTROLS", L"Clutch",		ControlFrontendLb,		L"./Gears.ini");
	
	controls[KShiftUp] =	GetPrivateProfileInt(L"CONTROLS", L"KShiftUp", 		VK_NUMPAD9, L"./Gears.ini");
	controls[KShiftDown] =	GetPrivateProfileInt(L"CONTROLS", L"KShiftDown",	VK_NUMPAD7, L"./Gears.ini");
	controls[KClutch] =		GetPrivateProfileInt(L"CONTROLS", L"KClutch", 		VK_NUMPAD8, L"./Gears.ini");

	hshifter = GetPrivateProfileInt(L"CONTROLS", L"EnableH", 0, L"./Gears.ini");
	controls[HR] = GetPrivateProfileInt(L"CONTROLS", L"HR", VK_NUMPAD0, L"./Gears.ini");
	controls[H1] = GetPrivateProfileInt(L"CONTROLS", L"H1", VK_NUMPAD0, L"./Gears.ini");
	controls[H2] = GetPrivateProfileInt(L"CONTROLS", L"H2", VK_NUMPAD0, L"./Gears.ini");
	controls[H3] = GetPrivateProfileInt(L"CONTROLS", L"H3", VK_NUMPAD0, L"./Gears.ini");
	controls[H4] = GetPrivateProfileInt(L"CONTROLS", L"H4", VK_NUMPAD0, L"./Gears.ini");
	controls[H5] = GetPrivateProfileInt(L"CONTROLS", L"H5", VK_NUMPAD0, L"./Gears.ini");
	controls[H6] = GetPrivateProfileInt(L"CONTROLS", L"H6", VK_NUMPAD0, L"./Gears.ini");
	controls[H7] = GetPrivateProfileInt(L"CONTROLS", L"H7", VK_NUMPAD0, L"./Gears.ini");
	controls[H8] = GetPrivateProfileInt(L"CONTROLS", L"H8", VK_NUMPAD0, L"./Gears.ini");

	controls[CThrottle] = GetPrivateProfileInt(L"CONTROLS", L"CThrottle",	ControlFrontendRt, L"./Gears.ini");
	controls[CBrake]	= GetPrivateProfileInt(L"CONTROLS", L"CBrake",		ControlFrontendLt, L"./Gears.ini");
	controls[KThrottle] = GetPrivateProfileInt(L"CONTROLS", L"KThrottle",	0x57, L"./Gears.ini");
	controls[KBrake]	= GetPrivateProfileInt(L"CONTROLS", L"KBrake",		0x53, L"./Gears.ini");

	debug = (GetPrivateProfileInt(L"DEBUG", L"Info", 0, L"./Gears.ini") == 1);
}

void writeSettings() {
	WritePrivateProfileString(L"MAIN", L"DefaultEnable", (enableManual ? L" 1" : L" 0"), L"./Gears.ini");
}

bool isKeyPressed(int key) {
	if (GetAsyncKeyState(key) & 0x8000)
		return true;
	return false;
}

bool isKeyJustPressed(int key, ControlType control) {
	if (GetAsyncKeyState(key) & 0x8000)
		controlCurr[control] = true;
	else
		controlCurr[control] = false;

	// raising edge
	if (controlCurr[control] == true && controlPrev[control] == false) {
		controlPrev[control] = controlCurr[control];
		return true;
	}

	controlPrev[control] = controlCurr[control];
	return false;
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
	UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
	UI::_ADD_TEXT_COMPONENT_STRING(message);
	UI::_DRAW_NOTIFICATION(false, false);
}

void update() {
	if (isKeyJustPressed(controls[Toggle], Toggle)) {
		enableManual = !enableManual;
		std::stringstream message;
		message << "Manual Transmission " <<
			(enableManual ? "Enabled" : "Disabled");
		showNotification((char *)message.str().c_str());
		if (ENTITY::DOES_ENTITY_EXIST(vehicle)) {
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
		}
		writeSettings();
		readSettings();
	}

	if (CONTROLS::IS_CONTROL_JUST_RELEASED(0, ControlEnter)) {
		readSettings();
		lockGears = 0x00010001;
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

	if (!ENTITY::DOES_ENTITY_EXIST(vehicle))
		return;

	if (!(VEHICLE::IS_THIS_MODEL_A_CAR(ENTITY::GET_ENTITY_MODEL(vehicle))
		|| VEHICLE::IS_THIS_MODEL_A_BIKE(ENTITY::GET_ENTITY_MODEL(vehicle))
		|| VEHICLE::IS_THIS_MODEL_A_QUADBIKE(ENTITY::GET_ENTITY_MODEL(vehicle))
		) || VEHICLE::IS_THIS_MODEL_A_BICYCLE(ENTITY::GET_ENTITY_MODEL(vehicle)))
		return;


	if (VEHICLE::IS_THIS_MODEL_A_BIKE(ENTITY::GET_ENTITY_MODEL(vehicle))) {
		isBike = true;
	}
	else {
		isBike = false;
	}
	
	//address = ext.GetAddress(vehicle);
	gears = ext.GetGears(vehicle);
	currGear = (0xFFFF0000 & gears) >> 16;
	nextGear = 0x0000FFFF & gears;

	if (enableManual || debug) {
		rpm = ext.GetCurrentRPM(vehicle);
		clutch = ext.GetClutch(vehicle);
		throttle = ext.GetThrottle(vehicle);
		turbo = ext.GetTurbo(vehicle);
		speed = ENTITY::GET_ENTITY_SPEED(vehicle);
		velocity = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y;
	}

	if (CONTROLS::_GET_LAST_INPUT_METHOD(2)) {
		rtvalf = (isKeyPressed(controls[KThrottle]) ? 1.0f : 0.0f);
		ltvalf = (isKeyPressed(controls[KBrake]) ? 1.0f : 0.0f);
	}
	else {
		rtvalf = (CONTROLS::GET_CONTROL_VALUE(0, controls[CThrottle]) - 127) / 127.0f;
		ltvalf = (CONTROLS::GET_CONTROL_VALUE(0, controls[CBrake]) - 127) / 127.0f;
	}

	// Other scripts. 0 = nothing, 1 = Shift up, 2 = Shift down
	if (currGear == nextGear) {
		DECORATOR::DECOR_SET_INT(vehicle, "hunt_score", 0);
	}
	else if (currGear < nextGear) {
		DECORATOR::DECOR_SET_INT(vehicle, "hunt_score", 1);
	}
	else if (currGear > nextGear) {
		DECORATOR::DECOR_SET_INT(vehicle, "hunt_score", 2);
	}

	if (debug) {
		std::stringstream infos;

		infos << "RPM: " << std::setprecision(3) << rpm <<
			"\nCurrGear: " << currGear <<
			"\nNextGear: " << nextGear <<
			"\nClutch: " << std::setprecision(3) << clutch <<
			"\nThrottle: " << std::setprecision(3) << throttle <<
			"\nTurbo: " << std::setprecision(3) << turbo <<
			"\nV:" << std::setprecision(3) << velocity <<
			//"\nAddress: " << std::hex << address <<
			"\nE: " << (enableManual ? "Y" : "N");
		const char *infoc = infos.str().c_str();
		showText(0.01f, 0.5f, 0.4f, (char *)infoc);
	}

	if (!enableManual || !VEHICLE::IS_VEHICLE_DRIVEABLE(vehicle, false))
		return;

	// Reverse behavior
	if (isBike || (oldReverse && autoReverse)) {
		// For bikes, do this automatically.
		if (currGear == 0 && CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate)
			&& !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake) && speed < 2.0f) {
			lockGears = 0x00010001;
		}
		else if (currGear > 0 && CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake)
			&& !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) && speed < 2.0f) {
			lockGears = 0x00000000;
		}
	}
	else {
		if (oldReverse) {
			//Park/Reverse. Gear 0. Prevent going forward in gear 0.
			if (currGear == 0
				&& throttle > 0) {
				VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
				VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			}
			// Forward gears. Prevent reversing.
			else if (currGear > 0
				&& throttle < 0) {
				VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
				VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			}
			else {
				VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
			}
		}
		// Throttle pedal does old reverse stuff thing
		else {
			// Case forward gear
			// Desired: Only brake
			if (currGear > 0) {
				// LT behavior when still
				if (ltvalf > 0.0f && rtvalf < ltvalf &&
					velocity >= -0.1f && velocity <= 0.5f) {
					VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
					CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
					ext.SetThrottle(vehicle, 0.0f);
					ext.SetThrottleP(vehicle, 0.1f);
					VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
				}
				else {
					VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
				}
				// LT behavior when reversing
				if (ltvalf > 0.0f && rtvalf < ltvalf &&
					velocity < -0.1f) {
					VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
					CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
					CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, ltvalf);
					ext.SetThrottle(vehicle, 0.0f);
					ext.SetThrottleP(vehicle, 0.1f);
				}
			}
			// Case reverse gear
			// Desired: RT reverses, LT brakes
			if (currGear == 0) {
				// RT behavior
				if (rtvalf > 0.0f && rtvalf > ltvalf) {
					CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
					CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, rtvalf);
				}
				// LT behavior when still
				if (ltvalf > 0.0f && rtvalf < ltvalf &&
					velocity > -0.55f && velocity <= 0.5f) {
					VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
					CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
					ext.SetThrottle(vehicle, 0.0f);
					ext.SetThrottleP(vehicle, 0.1f);
					VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
				}
				else {
					VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
				}
				// LT behavior when reversing
				if (ltvalf > 0.0f &&
					velocity <= -0.5f) {
					CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
					CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, ltvalf);
				}
			}
		}
	}

	// Automatically engage first gear when stationary
	if (autoGear1 && throttle < 0.1f && speed < 0.1f && currGear > 1) {
		lockGears = 0x00010001;
	}

	int accelval = CONTROLS::GET_CONTROL_VALUE(0, ControlVehicleAccelerate);
	float accelvalf = (accelval - 127) / 127.0f;

	// Game wants to shift up. Triggered at high RPM, high speed.
	// Also user is pressing gas hard and game disagrees
	// Desired result: high RPM, same gear (currGear)
	// Current: Game fully depresses clutch and throttle
	// Want:	Clutch and throttle full override
	if ((currGear < nextGear && speed > 2.0f) || (clutch < 0.5 && accelvalf > 0.1f)) {
		ext.SetCurrentRPM(vehicle, accelvalf * 0.9f);
		ext.SetClutch(vehicle, 1.0f);	// Why won't this work
		ext.SetThrottle(vehicle, 1.0f);	// Why won't this work
		ext.SetCurrentRPM(vehicle, accelvalf * 1.1f);
	}

	// Game wants to shift down. Usually triggered when user accelerates
	// while in a too high gear.
	// Desired result: low RPM or stall, even. Stay in current gear!
	// Otherwise: force car to drive anyway to keep RPM up.
	// Current: Game fully depresses clutch and throttle
	// Update:	Game somewhat cooperates at low accelval?
	// Want:	Clutch override
	if (currGear > nextGear) {
		ext.SetClutch(vehicle, 1.0f);
		ext.SetThrottle(vehicle, accelvalf);
	}


	// sequential or h?
	if (!hshifter) {
		// Shift up
		if (CONTROLS::IS_CONTROL_JUST_PRESSED(0, controls[ShiftUp])
			|| isKeyJustPressed(controls[KShiftUp], KShiftUp)) {
			if (currGear < 8) {
				// Blowoff valve sound when game does this. Unknown why this can't
				// be emulated this way. Probably writing these values isn't working.
				ext.SetThrottle(vehicle, 0.0f);
				ext.SetClutch(vehicle, 0.1f);
				lockGears = currGear + 1 | ((currGear + 1) << 16);
			}
		}


		// Shift down
		if (CONTROLS::IS_CONTROL_JUST_PRESSED(0, controls[ShiftDown])
			|| isKeyJustPressed(controls[KShiftDown], KShiftDown)) {
			if (currGear > 0) {
				ext.SetThrottle(vehicle, 0.0f);
				ext.SetClutch(vehicle, 0.1f);
				lockGears = currGear - 1 | ((currGear - 1) << 16);
			}
		}
	}
	else
	{
		for (int i = 0; i <= 8; i++) {
			if (isKeyJustPressed(controls[i], (ControlType)i)) {
				ext.SetThrottle(vehicle, 0.0f);
				ext.SetClutch(vehicle, 0.1f);
				lockGears = i | (i << 16);
			}
		}
	}

	// Press clutch (doesn't actually do anything yet)
	if (CONTROLS::IS_CONTROL_PRESSED(0, controls[Clutch])
		|| isKeyPressed(controls[KClutch])) {
		ext.SetClutch(vehicle, 0.0f);
		//ext.SetCurrentRPM(vehicle, accelvalf * 1.1f);
		ext.SetThrottle(vehicle, accelvalf);
	}

	ext.SetGears(vehicle, lockGears);
}

void main() {
	readSettings();
	while (true) {
		update();
		WAIT(0);
	}
}

void ScriptMain() {
	srand(GetTickCount());
	main();
}
