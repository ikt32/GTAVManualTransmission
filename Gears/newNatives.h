#pragma once

#include <inc/types.h>
#include <inc/nativeCaller.h>

struct vehsStruct
{
	int maxSize;
	int padding;
	Vehicle vehs[300];
};

namespace VEHICLE {
	// 1.0.1103.2
	// 0.0f - 1.0f
	static void _SET_VEHICLE_ROCKET_BOOST_PERCENTAGE(Vehicle vehicle, float percentage) { invoke<Void>(0xFEB2DDED3509562E, vehicle, percentage); } // 0xFEB2DDED3509562E
	static int _GET_ALL_VEHICLES(int* vehsStruct) { return invoke<int>(0x9B8E1BF04B51F2E8, vehsStruct); } // 0x9B8E1BF04B51F2E8

	// 1.0.944.2
	static BOOL _HAS_VEHICLE_ROCKET_BOOST(Vehicle vehicle) { return invoke<BOOL>(0x36D782F68B309BDA, vehicle); }
	static BOOL _IS_VEHICLE_ROCKET_BOOST_ACTIVE(Vehicle vehicle) { return invoke<BOOL>(0x3D34E80EED4AE3BE, vehicle); }
	static void _SET_VEHICLE_ROCKET_BOOST_ACTIVE(Vehicle vehicle, BOOL active) { invoke<Void>(0x81E1552E35DC3839, vehicle, active); }
	static void _SET_VEHICLE_ROCKET_BOOST_REFILL_TIME(Vehicle vehicle, float seconds) { invoke<Void>(0xE00F2AB100B76E89, vehicle, seconds); }
	static BOOL _HAS_VEHICLE_JUMPING_ABILITY(Vehicle vehicle) { return invoke<BOOL>(0x9078C0C5EF8C19E9, vehicle); }
	static BOOL _HAS_VEHICLE_PARACHUTE(Vehicle vehicle) { return invoke<BOOL>(0xBC9CFF381338CB4F, vehicle); }
	static BOOL _CAN_VEHICLE_PARACHUTE_BE_ACTIVATED(Vehicle vehicle) { return invoke<BOOL>(0xA916396DF4154EE3, vehicle); }
	static void _SET_VEHICLE_PARACHUTE_ACTIVE(Vehicle vehicle, BOOL active) { invoke<Void>(0x0BFFB028B3DD0A97, vehicle, active); }
	static BOOL _IS_THIS_MODEL_AN_AMPHIBIOUS_CAR(Hash model) { return invoke<BOOL>(0x633F6F44A537EBB6, model); } // 0x633F6F44A537EBB6
}

namespace STREAMING {
	// 1.0.1103.2
	static BOOL _IS_MODEL_A_PED(Hash modelHash) { return invoke<BOOL>(0x75816577FEA6DAD5, modelHash); } // 0x75816577FEA6DAD5
}

namespace WEAPON {
	// 1.0.1103.2
	static Hash _GET_PED_AMMO_TYPE_FROM_WEAPON_2(Ped ped, Hash weaponHash) { return invoke<Hash>(0xF489B44DD5AF4BD9, ped, weaponHash); } // 0xF489B44DD5AF4BD9
}