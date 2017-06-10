#pragma once
#include <cstdint>
#include <windows.h>

struct vecOffset {
	int X;
	int Y;
	int Z;
};

const struct HandlingOffset {
	DWORD dwHandlingNameHash = 0x0008;
	int fMass = 0x000C;
	int fInitialDragCoeff = 0x0010;
	// 0x0014
	// 0x0018
	// 0x001C
	vecOffset vecCentreOfMass = {
		0x0020,
		0x0024,
		0x0028 };
	// 0x002C
	vecOffset vecInertiaMultiplier = {
		0x0030,
		0x0034,
		0x0038 };
	// 0x003C
	int fPercentSubmerged = 0x0040;
	int fSubmergedRatio = 0x0044;
	int fDriveBiasFront = 0x0048;
	int fDriveBiasRear = 0x004C;
	uint8_t nInitialDriveGears = 0x0050;
	int fDriveInertia = 0x0054;
	int fClutchChangeRateScaleUpShift = 0x0058;
	int fClutchChangeRateScaleDownShift = 0x005C;
	int fInitialDriveForce = 0x0060;
	int fDriveMaxFlatVel = 0x0064;
	int fInitialDriveMaxFlatVel = 0x0068;
	int fBrakeForce = 0x006C;
	// 0x0070
	int fBrakeBiasFront = 0x0074;
	int fBrakeBiasRear = 0x0078;
	int fHandBrakeForce = 0x007C;
	int fSteeringLock = 0x0080;
	int fSteeringLockRatio = 0x0084;
	int fTractionCurveMax = 0x0088;
	int fTractionCurveMaxRatio = 0x008C;
	int fTractionCurveMin = 0x0090;
	int fTractionCurveRatio = 0x0094;
	int fTractionCurveLateral = 0x0098;
	int fTractionCurveLateralRatio = 0x009C;
	int fTractionSpringDeltaMax = 0x00A0;
	int fTractionSpringDeltaMaxRatio = 0x00A4;
	int fLowSpeedTractionLossMult = 0x00A8;
	int fCamberStiffness = 0x00AC;
	int fTractionBiasFront = 0x00B0;
	int fTractionBiasRear = 0x00B4;
	int fTractionLossMult = 0x00B8;
	int fSuspensionForce = 0x00BC;
	int fSuspensionCompDamp = 0x00C0;
	int fSuspensionReboundDamp = 0x00C4;
	int fSuspensionUpperLimit = 0x00C8;
	int fSuspensionLowerLimit = 0x00CC;
	int fSuspensionRaise = 0x00D0;
	int fSuspensionBiasFront = 0x00D4;
	int fSuspensionBiasRear = 0x00D8;
	int fAntiRollBarForce = 0x00DC;
	int fAntiRollBarBiasFront = 0x00E0;
	int fAntiRollBarBiasRear = 0x00E4;
	int fRollCentreHeightFront = 0x00E8;
	int fRollCentreHeightRear = 0x00EC;
	int fCollisionDamageMult = 0x00F0;
	int fWeaponDamageMult = 0x00F4;
	int fDeformationDamageMult = 0x00F8;
	int fEngineDamageMult = 0x00FC;
	int fPetrolTankVolume = 0x0100;
	int fOilVolume = 0x0104;
	// 0x0108
	int fSeatOffsetDistX = 0x010C;
	int fSeatOffsetDistY = 0x0110;
	int fSeatOffsetDistZ = 0x0114;
	int nMonetaryValue = 0x0118;
	DWORD dwStrModelFlags = 0x011C;
	DWORD dwStrHandlingFlags = 0x0120;
	DWORD dwStrDamageFlags = 0x0124;
	DWORD dwAIHandlingHash = 0x0134;
} hOffsets = {};
