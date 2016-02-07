###############################################################################
Manual Transmission for GTA V by ikt
###############################################################################
1.5

This mod allows you to drive like with a manual vehicle. Each vehicle retains
its original characteristics, but now under your control.

###############################################################################
Requirements
###############################################################################
GTA V
Alexander Blade's C++ Scripthook
Microsoft Visual C++ Redistributable 2015 (x64)
https://www.microsoft.com/en-us/download/details.aspx?id=48145

Recommended: LeFix Speedometer. Currently the only speedometer to accurately
read the gears.
https://www.gta5-mods.com/scripts/speedometer-improvedalexbladeversion

###############################################################################
Installation
###############################################################################
Extract Gears.asi and Gears.ini to your GTA V game folder. Configure the ini
settings to your likings.

Get in and out of your car to reload changes to the .ini.

###############################################################################
Usage
###############################################################################
Switch between manual or automatic:
Press the |\ key on your keyboard.

Manual sequential:
	Shift Up:	Controller A			or Keyboard Numpad 9
	Shift Down:	Controller X			or Keyboard Numpad 7
	Clutch:		Controller Left Stick	or Keyboard Numpad 8
		Pull down the left stick like you would press the clutch pedal

H-pattern shifting:
	Numpad is mapped to gear. Map your wheel's H-shifter to this.
	
To reverse: Put in reverse gear accelerate or brake, depending on settings.

If you have a turbo installed, release the throttle before shifting for that
blowoff valve "pssssh" sound. If you've got a high performance engine
installed, the exhaust bang (antilag) should fire normally when shifting. 

By default, the engine will stall if you're in a high gear and try to take off
from standstill or a very low speed. Rev the engine and use the clutch to get
moving, or switch into first gear like a sensible individual.

De default, the engine will receive damage when going full throttle at max
RPMs. A full minute worth's of this will destroy the engine and set it on fire.

###############################################################################
Configuration and Gears.ini usage instructions
###############################################################################
Default values:
-------------------------------------------------------------------------------
[MAIN]
DefaultEnable 	= 1
Toggle 			= 0xDC

AutoGear1 		= 0
AutoReverse 	= 0
OldReverse 		= 0

EngineDamage 	= 1
EngineStalling 	= 1

[CONTROLS]
; Controller buttons
ShiftUp		= 201		
ShiftDown	= 203	
Clutch		= 205
Engine		= 209

; Keyboard buttons
KShiftUp 	= 0x69
KShiftDown 	= 0x67
KClutch 	= 0x68
KEngine		= 0x45

; Controller controls for new reverse
CThrottle	= 208
CBrake		= 207

; Keyboard controls for new reverse
KThrottle	= 0x57
KBrake		= 0x53

; Enable or disable H-shifter
EnableH = 0
; Keyboard buttons
HR	= 0x60
H1	= 0x61
H2	= 0x62
H3	= 0x63
H4	= 0x64
H5	= 0x65
H6	= 0x66
H7	= 0x67
H8	= 0x68

[DEBUG]
Info = 0
-------------------------------------------------------------------------------
DefaultEnable:
Enable manual gears on game start or not.

Toggle:
Control to disable or enable manual transmission control. The keys are in the
following link or in VK_Keycodes.txt. These map to the keyboard:
THESE ARE NOT GAMEPAD COMPATIBLE.
https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx
This toggle is persistent - it saves back to DefaultEnable.

AutoGear1:
Enable to automatically go to first gear if the car has stopped in a high gear.

AutoReverse:
Enable to automatically reverse. Only works if OldReverse is enabled.

OldReverse:
Enable to use the brake to reverse like in the normal game. Disable to reverse
with the throttle like in real cars.

EngineDamage:
Enable to allow the engine to be damaged.
Damages the engine when overrevving at full throttle. Overrrevving for a full
minute will set the engine on fire.

EngineStalling:
Enable to allow the engine to stall.
Stalls the engine at a too low RPM in a too high gear for a too low speed.
Press the Engine button (horn) to restart the engine.

CThrottle and CBrake:
Please set these the same as you have the accelerator and brake buttons set
in the game options to allow realistic manual reversing to work properly.

; Keyboard controls for new reverse
Please set these the same as you have the accelerator and brake buttons set
in the game options to allow realistic manual reversing to work properly.

ShiftUp:
Control to shift up. Press this to raise the current gear by 1. The keys are in
the table in Controls.txt, with the Control values.
This is gamepad-compatible.

ShiftDown:
Control to shift down. Press this to lower the current gear by 1. The keys are
in the Controls.txt, with the Control values.
This is gamepad-compatible.

Clutch:
Control to clutch. Press this to roll free. The keys are in Controls.txt
with the Control values. This is gamepad-compatible.

Engine:
Restart the engine.

KShiftUp:
KShiftDown:
KClutch:
Same as above, but for exclusively keyboard. All keys can be used.
The keys are in the
following link or in VK_Keycodes.txt. These map to the keyboard:
THESE ARE NOT GAMEPAD COMPATIBLE.
https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx

Info:
Shows information about the game engine/transmission values.

###############################################################################
Issues and todo's
###############################################################################
Issues:
	- ?

You're very welcome to help with these things. Message me on GTAForums or
post in the thread if you have anything. GTA5-Mods.com is also fine if you
don't have an account there.

###############################################################################
Elaboration on GTA V transmission and this mod
###############################################################################
Since GTA V doesn't have any natives to read anything worthwhile about the
vehicle's engine and gearbox, some poking around in memory was needed. 
Gears can be locked into whatever gear is desired. As of version 1.5, full
clutch control is possible so driving will be pretty much very similar to irl.


###############################################################################
Changelog
###############################################################################
1.0
Initial release

1.1
Transmission optimizations
Disabled for bicycles
Fixed motorcycle support

1.2
Fix version 573 compatibility issues
Tweaked reverse behavior
Fix overrevving behavior

1.3
Full keyboard support
H-Shifter support - map to keyboard buttons
Preliminary compatiblity with LeFix Speedometer for gear shift indicators
Disable/enable manual notification
Changes in ini

1.3.1
Full compatibility with LeFix Speedometer for gear shift indicators
Restore 1.2 overrevving behavior
Tweak low-RPM low-throttle behavior

1.3.2
Enable/Disable manual gears by default in .ini
Persistent enable/disable manual gears
Enable/Disable engaging first gear on stopping
Enable/Disable automatic reverse gear

1.4
Enable throttle to be used for reverse

1.5
Top gear known (Thank you, kagikn!)
Full clutch control (Thank you, leftas!)
Allow analog clutch control
Add engine damage
Add engine stalling
Changed build parameters - should work for more people now

###############################################################################
Credits
###############################################################################
Alexander Blade
LeFix
XMOD
InfamousSabre
leftas
kagikn

Have fun!
- ikt

Source available at
https://github.com/E66666666/GTAVManualTransmission
