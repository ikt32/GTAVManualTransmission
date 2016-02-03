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
	SIZE_OF_ARRAY
};

Vehicle vehicle;
VehExt::VehicleExtensions ext;

bool enableManual = true;
int hshifter;
int debug;
int controls[SIZE_OF_ARRAY];
bool controlCurr[SIZE_OF_ARRAY];
bool controlPrev[SIZE_OF_ARRAY];


bool isBike;
uint64_t address;
uint32_t gears;
float rpm;
float clutch;
float throttle;
float turbo;
float speed;
uint32_t currGear;
uint32_t nextGear;
uint32_t lockGears = 0x00010001;

uint32_t showTicks = 0;

void readSettings() {
	debug = GetPrivateProfileInt(L"DEBUG", L"Info", 0, L"./Gears.ini");

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

	controls[Toggle]	= GetPrivateProfileInt(L"CONTROLS", L"Toggle",		VK_OEM_5,	L"./Gears.ini");
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
		if (vehicle) {
			VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
		}
	}

	if (CONTROLS::IS_CONTROL_JUST_RELEASED(0, ControlEnter)) {
		readSettings();
		lockGears = 0x00010001;
	}

	Player player = PLAYER::PLAYER_ID();
	Ped playerPed = PLAYER::PLAYER_PED_ID();

	// check if player ped exists and control is on (e.g. not in a cutscene)
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed) || !PLAYER::IS_PLAYER_CONTROL_ON(player))
		return;

	// check for player ped death and player arrest
	if (ENTITY::IS_ENTITY_DEAD(playerPed) || PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE))
		return;

	vehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);

	if (!vehicle)
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

	address = ext.GetAddress(vehicle);
	gears = ext.GetGears(vehicle);
	rpm = ext.GetCurrentRPM(vehicle);
	clutch = ext.GetClutch(vehicle);
	throttle = ext.GetThrottle(vehicle);
	turbo = ext.GetTurbo(vehicle);
	currGear = (0xFFFF0000 & gears) >> 16;
	nextGear = 0x0000FFFF & gears;
	speed = ENTITY::GET_ENTITY_SPEED(vehicle);

	if (debug) {
		std::stringstream infos;

		infos << "RPM: " << std::setprecision(3) << rpm <<
			"\nCurrGear: " << currGear <<
			"\nNextGear: " << nextGear <<
			"\nClutch: " << std::setprecision(3) << clutch <<
			"\nThrottle: " << std::setprecision(3) << throttle <<
			"\nTurbo: " << std::setprecision(3) << turbo <<
			//"\nAddress: " << std::hex << address <<
			"\nE: " << (enableManual ? "Y" : "N");
		const char *infoc = infos.str().c_str();
		showText(0.01f, 0.5f, 0.4f, (char *)infoc);
	}

	// Other scripts. 0 = nothing, 1 = Shift up, 2 = Shift down
	//50 ticks before the arrow should disappear
	if (currGear == nextGear) {
		showTicks++;
		if (showTicks > 50) {
			DECORATOR::DECOR_SET_INT(vehicle, "Carwash_Vehicle_Decorator", 0);
			showTicks = 0;
		}
	}
	else if (currGear < nextGear) {
		DECORATOR::DECOR_SET_INT(vehicle, "Carwash_Vehicle_Decorator", 1);
	}
	else if (currGear > nextGear) {
		DECORATOR::DECOR_SET_INT(vehicle, "Carwash_Vehicle_Decorator", 2);
	}

	if (!enableManual)
		return;

	// Reverse behavior
	if (!isBike) {
		// Park/Reverse. Gear 0. Prevent going forward in gear 0.
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
	else {
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

	int accelval = CONTROLS::GET_CONTROL_VALUE(0, ControlVehicleAccelerate);
	float accelvalf = (accelval - 127) / 127.0f;

	// Game wants to shift up. Triggered at high RPM, high speed.
	// Desired result: high RPM, same gear (currGear)
	// Also user is pressing gas hard and game disagrees
	if (currGear < nextGear && speed > 2.0f || (clutch < 1.0 && accelval > 200)) {
		ext.SetThrottle(vehicle, 1.0f);
		ext.SetClutch(vehicle, 1.0f);
		ext.SetCurrentRPM(vehicle, accelvalf * 1.0f);
	}

	// Game wants to shift down. Usually triggered when user accelerates
	// while in a too high gear.
	// Desired result: low RPM or stall, even. Stay in current gear!
	// Otherwise: force car to drive anyway to keep RPM up.
	// Current: Game cuts engine power
	if (currGear > nextGear) {
		ext.SetClutch(vehicle, accelvalf);
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

	ext.SetGears(vehicle, lockGears);

	// Press clutch (doesn't actually do shit)
	if (CONTROLS::IS_CONTROL_PRESSED(0, controls[Clutch])
		|| isKeyPressed(controls[KClutch])) {
		ext.SetClutch(vehicle, 0.0f);
		//ext.SetCurrentRPM(vehicle, accelvalf * 1.1f);
		ext.SetThrottle(vehicle, accelvalf);
	}
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
