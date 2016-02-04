###############################################################################
Manual Transmission for GTA V by ikt
###############################################################################
1.3.1

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
Extract Gears.asi and Gears.ini to your GTA V game folder. Configure the ini
settings to your likings.

###############################################################################
Usage
###############################################################################
Switch between manual or automatic:
Press the |\ key on your keyboard.

Manual sequential:
	Shift Up:	Controller A	or Keyboard Numpad 9
	Shift Down:	Controller X	or Keyboard Numpad 7
	Clutch:		Controller LB	or Keyboard Numpad 8

H-pattern shifting:
	Numpad is mapped to gear. Map your wheel's H-shifter to this.
	
To reverse: Put in reverse gear and press brake.

Clutch isn't needed to shift. Going below RPM in high gears causes stalling,
so shift down when this happens until the gear catches again. If you have
a turbo installed, release the throttle before shifting for that blowoff valve
"Pssssh" sound. If you've got a high performance engine installed, the exhaust
bang (antilag) should fire normally when shifting. 

Driving at very low revs isn't nice yet, but now you can go a bit slower
in a higher gear by being very gentle on the throttle. If you press it too much
while in a too high gear the game still depresses the clutch. Being gentle
does give some control. This is game behavior, I'm still looking to get this
done properly.

###############################################################################
Configuration and Gears.ini usage instructions
###############################################################################
Default values:
-------------------------------------------------------------------------------
[CONTROLS]
; Controller buttons
ShiftUp		= 201		
ShiftDown	= 203	
Clutch		= 205

; Keyboard buttons
KShiftUp 	= 0x69
KShiftDown 	= 0x67
KClutch 	= 0x68

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

;Button to toggle manual
Toggle = 0xDC

[DEBUG]
Info = 0

; Default values
;ShiftUp	= 201		Controller A
;ShiftDown	= 203		Controller X
;Clutch		= 205		Controller LB

; Keyboard buttons
;KShiftUp 	= 0x69		Numpad 9
;KShiftDown = 0x67		Numpad 7
;KClutch 	= 0x68		Numpad 8

; H-shifter
;EnableH = 0			0 - Sequential, 1 - H-shifter
;HR	= 0x60				Numpad 0
;H1	= 0x61				Numpad 1
;H2	= 0x62				Numpad 2
;H3	= 0x63				Numpad 3
;H4	= 0x64				Numpad 4
;H5	= 0x65				Numpad 5
;H6	= 0x66				Numpad 6
;H7	= 0x67				Numpad 7
;H8	= 0x68				Numpad 8

; Disable/Enable
;Toggle = 0xDC			|\ Key
-------------------------------------------------------------------------------
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

KShiftUp:
KShiftDown:
KClutch:
Same as above, but for exclusively keyboard. All keys can be used.
The keys are in the
following link or in VK_Keycodes.txt. These map to the keyboard:
THESE ARE NOT GAMEPAD COMPATIBLE.
https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx

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
		  the speed is too low, can't set it either.

You're very welcome to help with these things. Message me on GTAForums or
post in the thread if you have anything. GTA5-Mods.com is also fine if you
don't have an account there.

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


###############################################################################
Credits
###############################################################################
LeFix & XMOD - ScriptHookV memory access
Alexander Blade - ScriptHookV, performance

Have fun!
- ikt

Source available at
https://github.com/E66666666/GTAVManualTransmission
