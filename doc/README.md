[comment]: <> (User README.md)
Manual Transmission & Steering Wheel Support
===========

# Manual Transmission for GTA V
This mod will enable manual transmission for vehicles, using the games' real
gear box! This means real gears - not speed capping. There are plenty of
features to emulate how real transmissions work.

# Steering Wheel Support for GTA V
Additionally, this mod has full support for steering wheels since version 4.0!
All steering wheels properly compatible with DirectInput should work. Force
feedback is fully supported. Steering wheel support is always turned on, so it
also works without manual transmission!

# Table of Contents
<!-- TOC depthFrom:1 depthTo:2 withLinks:1 updateOnSave:1 orderedList:0 -->

- [Manual Transmission for GTA V](#manual-transmission-for-gta-v)
- [Steering Wheel Support for GTA V](#steering-wheel-support-for-gta-v)
- [Table of Contents](#table-of-contents)
- [Requirements](#requirements)
- [Installation](#installation)
	- [Additional steps for wheel users](#additional-steps-for-wheel-users)
- [Basic usage and controls](#basic-usage-and-controls)
	- [Keyboard:](#keyboard)
	- [Controller:](#controller)
	- [Wheel:](#wheel)
	- [Usage](#usage)
- [Troubleshooting](#troubleshooting)
	- [Installation dependencies](#installation-dependencies)
	- [Non-conflicting software](#non-conflicting-software)
	- [Conflicting software](#conflicting-software)
	- [Steering wheel reports strange values](#steering-wheel-reports-strange-values)
	- [Steering wheel not detected](#steering-wheel-not-detected)
- [Configuration](#configuration)
	- [`[OPTIONS]`](#options)
	- [`[CONTROLLER]`](#controller)
	- [`[KEYBOARD]`](#keyboard)
	- [`[WHEEL]`](#wheel)
- [Thanks](#thanks)
- [Source](#source)

<!-- /TOC -->

# Requirements
* Grand Theft Auto V
* [ScriptHookV](http://www.dev-c.com/gtav/scripthookv/)

# Installation
1.	Put `Gears.asi` and `Gears.ini` in your GTA V folder. Overwrite if asked.
2.	Read `CONFIGURATION.txt` and configure `Gears.ini` to your preferences.

## Additional steps for wheel users
1.	Remove or disable any XInput or DirectInput DLL files for your wheel for
GTA V. For example, disable or remove x360ce.
2.	Read `STEERINGCONFIG.txt` and configure your wheel. Pay special attention to
the `[WHEELAXIS]` section.
3.	Use `Joystick.exe` to find out which input is mapped to which axis and the
values for the positions.

### Recommended mods
* [Custom Steering](https://www.gta5-mods.com/scripts/custom-steering)
* [NFS Speedo](https://www.gta5-mods.com/scripts/nfsgauge-rpm-gear-speedometer)


When reloading the mod by toggling it off and on, the settings are read again
and the steering wheel (if connected) is reset again. You can use this to fine-
tune your settings and change things on-the-fly, without restarting the game.

# Basic usage and controls
__Take a look inside Gears.ini__, as most sections have a small explanation. You
WILL need to configure it correctly, otherwise some features will not work!
The buttons listed below are default controls. You can change these at will.

Basic knowledge of a manually operated vehicle is required, such as what a
clutch is and what it does. You can play with these things disabled too, for
a more arcade gameplay.

## Keyboard:
* Press `|\` to disable or enable manual transmission.
* Press `}]` to switch between a sequential gearbox, H-pattern gearbox or the
automatic gearbox.

## Controller:
* Hold Dpad Right to disable or enable manual transmission.
* Hold ???? ????? to switch between a sequential gearbox, H-pattern gearbox or
the automatic gearbox.

## Wheel:
* Use the assigned buttons to toggle the mod and switch between gearboxes.
* Refer to `Gears.ini` for these buttons.

## Usage
To switch between inputs, you only need to tap the throttle, the mod will
automatically switch between these inputs.

Specifically for wheel users, you  might need to fully depress the throttle
pedal (once) if the mod keeps swapping away from the keyboard/controller.

# Troubleshooting
## Installation dependencies
This mod should work on a bare system with only GTA V, build 350 to 877.1 and
the necessary programs to run GTA V and ScriptHookV supporting that version of
GTA V.

## Non-conflicting software
The mod has been tested with GTA V version 350, 617, 678, 791.2 with
ScriptHookV, ScriptHookVDotNet, RAGEPluginHook, ENB Series, OpenIV and FoV 1.33
without any incompatibilities. Switching off Cross Script communications also
makes it compatible with CitizenFX-based mods like FiveReborn.

## Conflicting software
x360ce will conflict with input detection if throttle, brake or steering clutch
are mapped, but the mod should still register your wheel. Assigning inputs
without overlap is no problem.

## Steering wheel reports strange values
Check if your wheel is recognized correctly, a recent Windows 10 update forces
new Logitech software which will mess up older Logitech steering wheels.

## Steering wheel not detected
* Try toggling the mod (|\ key).
* Ensure you have removed xinput dlls from the GTA V directory.

# Configuration
This guide will explain the usage of Gears.ini and what the options mean. It’s
highly recommended to play with this mod using a controller or a wheel.

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

### `ClutchShifting` : `0` or `1`
This option controls the requirement to hold the clutch for H-shifting.

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

### `HillBrakeWorkaround`: `0` or `1`
Turn this on to emulate a hill start and car roll on a hill. It gives your car
a little push. Idea and implementation by XMOD.

* `0`: No change
* `1`: Workaround enabled. A force will push the car down a slope.

### `AutoLookBack`: `0` or `1`
Turn this on to automatically look back while in the reverse gear.

* `0`: No change
* `1`: Look back automatically

### `UITips`: `0` or `1`
This is a simple gear display, which might be of help to determine if you’re
in Neutral or not. It also indicates if the gear you're in is the top gear with
the specified color.

* `UITips_X`: `0` is left, `100` is right.
* `UITips_Y`: `0` is top, `100` is bottom.
* `UITips_Size`: Any numerical value.
* `UITips_OnlyNeutral`: `0`: always show. `1`: Show only when in neutral
* `UITips_TopGearC_R`: `0` to `255`: Red
* `UITips_TopGearC_G`: `0` to `255`: Green
* `UITips_TopGearC_B`: `0` to `255`: Blue

### `CrossScript`: `0` or `1`
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

### `Throttle` and `Brake`
You __need__ to correctly set these to get braking and a standstill and
reversing with the throttle to work.

## `[KEYBOARD]`
This section assumes a regular ANSI keyboard with the US/QWERTY layout.

Look up available keys in `Keys.txt`.

### `Throttle` and `Brake`
You __need__ to correctly set these to get braking and a standstill and
reversing with the throttle to work.

## `[WHEEL]`


























# Thanks
* Alexander Blade
* LeFix
* XMOD
* InfamousSabre
* leftas
* kagikn
* aXurez

# Source
This mod is fully open source. The source code is available at
https://github.com/E66666666/GTAVManualTransmission
