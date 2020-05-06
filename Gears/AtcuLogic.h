#include "script.h"
#include <inc\types.h>
#include <inc\natives.h>
#include "VehicleData.hpp"
#include "Input/CarControls.hpp"
#include "Util/MathExt.h"
#include "Memory/Offsets.hpp"
#include "ScriptSettings.hpp"

extern ScriptSettings g_settings;
extern CarControls g_controls;
extern Vehicle g_playerVehicle;
extern VehicleGearboxStates g_gearStates;
extern VehicleData g_vehData;
extern VehicleExtensions g_ext;

namespace AtcuLogic{
	void Cycle();
	float getGearMinSpeed(int gear);
	float getGearMaxSpeed(int gear);
	float gearPredictStandardPower(int gear);
	float gearPredictRpm(int atGear);
};
