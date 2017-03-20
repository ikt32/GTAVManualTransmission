[comment]: # (GitHub README.md)

GTA V Manual Transmission
=========================
[![Build status](https://ci.appveyor.com/api/projects/status/gy6yh17lp5l1k48d?svg=true)](https://ci.appveyor.com/project/E66666666/gtavmanualtransmission)

This is an ASI script allowing for manual control over the ingame gearbox. As a secondary function, this mod has full racing wheel support. Supported features are steering, the throttle, brake and throttle pedals, sequential shifting and using the H-shifter.

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

Download the [ScriptHookV SDK](http://www.dev-c.com/gtav/scripthookv/) and extract it's contents to ScriptHookV_SDK.
Clone this repository to the same folder ScriptHookV_SDK was extracted so you have ScriptHookV_SDK and GTAVManualTransmission in the same folder. If you get build errors about missing functions, update your [natives.h](http://www.dev-c.com/nativedb/natives.h). Check AppVeyor build logs for the natives.h I use.

## Mod support
You can read decorators to get some info about this mod.

Example: ```DECORATOR::DECOR_GET_INT(vehicle, "hunt_score");``` gets the current shift up/down status. Currently this mod exposes 2 variables which can be used in other scripts.

Current gear: `doe_elk`
* `0`: Reverse
* `1 through 7`: Matching gear

Speedometers shift up/down indicator: `hunt_score`
* `0` - Nothing
* `1` - Shift up
* `2` - Shift down

Speedometers Neutral gear:
* `hunt_weapon`: `0` - Not in neutral
* `hunt_weapon`: `1` - In neutral

Please note that the neutral gear is fake - it's achieved by having the clutch disengaged fully.

Since version `4.0.2` it's possible to set shifting mode externally.

* `hunt_chal_weapon`: `0` - No change
* `hunt_chal_weapon`: `1` - Sequential
* `hunt_chal_weapon`: `2` - H-pattern
* `hunt_chal_weapon`: `3` - Automatic

### FiveM Update
From version 4.2.0 beta 3 onwards, FiveM is supported! An [update](https://forum.fivem.net/t/fivem-update-march-19th-2017/8703) fixed some issues
this mod had with FiveM. On top of that, beta 3 has more robust patching for GTA V's 
stability control.
