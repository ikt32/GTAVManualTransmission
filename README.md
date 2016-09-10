GTA V Manual Transmission
=========================
[![Build status](https://ci.appveyor.com/api/projects/status/gy6yh17lp5l1k48d?svg=true)](https://ci.appveyor.com/project/E66666666/gtavmanualtransmission)

This is an ASI script allowing for manual control over the ingame gearbox. As a secondary function, this mod has full racing wheel support. Supported features are steering, the throttle, brake and throttle pedals, sequential shifting and using the H-shifter.

Discuss the mod over at [GTAForums.com](http://gtaforums.com/topic/840830-manual-transmission/).


## Requirements
* Grand Theft Auto V
* [ScriptHookV by Alexander Blade](http://www.dev-c.com/gtav/scripthookv/)

## Downloads

* [GTA5-Mods.com](https://www.gta5-mods.com/scripts/manual-transmission-ikt)
* [Releases page](https://github.com/E66666666/GTAVManualTransmission/releases)
* [AppVeyor build fragments](https://ci.appveyor.com/project/E66666666/gtavmanualtransmission)

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
* "hunt_score" 0 - Nothing
* "hunt_score" 1 - Shift up
* "hunt_score" 2 - Shift down

Scripts changing torque
* "hunt_score" 2 - Torque is decreased 

Speedometers Neutral gear:
* "hunt_weapon" 0 - In gear
* "hunt_weapon" 1 - Neutral

Please note that the neutral gear is the gearbox simulated neutral gear, which isn't affected by the hand brake!

__Important note:__ Usage of decorators out of base game context isn't guaranteed to work. Think CitizenFX-based mods. The reason behind this is that we're just hijacking some existing base game scripts and hope nothing conflicts.

