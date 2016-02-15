#include "script.h"
// itching for a proper refactoring

void clearLog() {
	std::ofstream logFile;
	logFile.open("Gears.log", std::ofstream::out | std::ofstream::trunc);
	logFile.close();
}

void writeToLog(const std::string &text)
{
	std::ofstream logFile(
		"Gears.log", std::ios_base::out | std::ios_base::app);
	SYSTEMTIME currTimeLog;
	GetLocalTime(&currTimeLog);
	logFile << "[" <<
		std::setw(2) << std::setfill('0') << currTimeLog.wHour << ":" <<
		std::setw(2) << std::setfill('0') << currTimeLog.wMinute << ":" <<
		std::setw(2) << std::setfill('0') << currTimeLog.wSecond << "." <<
		std::setw(3) << std::setfill('0') << currTimeLog.wMilliseconds << "] " <<
		text << std::endl;
}

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
	Engine,
	KEngine,
	CToggle,
	SIZE_OF_ARRAY
};

std::array <char *, 22> badModelNames =
{
	"BENSON",
	"BIFF",
	"HAULER",
	"PACKER",
	"PHANTOM",
	"POUNDER",
	"FIRETRUK",
	"DUMP",
	"FLATBED",
	"MIXER",
	"MIXER2",
	"RUBBLE",
	"TIPTRUCK",
	"TIPTRUCK2",
	"BARRACKS",
	"BARRACKS2",
	"BARRACKS3",
	"RIPLEY",
	"SCRAP",
	"UTILLITRUCK",
	"UTILLITRUCK2",
	"UTILLITRUCK3"
};

Vehicle vehicle;
Vehicle prevVehicle;
Hash model;
VehicleExtensions ext;
Player player;
Ped playerPed;
uintptr_t clutchInstrLowAddr = 0;
uintptr_t clutchInstrLowTemp = 0;

uintptr_t clutchS01Addr = 0;
uintptr_t clutchS01Temp = 0;

uintptr_t clutchS04Addr = 0;
uintptr_t clutchS04Temp = 0;

uintptr_t throttleCutAddr = 0;
uintptr_t throttleCutTemp = 0;

bool runOnceRan = false;
int patched = 0;
int prevNotification = 0;

bool enableManual = true;
bool autoGear1 = false;
bool autoReverse = false;
bool oldReverse = false;
bool engDamage = false;
bool engStall = false;
bool engBrake = true;
bool debug = false;
int hshifter;
int controls[SIZE_OF_ARRAY];
int cToggleTime;
bool controlCurr[SIZE_OF_ARRAY];
bool controlPrev[SIZE_OF_ARRAY];

bool isBike;
bool isTruck;
uint64_t address;
uint32_t gears;
float rpm;
float clutch;
float throttle;
float turbo;
float speed;
float velocity;
uint8_t topGear;
uint32_t currGear;
uint32_t nextGear;
uint32_t lockGears = 0x00010001;
uint8_t prevGear;
float lockSpeed;
bool lockTruck;
std::vector<float> lockSpeeds(20);

float ltvalf = 0.0f;
float rtvalf = 0.0f;
float clutchvalf = 0.0f;

int accelval;
float accelvalf = 0.0f;

struct timeval currTime;
long long pressTime;
long long releaseTime;

