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
* [Releases page](https://github.com/E66666666/GTAVManualTransmission/releases)
* [AppVeyor build artifacts](https://ci.appveyor.com/project/E66666666/gtavmanualtransmission/build/artifacts)

## Building

### Requirements
* [ScriptHookV SDK by Alexander Blade](http://www.dev-c.com/gtav/scripthookv/)
* [DirectX SDK (June 2010)](https://www.microsoft.com/en-us/download/details.aspx?id=6812) for XInput 1.3 and DirectInput

Download the [ScriptHookV SDK](http://www.dev-c.com/gtav/scripthookv/) and extract it's contents to ScriptHookV_SDK.
Clone this repository to the same folder ScriptHookV_SDK was extracted so you have ScriptHookV_SDK and GTAVManualTransmission in the same folder. If you get build errors about missing functions, update your [natives.h](http://www.dev-c.com/nativedb/natives.h).

## Mod support
You can read decorators to get some info about this mod.

Example: ```DECORATOR::DECOR_GET_INT(vehicle, "hunt_score");``` gets the current shift up/down status. Currently this mod exposes 2 variables which can be used in other scripts.

Speedometers shift up/down indicator:
* `hunt_score`: `0` - Nothing
* `hunt_score`: `1` - Shift up
* `hunt_score`: `2` - Shift down

Scripts changing torque
* `hunt_score`: `2` - Torque is decreased

Speedometers Neutral gear:
* `hunt_weapon`: `0` - In gear
* `hunt_weapon`: `1` - Neutral

Please note that the neutral gear is a simulated neutral gear, which is clutch in gear 1. Take this in consideration for your speedometer implementation.

Since version `4.0.2` it's possible to set shifting mode externally.

* `hunt_chal_weapon`: `0` - No change
* `hunt_chal_weapon`: `1` - Sequential
* `hunt_chal_weapon`: `2` - H-pattern
* `hunt_chal_weapon`: `3` - Automatic

## FiveM Notes
Usage of decorators out of base game context isn't guaranteed to work. CitizenFX-based mods will crash. Cross-script communication can be simply disabled using the ```CrossScript = 0``` in ```Gears.ini``` for these situations.

### FiveM Update
From version 4.2.0 onwards, using a steering wheel with FiveM is not supported. Due to the future [change to the ScriptHookV compatibility within FiveM](https://forum.fivem.net/t/opening-reading-crash-dumps/7695/2), all support will be dropped for FiveM, until further details are clear.
