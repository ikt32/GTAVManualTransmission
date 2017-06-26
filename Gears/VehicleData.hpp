/*
 * What a mess. 
 */

#pragma once

#include "../../ScriptHookV_SDK/inc/types.h"

#include <Windows.h>
#include <array>
#include <vector>
#include "Memory/VehicleExtensions.hpp"
#include <chrono>

#define SAMPLES 2


class VehicleData {
public:
	VehicleData();
	void Clear();
	void UpdateRpm(); // call end of script loop only once

	enum class VehicleClass {
		Car,
		Bike,
		Bicycle,
		Boat,
		Plane,
		Heli,
		Quad,
		Unknown
	};

	void UpdateValues(VehicleExtensions& ext, Vehicle vehicle);
	std::vector<float> GetWheelCompressionSpeeds();
	//bool IsBike = false;

	VehicleClass Class = VehicleClass::Car;
	bool NoClutch = false;
	bool IsTruck = false;
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

	// just hardcode 20 gears because we only have 8 (0 thru 7) and if R* is changing
	// so much stuff that this breaks, other shit breaks too so this can be changed too
	// no need to overcomplicate stuff........................ i guess.
	std::vector<float> LockSpeeds = std::vector<float>(20);
	float Pitch = 0;

	// In radians
	float SteeringAngle = 0.0f;
	Vector3 RotationVelocity = {};
	std::vector<float> WheelCompressions = {};
	Vector3 V3Velocities = {};
	float DriveBiasFront;
	Vector3 getAccelerationVectors(Vector3 velocities);

	// Should be called after getAccelerationVectors has been called in a loop
	Vector3 getAccelerationVectorsAverage() const;

	// Moved out of ScriptControls to make it standalone
	// Perceived accelerator value, float
	float ControlAccelerate = 0.0f;

	// TODO: make these private or something idk

	float PrevRpm = 0.0f;

	bool BlinkerLeft = false;
	bool BlinkerRight = false;
	bool BlinkerHazard = false;
	int BlinkerTicks = 0;

	bool TruckShiftUp = false;

	int RadioStationIndex = 0;
	bool HasSpeedo = false;

private:
	std::array<char *, 20> badModelNames = {
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
	bool isBadTruck(char* name);
	VehicleClass findClass(Hash model);

	std::array<char *, 7> noClutchModelNames = {
		"SURGE",
		"VOLTIC",
		"KHAMEL",
		"CADDY",
		"AIRTUG",
		"RHINO",
		"BULLDOZE"
	};
	bool noClutch(char* name);

	Vector3 prevVelocities = {};
	std::vector<float> prevCompressions = {};

	long long prevAccelTime = 0;
	long long prevCompressTime = 0;

	std::array<Vector3, SAMPLES> accelSamples = {};
	int averageAccelIndex = 0;
	void zeroSamples();
};
