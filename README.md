[comment]: # (GitHub README.md)

# Manual Transmission and Wheel Support for Grand Theft Auto V

[![Build status](https://ci.appveyor.com/api/projects/status/gy6yh17lp5l1k48d?svg=true)](https://ci.appveyor.com/project/E66666666/gtavmanualtransmission) [![GitHub total downloads](https://img.shields.io/github/downloads/E66666666/GTAVManualTransmission/total.svg?label=downloads&logo=GitHub)](https://github.com/E66666666/GTAVManualTransmission/releases) [![Discord](https://img.shields.io/discord/848493320433827851.svg?logo=discord)](https://discord.gg/VrrAEV4j4b)

This project aims to expand the driving immersion of Grand Theft Auto V, with many features and options:

* Complete steering wheel support
  * DirectInput interfacing
  * Force feedback from scratch
  * Multiple devices supported
* Transmission replacement with custom modes and more
  * Manual sequential
  * Manual H-pattern
  * Automatic
  * Working clutch
* Tunable driving assists
  * Launch control
  * Traction control
  * Stability control
  * Custom anti-lock braking
* Synchronized steering wheel and animations
  * Match your actual wheel 1:1
  * First person hand-over-hand animations
* Custom first person camera
  * With inertia effects

And much more. The [user README.md](doc/README.md) has a more complete overview.

## Game requirements

* Grand Theft Auto V (build 1604+)
* [ScriptHookV by Alexander Blade](http://www.dev-c.com/gtav/scripthookv/)

## Downloads

* [GTA5-Mods.com](https://www.gta5-mods.com/scripts/manual-transmission-ikt)
* [GitHub releases](https://github.com/E66666666/GTAVManualTransmission/releases)
* [AppVeyor builds](https://ci.appveyor.com/project/E66666666/gtavmanualtransmission/build/artifacts)

## Building requirements

* Visual Studio 2022
* [DirectX SDK (June 2010)](https://www.microsoft.com/en-us/download/details.aspx?id=6812) for XInput 1.3 ([see doc](doc/notes.md))

The solution builds two files:

* `Gears.asi`, the actual script. This goes in Grand Theft Auto V's root folder.
* `WheelSetup.exe`, which is a companion program for debugging wheel inputs. Can also write configurations.

## Scripting API  

Some convenience functions are exposed by the script.

Check [ManualTransmission.h](https://github.com/E66666666/GTAVManualTransmission/blob/master/Gears/ManualTransmission.h) for available API functions.

* [C# Example](https://gist.github.com/E66666666/d11cdbd9800ad73efeff612374349347)
* [C++ Example](https://gist.github.com/E66666666/59390733b366cad4638901ae5fcfd046)

If you have any questions about using these, don't hesitate to ask on one of the channels below.

## Bug reporting and support

Please report bugs in one of the following locations - most preferred platform first:

* [GitHub issues](https://github.com/E66666666/GTAVManualTransmission/issues/new)
* [Discord server](https://discord.gg/VrrAEV4j4b)
* [5mods comments](https://www.gta5-mods.com/scripts/manual-transmission-ikt#comments_tab)

Always include `Gears.log`. If additional hardware, like a steering wheel is involved, also include those details in the report.
