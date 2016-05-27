#pragma once

#include "..\..\ScriptHookV_SDK\inc\natives.h"
#include "..\..\ScriptHookV_SDK\inc\types.h"

#include <Windows.h>
#include <array>
#include <vector>
#include "VehicleExtensions.hpp"
#include <chrono>

#define SAMPLES 10


class VehicleData
{
public:
	VehicleData();
	void Clear();

	void ReadMemData(VehicleExtensions ext, Vehicle vehicle);

	bool IsBike;
	bool NoClutch;
	bool IsTruck;
	uint64_t Address;
	uint32_t Gears;
	float Rpm;

	// 1 = Not pressed, 0 = Fully pressed
	float Clutch;
	float Throttle;
	float Turbo;

	// Absolute speed, in m/s
	float Speed;

	// Directional speed, in m/s
	float Velocity;


	uint8_t TopGear;
	uint8_t LockGear;
	uint32_t CurrGear;
	uint32_t NextGear;
	uint32_t LockGears = 0x00010001;
	uint8_t PrevGear;
	float LockSpeed;
	bool LockTruck;
	bool SimulatedNeutral;

	std::vector<float> LockSpeeds = std::vector<float>(20);
	Vector3 getAccelerationVectors(Vector3 velocities);

	// Should be called after getAccelerationVectors has been called in a loop
	Vector3 getAccelerationVectorsAverage();

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

	std::array <char *, 7> noClutchModelNames = {
		"SURGE",
		"VOLTIC",
		"KHAMEL",
		"CADDY",
		"AIRTUG",
		"RHINO",
		"BULLDOZE"
	};
	bool noClutch(char *name);

	Vector3 prevVelocities;
	long long prevTime;
	Vector3 samples[SAMPLES];
	int averageIndex = 0;
	void zeroSamples();

};

