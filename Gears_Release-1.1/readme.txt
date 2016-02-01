###############################################################################
Manual Transmission for GTA V by ikt
###############################################################################
1.0

This mod allows you to drive like with a manual vehicle. Each vehicle retains
its original characteristics, but now under your control.

###############################################################################
Requirements
###############################################################################
GTA V, version 393 or newer
Alexander Blade's C++ Scripthook
Microsoft Visual C++ Redistributable 2015 (x64)
https://www.microsoft.com/en-us/download/details.aspx?id=48145

Recommended: LeFix Speedometer. Currently the only speedometer to accurately
read the gears.
https://www.gta5-mods.com/scripts/speedometer-improvedalexbladeversion

###############################################################################
Installation
###############################################################################
Extract Gears.asi and Gears.ini to your GTA V game folder.

###############################################################################
Configuration and Gears.ini usage instructions
###############################################################################
Default values:
-------------------------------------------------------------------------------
[CONTROLS]
ShiftUp = 61		
ShiftDown = 62		
Clutch = 68
Toggle = 0xDC

[DEBUG]
Info = 1
-------------------------------------------------------------------------------
ShiftUp:
Control to shift up. Press this to raise the current gear by 1. The keys are in
the table at the bottom of this file, with the Control values.
This is gamepad-compatible.

ShiftDown:
Control to shift down. Press this to lower the current gear by 1. The keys are
in the table at the bottom of this file, with the Control values.
This is gamepad-compatible.

Clutch:
Control to clutch. Press this to roll free. The keys are in the table below
with the Control values. This is gamepad-compatible.

Toggle:
Control to disable or enable manual transmission control. The keys are in the
following link or in VK_Keycodes.txt. These map to the keyboard:
THESE ARE NOT GAMEPAD COMPATIBLE.
https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx

Info:
Shows information about the game engine/transmission values.

###############################################################################
Issues and todo's
###############################################################################
Issues:
	- Top gear is not known.
		- Revs/speed drop, clutch disengages when choosing a gear higher than
		  the max car gear.
	- Unreliable clutch/throttle control.
		- Can't set clutch/throttle when redlining. Clutch disengages when
		  the speed is too low.

You're very welcome to help with these things. Message me on GTAForums or
post in the thread if you have anything. GTA5-Mods.com is also fine if you
don't have an account there.

Todo:
	- H-pattern gears

###############################################################################
Elaboration on GTA V transmission and this mod
###############################################################################
Since GTA V doesn't have any natives to read anything worthwhile about the
vehicle's engine and gearbox, some poking around in memory was needed. This 
results in somewhat of a control over the transmission, but not all. Gears can
be locked into whatever gear is desired, but clutch control isn't possible yet.
This results in some weird behavior where the car loses power at top RPMs, full
throttle but not yet shifting up.

Another strange thing is a variable that behaves like throttle, it seems to
reflect accelerator input.
Driving at a too low speed for the gear results in the game pressing the clutch
and also releasing the throttle, resulting in a slowdown until the user goes in
the right gear again (downshift). I played with killing the engine to simulate
how real cars work when in this situation, but the game keeps restarting the
engine. I also didn't want to mess around with engine/tank health so I dropped
it in favour of the default game behavior.

Behavior in reverse gear is also strange. In gear 0, reverse, reversing is not
a problem by pressing the brake button, but going forward is also possible.
This gear seems like a weird something that deals with Park/Reverse. I have
chosen to just lock it to reverse. Same with forward, in gear >= 1, going back
is still possible. I also locked this to the acceleration key.

To be honest, with Drift Assist or InversePower, GTA V's transmission is pretty
good already. It almost never chooses the wrong gear and cars don't shift
anywhere as slowly/frustratingly as in GTA IV.

###############################################################################
Changelog
###############################################################################
1.0
Initial release

1.1
Transmission optimizations
Disabled for bicycles
Fixed motorcycle support