void readSettings() {
	enableManual = (GetPrivateProfileInt(L"MAIN", L"DefaultEnable",  1, L"./Gears.ini") == 1);
	autoGear1    = (GetPrivateProfileInt(L"MAIN", L"AutoGear1",      0, L"./Gears.ini") == 1);
	autoReverse  = (GetPrivateProfileInt(L"MAIN", L"AutoReverse",    0, L"./Gears.ini") == 1);
	oldReverse   = (GetPrivateProfileInt(L"MAIN", L"OldReverse",     0, L"./Gears.ini") == 1);
	engDamage    = (GetPrivateProfileInt(L"MAIN", L"EngineDamage",   0, L"./Gears.ini") == 1);
	engStall     = (GetPrivateProfileInt(L"MAIN", L"EngineStalling", 0, L"./Gears.ini") == 1);
	engBrake     = (GetPrivateProfileInt(L"MAIN", L"EngineBraking",  1, L"./Gears.ini") == 1);

	controls[Toggle]  = GetPrivateProfileInt(L"MAIN", L"Toggle",  VK_OEM_5, L"./Gears.ini");
	controls[CToggle] = GetPrivateProfileInt(L"MAIN", L"CToggle", ControlScriptPadDown, L"./Gears.ini");
	cToggleTime       = GetPrivateProfileInt(L"MAIN", L"CToggleTime", 500, L"./Gears.ini");

	controls[ShiftUp]   = GetPrivateProfileInt(L"CONTROLS", L"ShiftUp",   ControlFrontendAccept, L"./Gears.ini");
	controls[ShiftDown] = GetPrivateProfileInt(L"CONTROLS", L"ShiftDown", ControlFrontendX,      L"./Gears.ini");
	controls[Clutch]    = GetPrivateProfileInt(L"CONTROLS", L"Clutch",    ControlFrontendLb,     L"./Gears.ini");
	controls[Engine]    = GetPrivateProfileInt(L"CONTROLS", L"Engine",    ControlFrontendLs,     L"./Gears.ini");

	controls[KShiftUp]   = GetPrivateProfileInt(L"CONTROLS", L"KShiftUp",	VK_NUMPAD9, L"./Gears.ini");
	controls[KShiftDown] = GetPrivateProfileInt(L"CONTROLS", L"KShiftDown",	VK_NUMPAD7, L"./Gears.ini");
	controls[KClutch]    = GetPrivateProfileInt(L"CONTROLS", L"KClutch", 	VK_NUMPAD8, L"./Gears.ini");
	controls[KEngine]    = GetPrivateProfileInt(L"CONTROLS", L"KEngine",    0x45,       L"./Gears.ini");

	hshifter     = GetPrivateProfileInt(L"CONTROLS", L"EnableH", 0, L"./Gears.ini");
	controls[HR] = GetPrivateProfileInt(L"CONTROLS", L"HR", VK_NUMPAD0, L"./Gears.ini");
	controls[H1] = GetPrivateProfileInt(L"CONTROLS", L"H1", VK_NUMPAD0, L"./Gears.ini");
	controls[H2] = GetPrivateProfileInt(L"CONTROLS", L"H2", VK_NUMPAD0, L"./Gears.ini");
	controls[H3] = GetPrivateProfileInt(L"CONTROLS", L"H3", VK_NUMPAD0, L"./Gears.ini");
	controls[H4] = GetPrivateProfileInt(L"CONTROLS", L"H4", VK_NUMPAD0, L"./Gears.ini");
	controls[H5] = GetPrivateProfileInt(L"CONTROLS", L"H5", VK_NUMPAD0, L"./Gears.ini");
	controls[H6] = GetPrivateProfileInt(L"CONTROLS", L"H6", VK_NUMPAD0, L"./Gears.ini");
	controls[H7] = GetPrivateProfileInt(L"CONTROLS", L"H7", VK_NUMPAD0, L"./Gears.ini");
	controls[H8] = GetPrivateProfileInt(L"CONTROLS", L"H8", VK_NUMPAD0, L"./Gears.ini");

	controls[CThrottle] = GetPrivateProfileInt(L"CONTROLS", L"CThrottle", ControlFrontendRt, L"./Gears.ini");
	controls[CBrake]    = GetPrivateProfileInt(L"CONTROLS", L"CBrake",    ControlFrontendLt, L"./Gears.ini");
	
	controls[KThrottle] = GetPrivateProfileInt(L"CONTROLS", L"KThrottle", 0x57, L"./Gears.ini");
	controls[KBrake]    = GetPrivateProfileInt(L"CONTROLS", L"KBrake",	  0x53, L"./Gears.ini");

	debug = (GetPrivateProfileInt(L"DEBUG", L"Info", 0, L"./Gears.ini") == 1);
	writeToLog("Settings loaded");
}

void writeSettings() {
	WritePrivateProfileString(L"MAIN", L"DefaultEnable", (enableManual ? L" 1" : L" 0"), L"./Gears.ini");
}

