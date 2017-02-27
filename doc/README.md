[comment]: # (User README.md)
Manual Transmission & Steering Wheel Support
===========

# Manual Transmission for GTA V
This mod will enable manual transmission for vehicles, using the games' real
gear box! This means real gears - not speed capping. There are plenty of
features to emulate how real transmissions work.

It’s highly recommended to play with this mod using a controller or a wheel.

# Steering Wheel Support for GTA V
* all DirectInput compatible steering wheels supported since 4.0
* force feedback is fully supported
* steering wheel without manual transmission supported
* multiple input devices supported since 4.2.0

# Downloads
* [GTA5-Mods.com](https://www.gta5-mods.com/scripts/manual-transmission-ikt)
* [GitHub release (older versions)](https://github.com/E66666666/GTAVManualTransmission/releases)
* [Latest automated builds (unstable, probably)](https://ci.appveyor.com/project/E66666666/gtavmanualtransmission/build/artifacts)

# Table of Contents
<!-- TOC depthFrom:1 depthTo:2 orderedList:false updateOnSave:true withLinks:true -->

- [Manual Transmission for GTA V](#manual-transmission-for-gta-v)
- [Steering Wheel Support for GTA V](#steering-wheel-support-for-gta-v)
- [Downloads](#downloads)
- [Table of Contents](#table-of-contents)
- [Requirements](#requirements)
- [Installation](#installation)
    - [Additional steps for wheel users (manual method)](#additional-steps-for-wheel-users-manual-method)
    - [Additional steps for wheel users (config tool method)](#additional-steps-for-wheel-users-config-tool-method)
- [Basic usage and controls](#basic-usage-and-controls)
    - [Controls](#controls)
    - [Usage](#usage)
- [Configuration](#configuration)
    - [`settings_general.ini`](#settings_generalini)
    - [`[OPTIONS]`](#options)
    - [`[CONTROLLER]`](#controller)
    - [`[KEYBOARD]`](#keyboard)
    - [`[DEBUG]`](#debug)
    - [`settings_wheel.ini`](#settings_wheelini)
    - [`[OPTIONS]` (Wheel)](#options-wheel)
    - [`[FORCE_FEEDBACK]`](#force_feedback)
    - [`[INPUT_DEVICES]`](#input_devices)
    - [`[STEER]`, `[THROTTLE]`, `[BRAKES]`, `[CLUTCH]` and `[HANDBRAKE_ANALOG]`](#steer-throttle-brakes-clutch-and-handbrake_analog)
    - [`[TO_KEYBOARD]`](#to_keyboard)
- [Troubleshooting](#troubleshooting)
    - [Installation dependencies](#installation-dependencies)
    - [Non-conflicting software](#non-conflicting-software)
    - [Conflicting software](#conflicting-software)
    - [Steering wheel reports strange values](#steering-wheel-reports-strange-values)
    - [Steering wheel not detected](#steering-wheel-not-detected)
- [Thanks](#thanks)
- [Source code](#source-code)

<!-- /TOC -->

# Requirements
* Grand Theft Auto V
* [ScriptHookV](http://www.dev-c.com/gtav/scripthookv/)

# Installation
1.	Put `Gears.asi` and the folder `ManualTransmission` in your GTA V folder (overwrite when asked)
2.	Read this README and configure `ManualTransmission/settings_general.ini` to your preferences

## Additional steps for wheel users (manual method)
3.	Remove or disable any XInput or DirectInput DLL files for your wheel for
GTA V
4.	Read this README and configure `ManualTransmission/settings_wheel.ini` your wheel
5.	Launch and close `DiUtil.exe` in the `ManualTransmission` folder
6.	Check entries for correct device GUID
7.	Add entry in settings_wheel.ini as "DEV[n] = name", "GUID[n] = {GUID STRING}"
8.	For each entry, choose "DEVICE = n" (there are 22!)
9.	Re-launch DiUtil.exe to check/verify axes
10.	Change axis limits for steering, throttle, brake, clutch and handbrake

## Additional steps for wheel users (config tool method)
3. Launch GearsApp.exe (TODO)
4. Save changes

When reloading the mod by toggling it off and on, the settings are read again
and the steering wheel (if connected) is reset again. You can use this to fine-
tune your settings and change things on-the-fly, without restarting the game.

### Recommended mods
* [NFS Speedo](https://www.gta5-mods.com/scripts/nfsgauge-rpm-gear-speedometer)
* Any speedometer supporting RPM/Gear reading from memory.

Outdated:
* [~~Custom Steering~~](https://www.gta5-mods.com/scripts/custom-steering) 

Custom Steering hasn't been updated for the latest GTA V updates. Once it gets updated,
using this is fine. Before this happens, remove it if it crashes your game.

# Basic usage and controls
Basic knowledge of a manually operated vehicle is required, such as what a
clutch is and what it does. You can play with these things disabled too, for
a more arcade gameplay.

Read this README to see what the different switches do. You
WILL need to configure it correctly, otherwise some features will not work!
The buttons listed below are default controls. You can change these at will.

## Controls
### Keyboard defaults
* Press `|\` to disable or enable manual transmission.
* Press `}]` to switch between a sequential gearbox, H-pattern gearbox or the
automatic gearbox.

### Controller defaults
* Hold Dpad Right to disable or enable manual transmission.
* Hold B to switch between a sequential gearbox, H-pattern gearbox or
the automatic gearbox.

### Steering wheel settings
* Use the assigned buttons to toggle the mod and switch between gearboxes.

## Usage
To switch between inputs, you only need to tap the throttle, the mod will
automatically switch between these inputs.

Specifically for wheel users, you might need to fully depress the throttle
pedal (once) if the mod keeps swapping away from the keyboard/controller.

### Driving with Manual Transmission
To drive forward, ensure that
* the vehicle is not in neutral
* the vehicle is not in reverse

and press the accelerator key, button or
pedal. Depending on your settings, you might or might not
need to operate the clutch to make a smooth start. Like a real vehicle, remember
to not let the RPM dip too low for the current gear. It might stall otherwise.

To brake, press the brake/reverse key, button or pedal. When coming to a halt,
the vehicle will not reverse.

To reverse, shift into the reverse gear. Press the accelerator key, button or
pedal. Depending on your settings, you might or might not need to operate the
clutch to get moving.

#### Wheel-specific
Additional functionality is available for vehicles, like operating the lights,
horn, blinkers et cetera from the wheel. Refer to the `settings_wheel.ini` section for
details.

#### Wheel usage without Manual Transmission
The throttle and the brake work like the accelerator and brake/reverse inputs.
A clutch pedal won't have any action. The rest of the functions still work.

# Configuration
This guide will explain the usage of the ini files and what the options mean. 

Generally, if only `0` or `1` the following holds for that feature:
* `0`: Disabled
* `1`: Enabled

## `settings_general.ini`
This file contains most general settings. Configuring only this is sufficient if
no steering wheel is used.

## `[OPTIONS]`
The `[OPTIONS]` section is where you can configure how the mod behaves globally
and turn off and on features you want.

### `Enable` : `0` or `1`
This option is whether to enable or disable the mod. Toggling the mod in-game
will write the new value to this option, so your preference will be stored
between sessions.
* `0`: The mod is disabled and the original automatic transmission from GTA V
is fully restored.
* `1`: The mod is active and this mod will take over the transmission with
manual control.

### `ShiftMode` : `0`, `1` or `2`
This option switched between the sequential gearbox, H-pattern gearbox and
automatic gearbox. For the steering wheel and the keyboard this option can be
enabled and shifting happens with the numpad or with the H-shifter. Toggling the
mod in-game will write the new value to this option, so your preferences will be
stored between sessions.

__If controller input is detected, this option automatic reverts to sequential.__

* `0`: Sequential
* `1`: H-pattern
* `2`: Automatic

### `SimpleBike` : `0` or `1`
Disable stalling and clutch catching for bikes regardless of regular settings.
Useful for making bikes easier to operate.

* `0`: Clutch grabbing and stalling enabled
* `1`: Clutch grabbing and stalling disabled

### `EngineDamage` : `0` or `1`
This option turns on or off the engine damage when overrevving or shifting
without pressing the clutch. The damage values can be configured:
__RPMDamage__ and __MisshiftDamage__.

* `0`: Engine damage disabled
* `1`: Engine damage on over revving and shifting with the H-pattern gearbox
without using the clutch

### `EngineStalling` : `0` or `1`
This option turns on or off the engine stalling when releasing the clutch with
a low RPM at very low speeds. The point it shuts down is configured with
__ClutchCatchpoint__.

* `0`: Engine stalling disabled
* `1`: Engine stalls at low RPM with engaged clutch

### `EngineBraking` : `0` or `1`
This options controls engine braking. If driving at speed and downshifting to a
lower gear, the car will be slowed down accordingly.

* `0`: Engine braking disabled
* `1`: Engine braking active when over max gear speed

### `ClutchCatching` : `0` or `1`
This option will make the vehicle drive slowly if clutch is released gently,
and keeps the vehicle rolling at a speed depending on the gear.

* `0`: Clutch catching disabled
* `1`: Clutch catches/grabs/bites at specified point

### `ClutchShiftingH` : `0` or `1`
This option controls the requirement to hold the clutch for H-shifting.

* `0`: No need to hold the clutch while shifting
* `1`: Need to hold the clutch while shifting. Gearbox pops into neutral when
not holding the clutch.

### `ClutchShiftingS` : `0` or `1`
This option controls the requirement to hold the clutch for sequential shifting.
This feature is available from 4.2.0 on.

* `0`: No need to hold the clutch while shifting
* `1`: Need to hold the clutch while shifting. Gearbox pops into neutral when
not holding the clutch.

### `DefaultNeutral` : `0` or `1`
This option controls whether new vehicles start in neutral or not when you enter
 them. This is useful to turn on when you have __ClutchCatching__ and
__EngineStalling__ turned on.

* `0`: Vehicle starts in gear 1
* `1`: Vehicle starts in neutral gear

### `ClutchCatchpoint` : `0` to `100`
This specifies the point where the clutch starts making your vehicle roll. The
higher this value, the higher you need to lift the clutch pedal to get going.

### `StallingThreshold` : `0` to `100`
This specifies the point where your engine stalls with regard to the clutch
point. If you’re going too slowly and your clutch is lifted higher than this
point, your engine will stall. Keep this higher than __ClutchCatchpoint__ to get
both working together nicely.

### `RPMDamage` : `0` to any value
* Requires: `EngineDamage = 1`

This specifies how much damage your engine receives while overrevving.
Every tick, the engine gets damaged with __RPMDamage/100__.

### `MisshiftDamage` : `0` to any value
* Requires: `EnableH = 1`
* Requires: `EngineDamage = 1`
* Requires: `ClutchShifting = 1`

This specifies how much damage your engine receives when you shift. Every time
you shift into a gear without pressing the clutch past __ClutchCatchpoint__,
your engine will be damaged by __MisshiftDamage__. When you shift into Neutral
with an insufficiently pressed clutch, your engine will be damaged by
__MisshiftDamage/10__.

### `HillBrakeWorkaround` : `0` or `1`
Turn this on to emulate a hill start and car roll on a hill. It gives your car
a little push. Idea and implementation by XMOD.

* `0`: No change
* `1`: Workaround enabled. A force will push the car down a slope.

### `AutoGear1` : `0` or `1`
Turn this on to automatically shift into first gear when stopped, with a
sequential gearbox.

* `0`: No change
* `1`: Shift into first gear on stop while using the sequential gear box.

### `AutoLookBack` : `0` or `1`
Turn this on to automatically look back while in the reverse gear.

* `0`: No change
* `1`: Look back automatically

### `ThrottleStart` : `0` or `1`
Turn this on to be able to start the engine by pressing clutch + throttle,
like in some other games. This works alongside the usual button to start
the engine.

* `0`: Can only start engine with button
* `1`: Can start engine with button and clutch + throttle

### `UITips` : `0` or `1`
This is a simple gear display, which might be of help to determine if you’re
in Neutral or not. It also indicates if the gear you're in is the top gear with
the specified color.

* `0`: No indicator
* `1`: Indicator active

* `UITips_X`: `0` is left, `100` is right.
* `UITips_Y`: `0` is top, `100` is bottom.
* `UITips_Size`: Any numerical value.
* `UITips_OnlyNeutral`: `0`: always show. `1`: Show only when in neutral
* `UITips_TopGearC_R`: `0` to `255`: Red
* `UITips_TopGearC_G`: `0` to `255`: Green
* `UITips_TopGearC_B`: `0` to `255`: Blue

### `CrossScript` : `0` or `1`
Turn this off to disable communication (shift indicators and neutral gear) to
other mods. Leaving this on in a CitizenFX-based mod __crashes the game__.

* `0`: No mod info available for other mods
* `1`: Mod info available for other mods

## `[CONTROLLER]`
The controller can only be used for a sequential gearbox or automatic gearbox.
Upon having switched to this input, sequential shifting mode will automatically
engage if in H-Shifter mode.

The default settings are laid out so they conflict least with regular gameplay.
The controller assumes an Xbox 360 controller, the following buttons and
options are available.

    DpadUp
    DpadDown
    DpadLeft
    DpadRight
    Start
    Back
    LeftThumb
    RightThumb
    LeftShoulder
    RightShoulder
    A
    B
    X
    Y
    LeftTrigger
    RightTrigger
    LeftThumbLeft
    LeftThumbRight
    RightThumbLeft
    RightThumbRight
    LeftThumbUp
    LeftThumbDown
    RightThumbUp
    RightThumbDown

### `Toggle`
Hold this button to toggle Manual Transmission on or off

### `ToggleShift`
Hold this button to toggle between Automatic and Sequential Transmission

### `ToggleTime` : Any (milliseconds)
How long it takes for a button hold to register. Example, `ToggleTime = 500`
means you need to hold it half a second to trigger.

### `TriggerValue` : `0` to `100`
How many % the analog axis needs to be pressed in or pushed to, to register as
a button press.


### `ShiftUp` : Any button
### `ShiftDown` : Any button
### `Clutch` : Any button
Gearbox controls. Self-explanatory.

### `Engine` : Any button
Turn on or off engine.

### `ToggleEngine` : `0` or `1`
* `0`: On pressing `Engine`, engine only turns on when off
* `1`: On pressing `Engine`, engine can also turn off when on

### `Throttle` and `Brake`
You __need__ to correctly set these to get braking and a standstill and
reversing with the throttle to work.

## `[KEYBOARD]`
This section assumes a regular ANSI keyboard with the US/QWERTY layout.

Look up available keys in `Keys.txt`.

### `Toggle`
Key to toggle mod on or off.

### `ToggleH`
Key to toggle between shifting modes.

### `Throttle` and `Brake`
You __need__ to correctly set these to get braking and a standstill and
reversing with the throttle to work.

## `[DEBUG]`

### `Info` : `0` or `1`
* `0`: No debug info onscreen
* `1`: Debug info onscreen with input metrics and other data.

## `settings_wheel.ini`
This file contains all settings for the wheel controls. As this is fairly complex
to do manually, using the GUI is recommended.

DirectInput steering wheels are fully supported! Every axis, button and 8
directions on the D-pad are supported for inputs. Additionally, steering wheel
input has been built in even if you don’t want to drive with any gearbox and
just want the default behavior. __Force Feedback is fully present and active in
all modes.__

When assigning axes and buttons, __use DiUtil.exe__! This tool will report the
correct values for the .ini.

## `[OPTIONS]` (Wheel)

### `EnableWheel` : `0` or `1`
Enable detection and usage of a DirectInput wheel. Turn this on if you want to
use your racing wheel with GTA V and this mod.

### `WheelWithoutManual` : `0` or `1`
Enable usage of a wheel without using Manual Transmission features.

### `WheelBoatPlanes` : `0` or `1`
__EXPERIMENTAL__
Support for wheel input for boats and airplanes. Control boats like how you
 control cars. For plane, the layout is a bit different and requires a H-pattern
 shifter.
| Control | Effect |
|---------|--------|
| Throttle | Yaw right |
| Clutch | Yaw left |
| Wheel | Roll left / right |
| Shift up paddle | Pitch up |
| Shift down paddle | Pitch down |
| Gear 1| 33% power |
| Gear 3| 66% power |
| Gear 5| 100% power |
| Gear 2| 33% brake / reverse |
| Gear 4| 66% brake / reverse |
| Gear 6| 100% brake / reverse |

### `PatchSteering` : `0` or `1`
Patch steering correction. Credits to InfamousSabre's original
[CustomSteering](https://www.gta5-mods.com/scripts/custom-steering). This is
essential for 1:1 steering wheel and vehicle wheel mapping. Only works on anything
that isn't flying, a boat or some sort of bike.

Patching/unpatching happens automatically depending on input, so this option
can be safely left on if there's playing with controllers or keyboard and mouse.

### `PatchSteeringAlways` : `0` or `1`
Override the automatic unpatching, so the steering is still mapped 1:1 to your controller
or keyboard input. Recommended to leaving this off, for gameplay purposes.

## `[FORCE_FEEDBACK]`
### `Enable` : `0` or `1`
Disable or enable force feedback.

### `GlobalMult` : Any
Multiplier in percentage of how strong all forces are.
This feature is available from 4.2.0 on.

### `DamperMax` : `0` to `100`
Controls the friction feel when the vehicle is at a stop. A higher
value means more friction. Keep this higher than __DamperMin__.

### `DamperMin` : `0` to `100`
Controls the friction feel when the vehicle is moving. A higher
value means more friction. Keep this lower than __DamperMax__.

### `DamperTargetSpeed` : `0` to any (in m/s)
Sets the speed at which the damper effect is minimal. This is in
meters per second!

### `PhysicsStrength` : Any
How much physics affect your steering wheel and pulls it left or right. A higher 
value means a stronger force feedback.

### `DetailStrength` : `0` to any
How strong the feedback is from suspension compression. Think for terrain
details like road texture, potholes, manhole covers, sidewalk curbs etc.

## `[INPUT_DEVICES]`
To this, add your device GUIDs and a name so you can remember what is which.
An example entry looks like this:

```
[INPUT_DEVICES]
DEV0 = Logitech G27 Racing Wheel USB
GUID0 = {F69653F0-19B9-11E6-8002-444553540000}
```

This is taken from the data DiUtil.exe or Gears.asi generates in `ManualTransmission/DiUtil.log`
or `Gears.log`. A sample detection entry looks like this:

```
[23:20:19.989] Found 3 device(s)
[23:20:19.989] Device: Logitech G27 Racing Wheel USB
[23:20:19.989] GUID:   {F69653F0-19B9-11E6-8002-444553540000}
```

### Most controls

|Control|Usage|Effect|
|-------|-----|------|
|Toggle|Press|Toggle Manual Transmission on/off|
|ToggleH|Press|Switch between sequential, H-shifter or automatic|
|ShiftUp|Press|Shift up 1 gear (sequential/auto)|
|ShiftDown|Press|Shift down 1 gear (sequential/auto)|
|HN|Press/Hold|Shift into gear N (H-shifter)|
|Handbrake|Hold|Applies the hand brake|
|Horn|Hold|Sound the horn|
|LookBack|Hold|Look back|
|Engine|Press|Restart the engine or turn it off|
|Lights|Press|Switch between off, low beam and full beam|
|Camera|Press|Switch through cameras|
|RadioNext|Press|Next radio channel|
|RadioPrev|Press|Previous radio channel|
|IndicatorLeft|Press|Switch on/off left indicator|
|IndicatorRight|Press|Switch on/off right indicator|
|IndicatorHazard|Press|Switch on/off hazard lights|

Every single control can be assigned to any device. 

## `[STEER]`, `[THROTTLE]`, `[BRAKES]`, `[CLUTCH]` and `[HANDBRAKE_ANALOG]`
__To properly configure your wheel, use DiUtil.exe!__

These sections maps your wheel input axes.

### Supported input axes and ranges

	lX
	lY
	lZ
	lRx
	lRy
	lRz
	rglSlider0
	rglSlider1

* MIN – Value of axis while pedal is not pressed
* MAX – Value of axis while pedal is fully pressed

For ranges, values `0` to `65535` are usually reported.

Force feedback axis is locked to the steering axis.


### `SteerAngleMax` : Any
Physical steering wheel steering degrees, in angles. Match this with your wheel
spec.

### `SteerAngleCar` : Any less than `SteerAngleMax`
Soft lock for in cars.

### `SteerAngleBike` : Any less than `SteerAngleMax`
Soft lock for on bikes.

### `SteerAngleAlt` : Any less than `SteerAngleMax`
Soft lock for in planes and boats.

### Existing configurations
When users submit their configs, I will probably include these with future builds and here.

## `[TO_KEYBOARD]`
In this section you can assign wheel buttons to keyboard keys. A few examples
have been given. The format is `[BUTTON] = [KEY]`. Up to 128 buttons
are supported. Any keyboard key can be chosen, but Num Lock needs to be OFF for
keys to be interpreted correctly.
Use the included __Keys.txt__ for reference!

Only one device can be used for this feature.

Examples:

* `7 = H` makes `Button 7` act as the `H` key, which turns on the headlights.
* `20 = E` makes `Button 20` act as the `E` key, which is the horn or emergency lights.
* `18 = X` makes `Button 18` act as the `X` key. If [Slam It](https://www.gta5-mods.com/scripts/slam-it)
is installed, it'll lower the car.
* `16 = LEFT` makes `Button 16` act as the `LEFT` key. If [Windscreen Wipers](https://www.gta5-mods.com/scripts/car-wipers)
is installed and a compatible car is used, the wipers are turned on.


# Troubleshooting
## Installation dependencies
This mod should work on a bare system with only GTA V, build 350 to 877.1 and
the necessary programs to run GTA V and ScriptHookV supporting that version of
GTA V.

## Non-conflicting software
The mod has been tested with GTA V version 350, 617, 678, 791.2 and 877.1 with:
* ScriptHookV
* ScriptHookVDotNet
* RAGEPluginHook
* ENB Series
* OpenIV
* FoV 1.33

The mod runs with these other mods without any incompatibilities. Switching off
 Cross Script communications also makes it compatible with
 CitizenFX-based mods like FiveReborn.

## Conflicting software
* x360ce will conflict with input detection if throttle, brake or steering clutch
are mapped, but the mod should still register your wheel. Assigning inputs
without overlap is no problem.
* Forward/Reverse conflicts with
[Strapped](https://www.gta5-mods.com/scripts/pull-out-strap). Remove Strapped if
you experience input conflicts.
* Steering patching conflicts with [CustomSteering](https://www.gta5-mods.com/scripts/custom-steering).
Remove CustomSteering if `PatchSteering` is enabled.

## Steering wheel reports strange values
Check if your wheel is recognized correctly, a recent Windows 10 update forces
new Logitech software which will mess up older Logitech steering wheels.

## Steering wheel not detected
* Try toggling the mod (|\ key)
* Ensure you have removed xinput dlls from the GTA V directory
* Check back with DiUtil.exe to see if that thing does see your wheel

# Thanks
* Rockstar Games
* Alexander Blade
* LeFix
* XMOD
* InfamousSabre
* leftas
* kagikn
* aXurez

# Source code
This mod is fully open source. The source code is available at
https://github.com/E66666666/GTAVManualTransmission
