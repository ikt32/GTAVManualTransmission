#pragma once

#include "..\..\ScriptHookV_SDK\inc\types.h"

#include <Windows.h>
#include <array>
#include <vector>
#include "VehicleExtensions.hpp"
#include <chrono>

#define SAMPLES 4


class VehicleData
{
public:
	VehicleData();
	void Clear();

	void ReadMemData(VehicleExtensions ext, Vehicle vehicle);

	bool IsBike = false;
	bool NoClutch = false;
	bool IsTruck = false;
	uint64_t Address = 0;
	uint32_t Gears = 0x00010001;
	float Rpm = 0.0f;

	// 1 = Not pressed, 0 = Fully pressed
	float Clutch = 0.0f;
	float Throttle = 0.0f;
	float Turbo = 0.0f;

	// Absolute speed, in m/s
	float Speed = 0.0f;

	// Directional speed, in m/s
	float Velocity = 0.0f;


	uint8_t TopGear = 0;
	uint8_t LockGear = 0;
	uint32_t CurrGear = 0;
	uint32_t NextGear = 0;
	uint32_t LockGears = 0x00010001;
	uint8_t PrevGear = 0;
	float LockSpeed = 0;
	bool LockTruck = false;
	bool SimulatedNeutral = false;

	std::vector<float> LockSpeeds = std::vector<float>(20);
	Vector3 getAccelerationVectors(Vector3 velocities);

	// Should be called after getAccelerationVectors has been called in a loop
	Vector3 getAccelerationVectorsAverage() const;

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

	Vector3 prevVelocities = {};
	long long prevTime = 0;
	Vector3 samples[SAMPLES] = { {} };
	int averageIndex = 0;
	void zeroSamples();

};

