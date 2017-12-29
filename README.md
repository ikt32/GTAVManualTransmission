[comment]: # (GitHub README.md)

GTA V Manual Transmission
=========================
[![Build status](https://ci.appveyor.com/api/projects/status/gy6yh17lp5l1k48d?svg=true)](https://ci.appveyor.com/project/E66666666/gtavmanualtransmission) [![GitHub total downloads](https://img.shields.io/github/downloads/E66666666/GTAVManualTransmission/total.svg)](https://github.com/E66666666/GTAVManualTransmission/releases)

This is an ASI script allowing for manual control over the ingame gearbox with various control options:
  * Sequential 
  * H-pattern
  * Custom automatic
Along with clutch simulation, engine effects and realistic throttle/brake input schemes.

As a secondary function, this mod has full (DirectInput) racing wheel support: 
  * Direct steering
  * Proper force feedback
  * Analog throttle, brake and clutch pedals
  * Sequential shifter and H-pattern shifter support
  * Many more assignable controls
  * Multiple devices supported
  
Other features:
  * Seamless switching between keyboard, controller and steering wheel
  * Extremely customizable experience
  * User friendly configuration menu
  * Various HUD elements for vehicle info and input info

Discuss the mod over at [GTAForums.com](http://gtaforums.com/topic/840830-manual-transmission/) or [GTA5-Mods.com](https://forums.gta5-mods.com/topic/1840/script-wip-manual-transmission-steering-wheel-support-4-0).


## Requirements
* Grand Theft Auto V
* [ScriptHookV by Alexander Blade](http://www.dev-c.com/gtav/scripthookv/)

## Downloads

* [GTA5-Mods.com](https://www.gta5-mods.com/scripts/manual-transmission-ikt)
* [GitHub releases](https://github.com/E66666666/GTAVManualTransmission/releases)
* [AppVeyor builds](https://ci.appveyor.com/project/E66666666/gtavmanualtransmission/build/artifacts)

## Building

### Requirements
* [ScriptHookV SDK by Alexander Blade](http://www.dev-c.com/gtav/scripthookv/)
* [DirectX SDK (June 2010)](https://www.microsoft.com/en-us/download/details.aspx?id=6812) for XInput 1.3 and DirectInput
* [GTAVMenuBase](https://github.com/E66666666/GTAVMenuBase)

Download the [ScriptHookV SDK](http://www.dev-c.com/gtav/scripthookv/) and extract its contents to ScriptHookV_SDK.

Clone this repository to the same folder ScriptHookV_SDK was extracted so you have ScriptHookV_SDK and GTAVManualTransmission in the same folder. If you get build errors about missing functions, update your [natives.h](http://www.dev-c.com/nativedb/natives.h). Check AppVeyor build logs for the natives.h I use.

Clone my [GTAVMenuBase](https://github.com/E66666666/GTAVMenuBase) to the same folder you're gonna clone this to.

## Mod support  

You can read decorators to get some info about this mod.

Example: ```DECORATOR::DECOR_GET_INT(vehicle, "mt_shift_indicator");``` gets the current shift up/down status. Currently this mod exposes 2 variables which can be used in other scripts.

Current gear: `mt_gear`
* `0`: Reverse
* `1 through 7`: Matching gear

Speedometers shift up/down indicator: `mt_shift_indicator`
* `0` - Nothing
* `1` - Shift up
* `2` - Shift down

Speedometers Neutral gear: `mt_neutral`
* `0` - In gear
* `1` - Neutral

Please note that the neutral gear is fake - it's achieved by having the clutch disengaged fully.

Set shift mode: `mt_set_shiftmode`
* `0` - No change
* `1` - Sequential
* `2` - H-pattern
* `3` - Automatic
