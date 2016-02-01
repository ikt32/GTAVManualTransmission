#include "script.h"
#include <vector>
#include <string>
#include <sstream>
#include "VehicleExtensions.hpp"

enum ControlType {
	ShiftUp,
	ShiftDown,
	Clutch,
	Toggle
};

Vehicle vehicle;
VehExt::VehicleExtensions ext;
int debug;
int controls[3];
bool enableManual = true;
bool enableKeyPrev;
bool enableKeyCurr;

void readSettings() {
	debug = GetPrivateProfileInt(L"DEBUG", L"Info", 0, L"./Gears.ini");
	controls[ShiftUp] = GetPrivateProfileInt(L"CONTROLS", L"ShiftUp", ControlVehicleDuck, L"./Gears.ini");
	controls[ShiftDown] = GetPrivateProfileInt(L"CONTROLS", L"ShiftDown", ControlVehicleSelectNextWeapon, L"./Gears.ini");
	controls[Clutch] = GetPrivateProfileInt(L"CONTROLS", L"Clutch", ControlVehicleAim, L"./Gears.ini");
	controls[Toggle] = GetPrivateProfileInt(L"CONTROLS", L"Toggle", VK_OEM_5, L"./Gears.ini");
}

bool isEnableKeyJustPressed(DWORD Key) {
	if (GetAsyncKeyState(Key) & 0x8000)
		enableKeyCurr = true;
	else
		enableKeyCurr = false;

	// raising edge
	if (enableKeyCurr == true && enableKeyPrev == false) {
		enableKeyPrev = enableKeyCurr;
		return true;
	}

	enableKeyPrev = enableKeyCurr;
	return false;
}

void update() {
	if (isEnableKeyJustPressed(controls[Toggle]))
		enableManual = !enableManual;

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
		))
		return;

	if (CONTROLS::IS_CONTROL_JUST_RELEASED(0, ControlEnter))
		readSettings();

	uint64_t address = ext.GetAddress(vehicle);
	uint32_t gears = ext.GetGears(vehicle);
	float rpm = ext.GetCurrentRPM(vehicle);
	float clutch = ext.GetClutch(vehicle);
	float throttle = ext.GetThrottle(vehicle);
	float turbo = ext.GetTurbo(vehicle);
	uint16_t currGear = (0xFFFF0000 & gears) >> 16;
	uint16_t nextGear = 0x0000FFFF & gears;
	float speed = ENTITY::GET_ENTITY_SPEED(vehicle);

	if (debug) {
		std::stringstream infos;

		infos << "RPM: " << rpm <<
			"\nCurrGear: " << currGear <<
			"\nNextGear: " << nextGear <<
			"\nClutch: " << clutch <<
			"\nThrottle: " << throttle <<
			"\nTurbo: " << turbo <<
			"\nAddress: " << std::hex << address;
		const char *infoc = infos.str().c_str();

		UI::SET_TEXT_FONT(0);
		UI::SET_TEXT_SCALE(0.4f, 0.4f);
		UI::SET_TEXT_COLOUR(255, 255, 255, 255);
		UI::SET_TEXT_WRAP(0.0, 1.0);
		UI::SET_TEXT_CENTRE(0);
		UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
		UI::SET_TEXT_EDGE(1, 0, 0, 0, 205);
		UI::_SET_TEXT_ENTRY("STRING");
		UI::_ADD_TEXT_COMPONENT_STRING((char *)infoc);
		UI::_DRAW_TEXT(0.01f, 0.5f);
	}

	if (!enableManual)
		return;

	// Park/Reverse. Gear 0. Prevent going forward in gear 0.
	if (currGear == 0 && CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate)
		&& !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake) && speed < 2.0f) {
		VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
		VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
	}
	// Forward gears. Prevent reversing.
	else if (currGear > 0 && CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake)
		&& !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) && speed < 2.0f) {
		VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
		VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
	}
	else {
		VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
	}

	// Game wants to shift up. Triggered at high RPM, high speed.
	// Desired result: high RPM, same gear (currGear)
	if (currGear < nextGear) {
		ext.SetGears(vehicle, currGear | (currGear << 16));
		ext.SetCurrentRPM(vehicle, 0.9f);
		ext.SetClutch(vehicle, 1.2f);
		ext.SetThrottle(vehicle, 1.2f);
		ext.SetCurrentRPM(vehicle, 1.1f);
	}

	// Game wants to shift down. Usually triggered when user accelerates
	// while in a too high gear.
	// Desired result: low RPM or stall, even. Stay in current gear!
	// Otherwise: force car to drive anyway to keep RPM up.
	// Current: Game cuts engine power
	if (currGear > nextGear) {
		ext.SetGears(vehicle, currGear | (currGear << 16));
		ext.SetClutch(vehicle, 1.2f);
		ext.SetThrottle(vehicle, 1.2f);
	}

	// Shift up
	if (CONTROLS::IS_CONTROL_JUST_PRESSED(0, controls[ShiftUp])) {
		if (currGear < 8) {
			ext.SetGears(vehicle, currGear + 1 | ((currGear + 1) << 16));
			ext.SetClutch(vehicle, 0.1f);
			ext.SetThrottle(vehicle, 0.5f);
		}
	}

	// Shift down
	if (CONTROLS::IS_CONTROL_JUST_PRESSED(0, controls[ShiftDown])) {
		if (currGear > 0) {
			ext.SetGears(vehicle, currGear - 1 | ((currGear - 1) << 16));
		}
	}

	// Press clutch (doesn't actually do shit)
	if (CONTROLS::IS_CONTROL_PRESSED(0, controls[Clutch])) {
		ext.SetClutch(vehicle, 0.1f);
		ext.SetCurrentRPM(vehicle, ((CONTROLS::GET_CONTROL_VALUE(0, ControlVehicleAccelerate) - 127) / 127)*1.1f);
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
