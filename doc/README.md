[comment]: # (User README.md)

# Manual Transmission and Steering Wheel Support for GTA V

Version 5.4.0

![5mods Thumbnail](MTThumb.jpg)

## Description

This mod adds manual transmission support, with loads of customization options
for the drivetrain and handling.

Fully supports steering wheels: It adds force feedback, works with about all
driving hardware, and every imaginable setting is customizable to fit
your wheel and driving style.

## Features

* Steering wheel, gamepad and keyboard support
* Seamless input switching
* Easy in-game configuration menu, with many options
* Per-vehicle settings
* Sequential, H-pattern and custom automatic transmissions
* Engine and transmission mechanics:
  * Working clutch
  * Engine braking, engine damage, stalling
  * Engine damage
  * Engine stalling
* Customizable steering assists
* Custom realistic Stability Control, Traction Control, Anti-Lock Brakes
* Supports UDP telemetry (DiRT 4 format) for SimHub and similar tools
* Syncronized steering animations

## Downloads

* [GTA5-Mods.com](https://www.gta5-mods.com/scripts/manual-transmission-ikt)
* [GitHub release (older versions)](https://github.com/E66666666/GTAVManualTransmission/releases)
* [Latest automated builds](https://ci.appveyor.com/project/E66666666/gtavmanualtransmission/build/artifacts)

### Recommended mods

For gameplay and driving:

* A realistic handling mod
* [Custom Gear Ratios](https://www.gta5-mods.com/scripts/custom-gear-ratios): Essential if you have cars with more than 6 gears, and allows matching gear ratios with the real car counterparts.
* [Turbo Fix](https://www.gta5-mods.com/scripts/turbo-fix): Fixes spool rates of the turbo upgrade.
* [Dial Accuracy Fix](https://www.gta5-mods.com/scripts/dial-accuracy-fix): Remap dashboard dials to match your actual speed.
* [ACSPatch](https://www.gta5-mods.com/scripts/auto-center-steering-patch-temp-fix): Keep wheels turned when exiting cars.
* [Autosport Racing System by Eddlm](https://www.gta5-mods.com/scripts/autosport-racing-system): Complete custom racing system with advanced AI.

Any speedometer supporting RPM/Gear reading from memory:

* [NFS Speedo](https://www.gta5-mods.com/scripts/nfsgauge-rpm-gear-speedometer)
* [LeFix Speedometer](https://www.gta5-mods.com/scripts/speedometer-improvedalexbladeversion)
* [NFSU Speedometer](https://www.gta5-mods.com/scripts/need-for-speed-underground-speedometer)
* Any ScriptHookVDotNet-based speedometer with gears and RPM

Mods that counter the power loss when sliding sideways (Also partially mitigated by LSD):

* [InversePower](https://www.gta5-mods.com/scripts/inversepower)
* [Drift Assist](https://www.gta5-mods.com/scripts/drift-assist)
* [True Realistic Driving V](https://www.gta5-mods.com/scripts/true-realistic-driving-v-realistic-mass-v0-1-beta): Script-based physics
* [Stop!Powercutting](https://www.gta5-mods.com/scripts/stop-powercutting): InversePower alternative
* [InverseTorque](https://www.gta5-mods.com/scripts/inversetorque): InversePower alternative

Recommended handling mods

The default grip levels cause the wheel to bounce left and right because they're too grippy.
These handlings have reduced grip to realistic levels, and are essential for playing with a wheel. Mix and match all you need, as these don't overlap much.

* [Realistic Driving V](https://www.gta5-mods.com/vehicles/realistic-driving-v) by killatomate
* [Aquaphobic's Realistic Handling Pack](https://www.gta5-mods.com/vehicles/realistic-handling-packs)
* [Lore Friendly Handling Pack](https://www.gta5-mods.com/vehicles/lore-friendly-handling-pack) by Eddlm
* [Realish Handling Pack](https://www.gta5-mods.com/vehicles/realish-handling-pack) by Eddlm

## Table of Contents

* [Manual Transmission and Steering Wheel Support for GTA V](#manual-transmission-and-steering-wheel-support-for-gta-v)
  * [Description](#description)
  * [Features](#features)
  * [Downloads](#downloads)
    * [Recommended mods](#recommended-mods)
  * [Table of Contents](#table-of-contents)
  * [Requirements](#requirements)
  * [Installation](#installation)
    * [Wheel setup](#wheel-setup)
    * [FiveM](#fivem)
    * [Updating](#updating)
  * [Default controls](#default-controls)
    * [Keyboard defaults (US-ANSI)](#keyboard-defaults-us-ansi)
    * [Controller defaults](#controller-defaults)
    * [Wheel defaults](#wheel-defaults)
  * [Usage and setup](#usage-and-setup)
    * [Driving basics](#driving-basics)
    * [Input switching](#input-switching)
    * [Vehicle Configurations](#vehicle-configurations)
    * [Driving assists](#driving-assists)
    * [Custom camera](#custom-camera)
    * [Animations](#animations)
    * [Wheel FFB LUT](#wheel-ffb-lut)
  * [Troubleshooting](#troubleshooting)
    * [Game compatibility](#game-compatibility)
    * [Compatibility options](#compatibility-options)
    * [Known issues](#known-issues)
    * [Steering wheel issues](#steering-wheel-issues)
  * [Credits](#credits)
  * [Source code](#source-code)
  * [Contact](#contact)

## Requirements

* [Grand Theft Auto V](https://www.rockstargames.com/V/)
* [ScriptHookV](http://www.dev-c.com/gtav/scripthookv/)

Optional: (Downloads)

* [DashHook](https://www.gta5-mods.com/tools/dashhook)
* [Handling Replacement](https://www.gta5-mods.com/tools/handling-replacement-library)
* [CamxxCore's DismembermentASI](https://www.gta5-mods.com/scripts/dismemberment)

## Installation

Put `Gears.asi` and the folder `ManualTransmission` in your GTA V folder
 (overwrite when asked).

__Make sure the folder is writeable! (not `Read Only`)__

Open the menu using the `mtmenu` cheat or the `\|` hotkey, and start
customizing things.

### Wheel setup

1. Remove or disable any XInput or DirectInput input hook configurations for
your wheel for GTA V (x360ce, for example).
2. Open the menu, navigate to `Controls`, `Wheel & pedals`.
3. Set up your analog inputs in `Analog input setup` and set up your analog inputs (throttle, brakes, steering).
4. Go back to the `Wheel & pedals` menu and go through __all__ options. __Read the description of each option.__

### FiveM

FiveM is not supported. You can try to use it as user plugin, but don't expect any support.

1. Create a `plugins` folder in FiveM Application Data.
2. Put `Gears.asi` and the folder `ManualTransmission` in `plugins`.

You can also just copy-paste the `ManualTransmission` folder if you have
configured the mod for singleplayer already.

The script works in servers that allow user plugins (ScriptHookV scripts). Last
checked to work with MT v5.0.0 and FiveM using the 1604 version of the game.

No plans exist to port this into FiveM or "convert" it to server-script, but
if you have solid plans to do so, feel free to contact me if you have questions.

A FiveM resource research project seems to work well,
[so check that out instead.](https://forum.cfx.re/t/discussion-research-manual-transmission-resource/146444)

[LegacyFuel](https://forum.cfx.re/t/prerelease-trew-hud-ui-legacyfuel-advanced-fuel-usage-efficiency-displays-advanced-cruise-control/1433922)
implements the code discussed above.

### Updating

Replace `Gears.asi` and copy the `ManualTransmission` folder. You do **not**
need to overwrite changes in the `ManualTransmission` folder, the script will
write new settings in the file when saving.

## Default controls

Refer to `settings_controls.ini` for the default controls.

Opening the menu:

* Press `[{` to access the menu or
* Enter the `mtmenu` cheat or
* Press `RB` + `B` on your controller.

These shortcuts can be changed in `settings_menu.ini`.

### Keyboard defaults (US-ANSI)

By default, `W` is throttle and `S` is brake.

* Press `\|` to disable or enable manual transmission
* Press `]}` to switch between sequential, H-pattern or automatic
* Press `Z` for Clutch
* Press `X` for Engine

Sequential and Automatic:

* Press `LSHIFT` to shift up
* Press `LCTRL` to shift down

### Controller defaults

By default, `RightTrigger` is throttle and `LeftTrigger` is brake.

* Hold `B` to switch between sequential or automatic
* Press `A` to shift up
* Press `X` to shift down
* Use `LeftThumbUp` to control the clutch
* Press `DpadDown` for Engine

### Wheel defaults

**There are no defaults.** Use the menu to assign your controls.

## Usage and setup

After installation use the menu key, button(s) or cheat to open the Manual
Transmission menu. You will need this menu to change all the options of the
script and set up things like steering wheels or custom controls.

### Driving basics

Manual Transmission simulates a real car, so you might want to know how to
drive a manual.

__Using the clutch:__  Depending on your settings, you might need to operate
the clutch to drive your car.

When the stalling option is enabled, remember to not let the RPM dip too low.
It might stall otherwise. Stalling can be noticed by the RPM bar dropping below
the stationary RPM.

When using a H-pattern shifter, remember to clutch in to shift. Not pressing
the clutch might cause a misshift, which might damage the car, and the car
will not go in gear. You'll hear a grinding sound when this happens.

When timed right, it's possible to shift into gear without clutching, when the
speed of the car and the RPM match up.

__Braking and reversing:__ While Manual Transmission is active, the brake input will only
work as a brake. When stopped, the brake input will not reverse your car.

To reverse, shift into the reverse gear. Press the accelerator
input to accelerate in reverse.

__Wheel-specific:__ While Manual Transmission is active, the pedals behave
like real pedals. When the manual transmission part of the mod is turned off,
the throttle and the brake work like the left or right triggers on a controller.

### Input switching

The mod picks up the last control and is only active for that set of controls.
To switch between inputs (keyboard, controller or wheel), you only need to tap
the throttle on that device. The mod switches between these inputs by itself,
and the main menu shows what the current active input is.

Specifically for wheel users, you might need to fully depress the throttle
pedal or clutch pedal (once) if the mod keeps swapping away from the
 keyboard or controller.

If for some reason you want to lock the controls, head over to `Debug` and check
`Disable input detection`. This allows switching inputs manually in the main menu.

### Vehicle Configurations

The script supports various vehicle-specific options, such as shifting
behavior and driving assists. The submenu `Manual Transmission settings` ->
`Vehicle configurations` shows the current known configurations. When you're
in a vehicle that fits the model and/or plate, that configuration is loaded.

With the option `Create configuration...`, a new, clean configuration is
generated and activated. Some submenu subtitles show `CFG: [<Configuration>]`,
which means the options in that submenu are loaded from and saved to that
configuration. Edits you make for these options don't get applied globally.

When hand-making a configuration yourself, options that are missing in the
configuration file will use whatever the global settings are.

For instructions for this feature, check
`ManualTransmission/Vehicles/Information.txt`.

### Driving assists

Have trouble keeping the car on the road? The `Driving assists` feature might
help!

The following assists are available:

* Anti-lock Braking System: Prevents the wheels from completely locking up
under heavy braking, so steering input is still effective.
* Traction Control System: Prevents the wheels from spinning too much and
losing control under hard acceleration.
* Electronic Stability Control: Detects understeer and oversteer and applies
the brakes to counter these effects.
* Limited Slip Differential: Simulates a limited slip differential and sends
more power to the slower wheel.
* Adaptive All-Wheel-Drive: Changes all-wheel drive distribution between front
and rear in real-time, depending on wheel slip, oversteer or understeer.
The [Handling Replacement library](https://www.gta5-mods.com/tools/handling-replacement-library)
is needed for this feature.
* Launch Control: Keeps the RPMs steady at a custom level,
to prevent too much wheelspin on launch.

### Custom camera

When animations are active, the stock first person vehicle camera is clamped
to about 15 degrees. As a workaround, the mod has a custom camera feature.

Aside from restoring the looking angles, it can also react to acceleration and
turning forces. Make sure to take a look at the camera options and their
descriptions in `Misc options` -> `Camera options`.

To hide the player head, you'll need to install
[CamxxCore](https://www.gta5-mods.com/users/CamxxCore)s' DismembermentASI,
which is included with the mod.

### Animations

The script now overrides the animations and matches the steering wheel
rotation. The system needs a bit of help to understand what to do, though.

Let me know if anything is missing, so I can update `animations.yml` to support
as many vehicle types as possible out-of-the-box.

`animations.yml` is a text file containing the animation definitions: What
animations to use for which vehicle layouts, and how many degrees of rotation
chosen the animation supports. *Most* game vehicles are present already, but
most add-ons need to be added.

If a vehicle doesn't have matching animations, do this:

1. Open the `vehicles.meta` containing your car.
2. Find the `<layout>` for your car entry.
3. Copy the contents of that (for example, `LAYOUT_STD_AE86`).
4. Paste it in `animations.yml` in a suitable animation.

You can usually guess what's suitable from the other entries already present.
The debug menu has an animation section where you can force animations, you
can also use that to find a suitable animation.

If a vehicle defines an animation clipset *not* in `animations.yml`, it can
be added.

1. Check the layout name in `vehicles.meta`.
2. Check the corresponding clipset dictionaries in `vehiclelayouts.meta`
3. Check the corresponding clipset dictionaries in `clip_sets.xml`
4. Make an educated guess what the dictionary is for your vehicle
5. Check the dictionary in `clip_anim.rpf`
6. Open the `.ycd` in notepad and hope you find a `steer_no_lean` or `pov_steer`
7. Copy an `- Animation:` entry in `animations.yml` - **mind the indentation!**
8. Substitute the dictionary and animation name for your vehicle, replace
layouts with your new layout and throw in an educated guess what the rotation
degree is.

Useful resource:
[AlexGuirre's animation list](https://alexguirre.github.io/animations-list/).

If the current steering angle is more than what the animation supports, it will
just stay at the maximum.

### Wheel FFB LUT

A lookup table (LUT) can be used in this script. A LUT can be used to customize
wheel response, for example, to linearize the response from the motors.

The supported format is the same as Assetto Corsa. Use the following tools to
generate a LUT for your wheel:

1. [WheelCheck](https://www.dropbox.com/s/3zxf5yd1bvujyi5/WheelCheck_1_72.zip?dl=0)
2. [LUT Generator for AC](https://www.racedepartment.com/downloads/lut-generator-for-ac.9740/)

The instructions are similar as the LUT generator page, but here they are
specific to this script:

1. Run WheelCheck with your wheel plugged in.
2. Set `Max Count` to 100.
3. Select `Step Log 2 (linear force test)` and wait until the test stops. The
test starts directly when the option is selected, and ends when the wheel stops
moving. It might take a while to start if your wheel has a force feedback deadzone.
4. Run LUTGenerator.exe
5. Open the `.csv` generated by `WheelCheck`. It's in the `C:\Users\<user>\Documents` folder.
6. Save the generated LUT in `ManualTransmission` folder, eg `g27.lut`.
7. Open `settings_wheel.ini`, and under the `[FORCE_FEEDBACK]` section, add `LUTFile = g27.lut`.

Using a LUT file will disable the `AntiDeadForce`, as it already corrects for any
dead spots.

If you're hand-rolling your own LUT or changing the forces, make sure the first
entry is `0|0` and the last line is `1|1`. The `Gears.log` file will also output
warnings or errors if something is wrong.

## Troubleshooting

Something don't work? Read this first.

### Game compatibility

The current version of the mod has been tested with GTA V version v1.0.1604.0
through v1.0.2245.1. Limited support runs back to v1.0.877.1, but new features
have been added since.

### Compatibility options

Check the `Developer options` -> `Compatibility settings`.

* For the VR mod [R.E.A.L.](https://github.com/LukeRoss00/gta5-real-mod), check `Disable FPV camera`
* For wheels recognized as gamepad, check `Disable input detection` and choose wheel input in main menu.

### Known issues

* __x360ce__ will conflict with input detection if throttle, brake, clutch or steering axes are mapped in x360ce. Assigning inputs without overlap is no problem.
* [__Strapped__](https://www.gta5-mods.com/scripts/pull-out-strap) will conflict with inputs.
* [__CustomSteering__](https://www.gta5-mods.com/scripts/custom-steering) will conflict with steering patching.
* [__Smooth Driving V__](https://www.gta5-mods.com/scripts/smooth-driving-v-lieutenant-dan) will conflict with inputs and gearbox.
* Wheel not detected at all when using Steam.
  * Fix: Uncheck `Generic Gamepad Configuration Support` in Steam Big Picture settings, Controller settings. (Found by Kaerali)
  * Fix: Check your wheel drivers and software (Logitech)

### Steering wheel issues

#### Strange values reported

Check if your wheel is recognized correctly, an issue with the steering wheel
drivers can cause issues with reading the correct values.

#### Steering wheel not detected

* Try toggling the mod (|\ key)
* Ensure you have removed xinput dlls from the GTA V directory

## Credits

A massive *Thank You* to everyone who contributed!

* [Rockstar Games](http://store.steampowered.com/app/271590/Grand_Theft_Auto_V/)
* [Alexander Blade](http://www.dev-c.com/gtav/scripthookv/)
* [Crosire](https://github.com/crosire/scripthookvdotnet)
* [LeFix](https://github.com/Le-Fix)
* [XMOD](https://www.gta5-mods.com/users/XMOD)
* [InfamousSabre](https://www.gta5-mods.com/users/InfamousSabre)
* [leftas](https://github.com/leftas/)
* [kagikn](https://github.com/kagikn)
* [zorg93](https://github.com/zorg93)
* [Unknown Modder](https://github.com/UnknownModder)
* [any333](https://www.gta5-mods.com/users/any333)
* [Nyconing](https://github.com/Nyconing)
* [CamxxCore](https://github.com/CamxxCore)
* [guilhermelimak](https://github.com/guilhermelimak)
* [Rbn3D](https://github.com/Rbn3D)
* [LeeC2202](https://gtaforums.com/profile/1170715-leec22/)
* All others who helped :)

## Source code

You can check the source code at
[https://github.com/E66666666/GTAVManualTransmission](https://github.com/E66666666/GTAVManualTransmission).

Feel free to make issues, PRs and other contributions :)

## Contact

If you have any issues or questions, you can find me (ikt) on the following channels:

* [My Discord server](https://discord.gg/VrrAEV4j4b)
* [GTA5-Mods.com Discord server](https://discord.com/invite/hwYVCmw)
* [GTA5-Mods.com Manual Transmission page](https://www.gta5-mods.com/scripts/manual-transmission-ikt)

Please *directly* ask your question, and remember to provide the log files :)
