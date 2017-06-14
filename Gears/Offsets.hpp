#pragma once
#include <cstdint>
#include <windows.h>

struct vec3Offset {
	int X;
	int Y;
	int Z;
};

const struct CVehicleHandlingData {
	DWORD dwHandlingNameHash = 0x0008;
	int fMass = 0x000C;
	int fInitialDragCoeff = 0x0010;
	// 0x0014
	// 0x0018
	// 0x001C
	vec3Offset vecCentreOfMass = {
		0x0020,
		0x0024,
		0x0028 
	};
	// 0x002C
	vec3Offset vecInertiaMultiplier = {
		0x0030,
		0x0034,
		0x0038 
	};
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

// 1032
const struct CWheel {
	// Wheel stuff:
	// 20: offset from body?
	// 30: Similar-ish?

	// 40, 50: last position on ground?
	// 60 - current position? seems to be 0,0,0 when contact is lost

	// B0 - world velocity
	// C0 - Same, but flipped?

	// 168 - rotation speed rad/s
	// 16C - slippage y-dir?
	// 170 - heating

	// materials.dat related
	// 190 - surface related: tyre grip
	// 194 - surface related: wet grip
	// 198 - surface related: tyre drag
	// 19C - surface related: top speed mult


	// 1C4 - steering angle
	// 1C8/1D4 - Brake
	// 1CC/1D8 - Throttle/powered
	// 1E0 - Cur Health
	// 1E4 - Max Health

	// 1EC / (flags?) - Powered states
	// 1F0 - ? bitflags too
	// 1F4 - Surface flags?

	// ulong offPosX = 0x40;
	// ulong offPosY = 0x44;
	// ulong offPosZ = 0x48;
};

class CVehicleModelInfoArray {};
class CNavigation{};
class CVehicleDrawHandler{};
class CMatrix{};
class CAttacker{};
class CHandlingData{};
class CPed{};

//1032
class CVehicle_1032 {
public:
	char pad_0x0000[0x20];                                  // 0x0000
	CVehicleModelInfoArray* VehicleModelInfoArrayContainer; // 0x0020
	unsigned char typeByte;                                 // 0x0028
	char pad_0x0029[0x7];                                   // 0x0029
	CNavigation* CNavigation;                               // 0x0030
	char pad_0x0038[0x10];                                  // 0x0038
	CVehicleDrawHandler* CVehicleColorOptions;              // 0x0048
	char pad_0x0050[0x10];                                  // 0x0050
	CMatrix N000072BA;                                      // 0x0060
	char pad_0x00A0[0x20];                                  // 0x00A0
	DWORD dwBitmapc0;                                       // 0x00C0 1<<17=onfire
	char pad_0x00C4[0x14];                                  // 0x00C4
	__int8 N000072C5;                                       // 0x00D8 & 0x02 == 2(not in car), 0 (in car)
	char pad_0x00D9[0xB0];                                  // 0x00D9
	unsigned char btGodMode;                                // 0x0189
	char pad_0x018A[0xF6];                                  // 0x018A
	float fHealth;                                          // 0x0280
	char pad_0x0284[0x1C];                                  // 0x0284
	float fHealthMax;                                       // 0x02A0
	char pad_0x02A4[0x4];                                   // 0x02A4
	CAttacker* CAttacker;                                   // 0x02A8
	char pad_0x02B0[0x68];                                  // 0x02B0
	__int32 bRocketBoostEnabled;                            // 0x0318
	float fRocketBoostCharge;                               // 0x031C
	char pad_0x0320[0x440];                                 // 0x0320
	Vector3 v3Velocity;                                     // 0x0760
	float N00002422;                                        // 0x076C
	float fRotationSpeedRoll;                               // 0x0770
	float fRotationSpeedPitch;                              // 0x0774
	float fRotationSpeedYaw;                                // 0x0778
	float N00006E17;                                        // 0x077C
	char pad_0x0780[0x30];                                  // 0x0780
	float fUnknownHealth;                                   // 0x07B0 explodes when damage (not fire) decreases to 0
	float fFuelTankHealth;                                  // 0x07B4 big bang when reaches -1000
	float fFuelLevel;                                       // 0x07B8
	float fFireTimer;                                       // 0x07BC
	char pad_0x07C0[0x30];                                  // 0x07C0
	__int8 iGearNext;                                       // 0x07F0
	char pad_0x07F1[0x1];                                   // 0x07F1
	__int8 iGearCurrent;                                    // 0x07F2
	char pad_0x07F3[0x3];                                   // 0x07F3
	__int8 iGearTopGear;                                    // 0x07F6
	char pad_0x07F7[0x2D];                                  // 0x07F7
	float fCurrentRPM;                                      // 0x0824
	float fCurrentRPM2;                                     // 0x0828
	char pad_0x082C[0x4];                                   // 0x082C
	float fClutch;                                          // 0x0830
	float fThrottle;                                        // 0x0834
	char pad_0x0838[0x8];                                   // 0x0838
	__int32 UnknownFC40CBF7B90CA77C;                        // 0x0840 signed
	char pad_0x0844[0x4];                                   // 0x0844
	float fTurbo;                                           // 0x0848
	float fSteeringAngleZERO;                               // 0x084C
	char pad_0x0850[0x8];                                   // 0x0850
	float fVehicleCrashDowndown;                            // 0x0858
	float fHealth2;                                         // 0x085C
	char pad_0x0860[0x28];                                  // 0x0860
	CHandlingData* vehicleHandling;                         // 0x0888
	unsigned char vehicleType;                              // 0x0890 2=car,boat 6=heli,plane (0x04=flying + 0x02=vehicle)
	unsigned char dw4or5;                                   // 0x0891 always 4 then 5 in plane
	unsigned char bitmapCarInfo1;                           // 0x0892 3=in, 6=getting in, 2=out, 1=dead car; 2=in heli; 4=props on plane turning. (& 0x02 = drivable)
	unsigned char bitmapBulletProofTires;                   // 0x0893 wheelsCan'tBreak << 6
	char pad_0x0894[0x2];                                   // 0x0894
	unsigned char bitmapUnknown2;                           // 0x0896 << 4 (|= 16) by vehicle_6ebfb22d646ffc18
	char pad_0x0897[0x3];                                   // 0x0897
	unsigned char bitmapJumpRelated;                        // 0x089A & 0x10 = something
	char pad_0x089B[0x1];                                   // 0x089B
	__int8 bitmapOwnedByPlayer;                             // 0x089C 0x02 = hasBeenOwnedByPlayer
	char pad_0x089D[0xE];                                   // 0x089D
	unsigned char bitmapUnknownVehicleType1;                // 0x08AB VEHICLE__CAC66558B944DA67 &= 0xf7; |= 8*(enabled&1)
	char pad_0x08AC[0x4C];                                  // 0x08AC
	float N00002483;                                        // 0x08F8
	float fWheelBias2;                                      // 0x08FC -1.0 to 1.0
	char pad_0x0900[0x4];                                   // 0x0900
	float fWheelBias;                                       // 0x0904 -0.65~ to +~0.65
	char pad_0x0908[0x4];                                   // 0x0908
	float fThrottlePosition;                                // 0x090C
	float fBreakPosition;                                   // 0x0910
	unsigned char bHandbrake;                               // 0x0914
	char pad_0x0915[0x33];                                  // 0x0915
	float fDirtLevel;                                       // 0x0948
	char pad_0x094C[0x70];                                  // 0x094C
	float fEngineTemp;                                      // 0x09BC
	char pad_0x09C0[0x34];                                  // 0x09C0
	DWORD dwCarAlarmLength;                                 // 0x09F4
	char pad_0x09F8[0x8];                                   // 0x09F8
	float fDashSpeed;                                       // 0x0A00
	char pad_0x0A04[0x6];                                   // 0x0A04
	unsigned char bUsedInB2E0C0D6922D31F2;                  // 0x0A0A
	char pad_0x0A0B[0x45];                                  // 0x0A0B
	Vector3 vTravel;                                        // 0x0A50
	char pad_0x0A5C[0x44];                                  // 0x0A5C
	float fVelocityX;                                       // 0x0AA0
	float fVelocityY;                                       // 0x0AA4
	float fVelocityZ;                                       // 0x0AA8
	char pad_0x0AAC[0x6C];                                  // 0x0AAC
	__int32 iVehicleType;                                   // 0x0B18 0=car recently;1=plane, 0d = boat, 8=heli, 0x0e=train, 0x0b=motorbike, <= 0x0a = CAutomobile?
	char pad_0x0B1C[0x24];                                  // 0x0B1C
	unsigned char btOpenableDoors;                          // 0x0B40
	char pad_0x0B41[0x4B];                                  // 0x0B41
	float fGravity;                                         // 0x0B8C
	char pad_0x0B90[0x8];                                   // 0x0B90
	CPed PedsInSeats[15];                                   // 0x0B98
	char pad_0x0C10[0x798];                                 // 0x0C10
	unsigned char ReducedGrip;                              // 0x13A8 &=0xBF; |= (reduceGrip & 1) << 6
	unsigned char CreatesMoneyWhenExploded;                 // 0x13A9 &= 0xFD; |= 2 * enabled
	char pad_0x13AA[0x3F6];                                 // 0x13AA
	float fSudoJumpValue;                                   // 0x17A0
	char pad_0x17A4[0x130];                                 // 0x17A4
};                                                          // Size=0x18D4

//1103
