#pragma once

#include "..\..\ScriptHookV_SDK\inc\natives.h"
#include "..\..\ScriptHookV_SDK\inc\types.h"

#include <Windows.h>
#include <array>
#include <vector>
#include "VehicleExtensions.hpp"

class VehicleData
{
public:
	VehicleData();
	~VehicleData();

	void ReadMemData(VehicleExtensions ext, Vehicle vehicle);

	bool IsBike;
	bool IsTruck;
	uint64_t Address;
	uint32_t Gears;
	float Rpm;
	float Clutch;
	float Throttle;
	float Turbo;
	float Speed;
	float Velocity;
	uint8_t TopGear;
	uint32_t CurrGear;
	uint32_t NextGear;
	uint32_t LockGears = 0x00010001;
	uint8_t PrevGear;
	float LockSpeed;
	bool LockTruck;

	std::vector<float> LockSpeeds = std::vector<float>(20);

private:
	std::array <char *, 20> badModelNames = {
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
		"UTILTRUC"
	};
	bool isBadTruck(char *name);
};