long long milliseconds_now() {
	static LARGE_INTEGER s_frequency;
	static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
	if (s_use_qpc) {
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return (1000LL * now.QuadPart) / s_frequency.QuadPart;
	}
	else {
		return GetTickCount();
	}
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

bool wasControlPressedForMs(int control, int ms) {
	if (CONTROLS::IS_CONTROL_JUST_PRESSED(0, control)) {
		pressTime = milliseconds_now();
	}
	if (CONTROLS::IS_CONTROL_JUST_RELEASED(0, control)) {
		releaseTime = milliseconds_now();
	}

	if ((releaseTime - pressTime) >= ms) {
		pressTime = 0;
		releaseTime = 0;
		return true;
	}
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
	if (prevNotification)
		UI::_REMOVE_NOTIFICATION(prevNotification);
	UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
	UI::_ADD_TEXT_COMPONENT_STRING(message);
	prevNotification = UI::_DRAW_NOTIFICATION(false, false);
}

void showDebugInfo() {
	std::stringstream infos;

	infos << "RPM: " << std::setprecision(3) << rpm <<
		"\nCurrGear: " << currGear <<
		"\nNextGear: " << nextGear <<
		"\nClutch: " << std::setprecision(3) << clutch <<
		"\nThrottle: " << std::setprecision(3) << throttle <<
		//"\nTurbo: " << std::setprecision(3) << turbo <<
		//"\nV:" << std::setprecision(3) << velocity <<
		"\nAddress: " << std::hex << address <<
		"\nE: " << (enableManual ? "Y" : "N");
	const char *infoc = infos.str().c_str();
	showText(0.01f, 0.5f, 0.4f, (char *)infoc);
}

void patchClutchInstructions() {
	int success = 0;

	clutchInstrLowTemp = ext.PatchClutchLow();
	if (clutchInstrLowTemp) {
		clutchInstrLowAddr = clutchInstrLowTemp;
		writeToLog("Clutch patched");
		patched++;
	}

	clutchS01Temp = ext.PatchClutchStationary01();
	if (clutchS01Temp && !clutchS01Addr) {
		clutchS01Addr = clutchS01Temp;
		writeToLog("0.1 Patched");
		patched++;
	}

	clutchS04Temp = ext.PatchClutchStationary04();
	if (clutchS04Temp) {
		clutchS04Addr = clutchS04Temp;
		writeToLog("0.4 Patched");
		patched++;
	}

	throttleCutTemp = ext.PatchThrottleRedline();
	if (throttleCutTemp) {
		throttleCutAddr = throttleCutTemp;
		writeToLog("Throttle Patched");
		patched++;
	}

	if (patched != 4) {
		writeToLog("Patching failed");
	}

	runOnceRan = true;
}

bool restoreClutchInstructions() {
	int success = 0;
	if (clutchInstrLowAddr) {
		ext.RestoreClutchLow(clutchInstrLowAddr);
		writeToLog("Clutch restored");
		clutchInstrLowAddr = 0;
		success++;
		patched--;
	}
	else {
		writeToLog("Clutch not restored");
	}

	if (clutchS01Addr) {
		ext.RestoreClutchStationary01(clutchS01Addr);
		writeToLog("0.1 Restored");
		clutchS01Addr = 0;
		success++;
		patched--;
	}
	else {
		writeToLog("0.1 not restored");
	}

	if (clutchS04Addr) {
		ext.RestoreClutchStationary04(clutchS04Addr);
		writeToLog("0.4 Restored");
		clutchS04Addr = 0;
		success++;
		patched--;
	}
	else {
		writeToLog("0.4 not restored");
	}

	if (throttleCutAddr) {
		ext.RestoreThrottleRedline(throttleCutAddr);
		writeToLog("Throttle Restored");
		throttleCutAddr = 0;
		success++;
		patched--;
	}
	else {
		writeToLog("Throttle not restored");
	}

	if (success == 4 || patched == 0)
		return true;

	return false;
}

bool isBadTruck(char *name) {
	for (int i = 0; i < badModelNames.size(); i++) {
		if (strcmp(name, badModelNames[i]) == 0)
			return true;
	}
	return false;
}

void reInit() {
	readSettings();
	lockGears = 0x00010001;
}


void toggleManual() {
	enableManual = !enableManual;
	std::stringstream message;
	message << "Manual Transmission " <<
		(enableManual ? "Enabled" : "Disabled");
	showNotification((char *)message.str().c_str());
	writeToLog(message.str());
	if (ENTITY::DOES_ENTITY_EXIST(vehicle)) {
		VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, false, true);
	}
	if (enableManual) {
		patchClutchInstructions();
	}
	else {
		restoreClutchInstructions();
	}
	writeSettings();
	readSettings();
}

uintptr_t addrThrottle;

