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
    int fDownforceModifier = 0x0014;
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
    // 0x0108                           // 0.5f?
    int fSeatOffsetDistX = 0x010C;
    int fSeatOffsetDistY = 0x0110;
    int fSeatOffsetDistZ = 0x0114;
    int nMonetaryValue = 0x0118;        
    // 0x011C // Hex  , 0x3FA00000 ?
    // 0x0120 // Float, 70.0f?
    DWORD dwStrModelFlags = 0x0124;
    DWORD dwStrHandlingFlags = 0x0128;
    DWORD dwStrDamageFlags = 0x012C;
    // 0x0130 // Some hash?
    // 0x0134 // some hash? joaat "0"
    // 0x0138 // joaat("0") ??? wtf
    DWORD dwAIHandlingHash = 0x013C;
    // 0x0140 // Float 42.45703506 @ Deviant or 0x4229D401

    // 0x0150 // CBaseSubHandlingData* ptr?
    // 0x0154 // some ptr

    // 0x0330 // some ptr?
    // ccarhandlingdata area
    // 0x0338 // fBackEndPopUpCarImpulseMult
    // 0x033C // fBackEndPopUpBuildingImpulseMult
    // 0x0340 // fBackEndPopUpMaxDeltaSpeed
    // 0x0344 // fToeFront
    // 0x0348 // fToeRear
    // 0x034C // fCamberFront
    // 0x0350 // fCamberRear
    // 0x0354 // fCastor
    // 0x0358 // fEngineResistance
    // 0x035C // 
    // 0x0360 // fJumpForceScale
    // 0x0364 //
    // 0x0368 // 
    // 0x036C // strAdvancedFlags

    // 0x0660 // some ptr
    // 0x0668 // uWeaponHash array
    
    // 0x0698 // hex (0x1F @ Issi4)
    // 0x069C // hex (0x0A @ Issi4)
    // 0x06A0 // hex
    // 0x06A4 // hex
    // 0x06A8 // hex
    // 0x06AC // hex

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
    // 1F2 - Surface ID (1 byte)

    // ulong offPosX = 0x40;
    // ulong offPosY = 0x44;
    // ulong offPosZ = 0x48;
};