###############################################################################
Credits
###############################################################################
LeFix & XMOD - ScriptHookV memory access
Alexander Blade - ScriptHookV, performance thing

Have fun!
- ikt

Source available at
https://github.com/E66666666/GTAVManualTransmission




###############################################################################
Control table
###############################################################################
The full list can be found in Alexander Blade's ScriptHookV SDK, in enums.h.
This list is shortened for convenience.
enum eControl
{
	ControlNextCamera = 0,
	ControlLookLeftRight = 1,
	ControlLookUpDown = 2,
	ControlLookUpOnly = 3,
	ControlLookDownOnly = 4,
	ControlLookLeftOnly = 5,
	ControlLookRightOnly = 6,
	ControlPhone = 27,
	ControlVehicleMoveLeftRight = 59,
	ControlVehicleMoveUpDown = 60,
	ControlVehicleMoveUpOnly = 61,
	ControlVehicleMoveDownOnly = 62,
	ControlVehicleMoveLeftOnly = 63,
	ControlVehicleMoveRightOnly = 64,
	ControlVehicleSpecial = 65,
	ControlVehicleGunLeftRight = 66,
	ControlVehicleGunUpDown = 67,
	ControlVehicleAim = 68,
	ControlVehicleAttack = 69,
	ControlVehicleAttack2 = 70,
	ControlVehicleAccelerate = 71,
	ControlVehicleBrake = 72,
	ControlVehicleDuck = 73,
	ControlVehicleHeadlight = 74,
	ControlVehicleExit = 75,
	ControlVehicleHandbrake = 76,
	ControlVehicleHotwireLeft = 77,
	ControlVehicleHotwireRight = 78,
	ControlVehicleLookBehind = 79,
	ControlVehicleCinCam = 80,
	ControlVehicleNextRadio = 81,
	ControlVehiclePrevRadio = 82,
	ControlVehicleNextRadioTrack = 83,
	ControlVehiclePrevRadioTrack = 84,
	ControlVehicleRadioWheel = 85,
	ControlVehicleHorn = 86,
	ControlVehicleFlyThrottleUp = 87,
	ControlVehicleFlyThrottleDown = 88,
	ControlVehicleFlyYawLeft = 89,
	ControlVehicleFlyYawRight = 90,
	ControlVehiclePassengerAim = 91,
	ControlVehiclePassengerAttack = 92,
	ControlVehicleSpecialAbilityFranklin = 93,
	ControlVehicleStuntUpDown = 94,
	ControlVehicleCinematicUpDown = 95,
	ControlVehicleCinematicUpOnly = 96,
	ControlVehicleCinematicDownOnly = 97,
	ControlVehicleCinematicLeftRight = 98,
	ControlVehicleSelectNextWeapon = 99,
	ControlVehicleSelectPrevWeapon = 100,
	ControlVehicleRoof = 101,
	ControlVehicleJump = 102,
	ControlVehicleGrapplingHook = 103,
	ControlVehicleShuffle = 104,
	ControlVehicleDropProjectile = 105,
	ControlVehicleMouseControlOverride = 106,
	ControlVehicleFlyRollLeftRight = 107,
	ControlVehicleFlyRollLeftOnly = 108,
	ControlVehicleFlyRollRightOnly = 109,
	ControlVehicleFlyPitchUpDown = 110,
	ControlVehicleFlyPitchUpOnly = 111,
	ControlVehicleFlyPitchDownOnly = 112,
	ControlVehicleFlyUnderCarriage = 113,
	ControlVehicleFlyAttack = 114,
	ControlVehicleFlySelectNextWeapon = 115,
	ControlVehicleFlySelectPrevWeapon = 116,
	ControlVehicleFlySelectTargetLeft = 117,
	ControlVehicleFlySelectTargetRight = 118,
	ControlVehicleFlyVerticalFlightMode = 119,
	ControlVehicleFlyDuck = 120,
	ControlVehicleFlyAttackCamera = 121,
	ControlVehicleFlyMouseControlOverride = 122,
	ControlVehicleSubTurnLeftRight = 123,
	ControlVehicleSubTurnLeftOnly = 124,
	ControlVehicleSubTurnRightOnly = 125,
	ControlVehicleSubPitchUpDown = 126,
	ControlVehicleSubPitchUpOnly = 127,
	ControlVehicleSubPitchDownOnly = 128,
	ControlVehicleSubThrottleUp = 129,
	ControlVehicleSubThrottleDown = 130,
	ControlVehicleSubAscend = 131,
	ControlVehicleSubDescend = 132,
	ControlVehicleSubTurnHardLeft = 133,
	ControlVehicleSubTurnHardRight = 134,
	ControlVehicleSubMouseControlOverride = 135,
	ControlVehiclePushbikePedal = 136,
	ControlVehiclePushbikeSprint = 137,
	ControlVehiclePushbikeFrontBrake = 138,
	ControlVehiclePushbikeRearBrake = 139,
	ControlPhoneUp = 172,
	ControlPhoneDown = 173,
	ControlPhoneLeft = 174,
	ControlPhoneRight = 175,
	ControlPhoneSelect = 176,
	ControlPhoneCancel = 177,
	ControlPhoneOption = 178,
	ControlPhoneExtraOption = 179,
	ControlPhoneScrollForward = 180,
	ControlPhoneScrollBackward = 181,
	ControlPhoneCameraFocusLock = 182,
	ControlPhoneCameraGrid = 183,
	ControlPhoneCameraSelfie = 184,
	ControlPhoneCameraDOF = 185,
	ControlPhoneCameraExpression = 186,
	ControlScriptLeftAxisX = 218,
	ControlScriptLeftAxisY = 219,
	ControlScriptRightAxisX = 220,
	ControlScriptRightAxisY = 221,
	ControlScriptRUp = 222,
	ControlScriptRDown = 223,
	ControlScriptRLeft = 224,
	ControlScriptRRight = 225,
	ControlScriptLB = 226,
	ControlScriptRB = 227,
	ControlScriptLT = 228,
	ControlScriptRT = 229,
	ControlScriptLS = 230,
	ControlScriptRS = 231,
	ControlScriptPadUp = 232,
	ControlScriptPadDown = 233,
	ControlScriptPadLeft = 234,
	ControlScriptPadRight = 235,
	ControlScriptSelect = 236,
	ControlCursorAccept = 237,
	ControlCursorCancel = 238,
	ControlCursorX = 239,
	ControlCursorY = 240,
	ControlCursorScrollUp = 241,
	ControlCursorScrollDown = 242,
	ControlEnterCheatCode = 243,
	ControlInteractionMenu = 244,
	ControlMoveLeft = 266,
	ControlMoveRight = 267,
	ControlMoveUp = 268,
	ControlMoveDown = 269,
	ControlLookLeft = 270,
	ControlLookRight = 271,
	ControlLookUp = 272,
	ControlLookDown = 273,
	ControlVehicleMoveLeft = 278,
	ControlVehicleMoveRight = 279,
	ControlVehicleMoveUp = 280,
	ControlVehicleMoveDown = 281,
	ControlVehicleGunLeft = 282,
	ControlVehicleGunRight = 283,
	ControlVehicleGunUp = 284,
	ControlVehicleGunDown = 285,
	ControlVehicleLookLeft = 286,
	ControlVehicleLookRight = 287,
	ControlScaledLookLeftRight = 290,
	ControlScaledLookUpDown = 291,
	ControlScaledLookUpOnly = 292,
	ControlScaledLookDownOnly = 293,
	ControlScaledLookLeftOnly = 294,
	ControlScaledLookRightOnly = 295,
	ControlVehicleDriveLook = 329,
	ControlVehicleDriveLook2 = 330,
	ControlVehicleFlyAttack2 = 331,
	ControlVehicleSlowMoUpDown = 334,
	ControlVehicleSlowMoUpOnly = 335,
	ControlVehicleSlowMoDownOnly = 336
};