void update() {
	//prevTime = currTime;
	//currTime = milliseconds_now();
	//elapsed = currTime - prevTime;

	if (isKeyJustPressed(controls[Toggle], Toggle)) {
		toggleManual();
	}
	
	// Patch clutch on game start
	if (enableManual && !runOnceRan) {
		writeToLog("Patching clutch on start");
		patchClutchInstructions();
		runOnceRan = true;
	}

	if (CONTROLS::IS_CONTROL_JUST_RELEASED(0, ControlEnter)) {
		reInit();
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

	if (vehicle != prevVehicle)
	{
		isBike = VEHICLE::IS_THIS_MODEL_A_BIKE(model) == TRUE;
		isTruck = isBadTruck(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(model));
		reInit();
	}
	prevVehicle = vehicle;

	if (wasControlPressedForMs(controls[CToggle], cToggleTime)) {
		toggleManual();
	}
	
	address = ext.GetAddress(vehicle);
	gears = ext.GetGears(vehicle);
	currGear = (0xFFFF0000 & gears) >> 16;
	nextGear = 0x0000FFFF & gears;

	if (enableManual || debug) {
		rpm      = ext.GetCurrentRPM(vehicle);
		clutch   = ext.GetClutch(vehicle);
		throttle = ext.GetThrottle(vehicle);
		turbo    = ext.GetTurbo(vehicle);
		topGear  = ext.GetTopGear(vehicle);
		speed    = ENTITY::GET_ENTITY_SPEED(vehicle);
		velocity = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y;
	}

	if (CONTROLS::_GET_LAST_INPUT_METHOD(2)) {
		rtvalf     = (isKeyPressed(controls[KThrottle]) ? 1.0f : 0.0f);
		ltvalf     = (isKeyPressed(controls[KBrake]) ? 1.0f : 0.0f);
		clutchvalf = (isKeyPressed(controls[KClutch]) ? 1.0f : 0.0f);
	}
	else {
		rtvalf     = (CONTROLS::GET_CONTROL_VALUE(0, controls[CThrottle]) - 127) / 127.0f;
		ltvalf     = (CONTROLS::GET_CONTROL_VALUE(0, controls[CBrake]) - 127) / 127.0f;
		clutchvalf = (CONTROLS::GET_CONTROL_VALUE(0, controls[Clutch]) - 127) / 127.0f;
	}

	accelval = CONTROLS::GET_CONTROL_VALUE(0, ControlVehicleAccelerate);
	accelvalf = (accelval - 127) / 127.0f;

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
		showDebugInfo();
	}

	if (!enableManual ||
		(!VEHICLE::IS_VEHICLE_DRIVEABLE(vehicle, false) &&
		VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) < -100.0f) )
		return;

	// Automatically engage first gear when stationary
	if (autoGear1 && throttle < 0.1f && speed < 0.1f && currGear > 1) {
		lockGears = 0x00010001;
	}

	// Reverse behavior
	// For bikes, do this automatically.
	// Also if the user chooses to.
	if (isBike || (oldReverse && autoReverse)) {
		if (currGear == 0 && CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate)
			&& !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake) && speed < 2.0f) {
			lockGears = 0x00010001;
		}
		else if (currGear > 0 && CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake)
			&& !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) && speed < 2.0f) {
			lockGears = 0x00000000;
		}
	}
	// Reversing behavior: blocking
	else {
		// Block and reverse with brake in reverse gear
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
		// New reverse: Reverse with throttle
		else {
			// Case forward gear
			// Desired: Only brake
			if (currGear > 0) {
				// LT behavior when still
				//if (ltvalf > 0.0f && rtvalf < ltvalf &&
				//	velocity >= -0.1f && velocity <= 0.5f) {
				if (ltvalf > 0.0f && rtvalf < ltvalf &&
					velocity <= 0.5f && velocity >= -0.1f) {
					CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
					ext.SetThrottleP(vehicle, 0.0f);
					VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
					VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, true);
				}
				else {
					VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
				}
				// LT behavior when rolling back
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
				ext.SetThrottleP(vehicle, -0.1f);
				// RT behavior
				if (rtvalf > 0.0f && rtvalf > ltvalf) {
					CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
					CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, rtvalf);
				}
				// LT behavior when still
				if (ltvalf > 0.0f && rtvalf <= ltvalf &&
					velocity > -0.55f && velocity <= 0.5f) {
					VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
					CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
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

	// Since trucks don't stop accelerating and don't disengage the clutch at currGear < nextGear
	// Save the speed this happened at until next shift up and don't let truck go faster by doing
	// the clutch thing by ourselves instead.
	if (currGear < nextGear) {
		if (isTruck) {
			lockSpeed = velocity;
		}
		else if (prevGear <= currGear || velocity <= lockSpeed || lockSpeed < 0.01f) {
			lockSpeed = velocity;
		}
		lockTruck = isTruck;
	}

	// Disengage the clutch when cap is reached to cap speed
	if (isTruck) {
		if ((velocity > lockSpeed && lockTruck) ||
			(velocity > lockSpeed && prevGear > currGear)) {
			clutchvalf = 1.0f;
			VEHICLE::_SET_VEHICLE_ENGINE_TORQUE_MULTIPLIER(vehicle, 0.0f);
			VEHICLE::_SET_VEHICLE_ENGINE_POWER_MULTIPLIER(vehicle, 0.0f);
			ext.SetThrottle(vehicle, 0.0f);
		}
	}
	// Since we know prevGear and the speed for that, let's simulate "engine" braking.
	else if (engBrake && currGear > 0)
	{
		// Only when free rolling at high speeds
		if (velocity > lockSpeeds.at(currGear) &&
			rtvalf < 0.99 && rpm > 0.9) {
			CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, (1.0f - clutchvalf)*0.8f*rpm);
			if (ltvalf < 0.1f) {
				VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, false);
			}
			else {
				VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
			}
		}
		if (ltvalf < 0.1f) {
			VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, false);
		}
	}

	// Game wants to shift up. Triggered at high RPM, high speed.
	// Desired result: high RPM, same gear, no more accelerating
	// Result:	Is as desired. Speed may drop a bit because of game clutch.
	if (currGear > 0 &&
		((currGear < nextGear && speed > 2.0f) || (clutch < 0.5 && rtvalf > 0.6f))) {
		ext.SetThrottle(vehicle, 1.2f);
		ext.SetCurrentRPM(vehicle, 1.07f);
	}

	// Game wants to shift down. Usually triggered when user accelerates
	// while in a too high gear.
	// Desired result: low RPM or stall. Same gear. Low torque.
	// Result:	Patching the clutch ops results in above
	if (currGear > nextGear) {
		VEHICLE::_SET_VEHICLE_ENGINE_TORQUE_MULTIPLIER(vehicle, rpm);
		// RPM mismatch on transition currGear > nextGear to currGear == nextGear?
	}

	// Engine damage
	if (engDamage &&
		rpm > 0.98f && accelvalf > 0.99f) {
		VEHICLE::SET_VEHICLE_ENGINE_HEALTH(vehicle, VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) - (0.2f*accelvalf));
		if (VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) < 0.0f) {
			VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
		}
	}

	// Stalling
	if (engStall && currGear > 2 &&
		rpm < 0.25f && speed < 5.0f && clutchvalf < 0.33f) {
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
	}

	if (!VEHICLE::_IS_VEHICLE_ENGINE_ON(vehicle) &&
		(CONTROLS::IS_CONTROL_JUST_PRESSED(0, controls[Engine]) || isKeyJustPressed(controls[KEngine], KEngine))) {
		VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, false, true);
	}

	// sequential or h?
	if (!hshifter) {
		// Shift up
		if (CONTROLS::IS_CONTROL_JUST_PRESSED(0, controls[ShiftUp]) ||
			isKeyJustPressed(controls[KShiftUp], KShiftUp)) {
			if (currGear < topGear) {
				if (clutchvalf < 0.1f) {
					clutchvalf = 1.0f;
				}
				ext.SetThrottle(vehicle, 0.0f);
				lockGears = currGear + 1 | ((currGear + 1) << 16);
				lockTruck = false;
				prevGear = currGear;
				lockSpeeds[currGear] = velocity;
			}
		}

		// Shift down
		if (CONTROLS::IS_CONTROL_JUST_PRESSED(0, controls[ShiftDown]) ||
			isKeyJustPressed(controls[KShiftDown], KShiftDown)) {
			if (currGear > 0) {
				if (clutchvalf < 0.1f) {
					clutchvalf = 1.0f;
				}
				ext.SetThrottle(vehicle, 0.0f);
				lockGears = currGear - 1 | ((currGear - 1) << 16);
				lockTruck = false;
				prevGear = currGear;
				lockSpeeds[currGear] = velocity;
			}
		}
	}
	else
	{
		// All keys checking
		for (uint8_t i = 0; i <= topGear; i++) {
			if (i > H8)
				i = H8;
			if (isKeyJustPressed(controls[i], (ControlType)i)) {
				if (clutchvalf < 0.1f) {
					clutchvalf = 1.0f;
				}
				ext.SetThrottle(vehicle, 0.0f);
				lockGears = i | (i << 16);
				lockTruck = false;
				prevGear = currGear;
				lockSpeeds[currGear] = velocity;
			}
		}
	}

	// Clutch override working now
	ext.SetClutch(vehicle, 1.0f - clutchvalf);
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
