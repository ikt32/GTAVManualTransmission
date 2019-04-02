[comment]: # (GitHub README.md)

GTA V Manual Transmission
=========================
[![Build status](https://ci.appveyor.com/api/projects/status/gy6yh17lp5l1k48d?svg=true)](https://ci.appveyor.com/project/E66666666/gtavmanualtransmission) [![GitHub total downloads](https://img.shields.io/github/downloads/E66666666/GTAVManualTransmission/total.svg)](https://github.com/E66666666/GTAVManualTransmission/releases) [![Discord](https://img.shields.io/discord/499483293679353861.svg)](https://discord.gg/gHee23U)

This mod for Grand Theft Auto V enables manual transmission and offers various gearbox modes:
  * Sequential 
  * H-pattern
  * Custom automatic
  
Along with clutch simulation, engine effects and realistic throttle/brake input schemes.

Additionally, this mod offers racing wheel support with the following features: 
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

## Scripting API  

Decorators are now deprecated and will be removed in the first release after v4.6.6.

Check [ManualTransmission.h](https://github.com/E66666666/GTAVManualTransmission/blob/master/Gears/ManualTransmission.h) for available API functions.

A C# example on how to use it can be found in [this gist](https://gist.github.com/E66666666/d11cdbd9800ad73efeff612374349347).

## Bug reporting and support

Bugs can be reported in the following channels:

* [Discord server](https://discord.gg/gHee23U)
* [5mods comments](https://www.gta5-mods.com/scripts/manual-transmission-ikt#comments_tab)
* [GitHub issues](https://github.com/E66666666/GTAVManualTransmission/issues/new)

Please always include `Gears.log`. If additional hardware, like a steering wheel is involved, also include those details in the report.
