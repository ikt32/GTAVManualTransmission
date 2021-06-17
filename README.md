[comment]: # (GitHub README.md)

GTA V Manual Transmission
=========================
[![Build status](https://ci.appveyor.com/api/projects/status/gy6yh17lp5l1k48d?svg=true)](https://ci.appveyor.com/project/E66666666/gtavmanualtransmission) [![GitHub total downloads](https://img.shields.io/github/downloads/E66666666/GTAVManualTransmission/total.svg?label=downloads&logo=GitHub)](https://github.com/E66666666/GTAVManualTransmission/releases) [![Discord](https://img.shields.io/discord/848493320433827851.svg?logo=discord)](https://discord.gg/VrrAEV4j4b)

This mod for Grand Theft Auto V enables manual transmission and offers various gearbox modes:
  * Sequential 
  * H-pattern
  * Custom automatic
  
Along with clutch simulation, engine effects, realistic throttle/brake input schemes and driving assists.

Additionally, this mod offers racing wheel support with the following features: 
  * Analog throttle, brake and clutch pedals
  * Proper force feedback with LUT support
  * 1:1 steering
  * Support for sequential shifter and H-pattern shifters
  * Support for multiple different devices for each input
  * Many more assignable controls
  
Other features:
  * Seamless switching between keyboard, controller and steering wheel
  * Extremely customizable experience
  * User friendly in-game configuration menu
  * Various HUD elements for vehicle info and input info

Check [the user readme](doc/README.md) for more information.

## Requirements
* Grand Theft Auto V (build 1604+)
* [ScriptHookV by Alexander Blade](http://www.dev-c.com/gtav/scripthookv/)

## Downloads

* [GTA5-Mods.com](https://www.gta5-mods.com/scripts/manual-transmission-ikt)
* [GitHub releases](https://github.com/E66666666/GTAVManualTransmission/releases)
* [AppVeyor builds](https://ci.appveyor.com/project/E66666666/gtavmanualtransmission/build/artifacts)

## Building requirements

* VS2019 16.9+
* [DirectX SDK (June 2010)](https://www.microsoft.com/en-us/download/details.aspx?id=6812) for XInput 1.3 ([see doc](doc/notes.md))

Remember to (recursively) init/update your submodules after cloning!

## Scripting API  

Some convenience functions are exposed by the script.

Check [ManualTransmission.h](https://github.com/E66666666/GTAVManualTransmission/blob/master/Gears/ManualTransmission.h) for available API functions.

* [C# Example](https://gist.github.com/E66666666/d11cdbd9800ad73efeff612374349347)
* [C++ Example](https://gist.github.com/E66666666/59390733b366cad4638901ae5fcfd046)

If you have any questions, don't hesitate to ask me on one of the channels below.

## Bug reporting and support

Please report bugs in one of the following locations - most preferred platform first:

* [GitHub issues](https://github.com/E66666666/GTAVManualTransmission/issues/new)
* [Discord server](https://discord.gg/VrrAEV4j4b)
* [5mods comments](https://www.gta5-mods.com/scripts/manual-transmission-ikt#comments_tab)

Always include `Gears.log`. If additional hardware, like a steering wheel is involved, also include those details in the report.
