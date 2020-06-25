# Changelog

## 4.8.2

* Add "Disable autostart" option
* Add "Leave engine running" option
* Add engine on/off animations
* Fix haptic stalling effect playing on wrong device
* Fix phone animation in cars not playing
* Fix LSD settings not being vehicle-specific

## 4.8.1

* Replace exceptions-using error handling, as workaround for an
[issue with ScriptHookVDotNet](https://github.com/crosire/scripthookvdotnet/issues/936)
* Update default configuration files to auto-generated ones
* Allow faster sequential shifting

## 4.8.0

New features and lots of small fixes and improvements! The settings files are
generally compatible, aside from the following:

* ini updates
  * `SHIFT_ASSISTS` typo fix: now ignores the old wrongly spelled `SHIFT_ASSIST` section
  * H-Pattern shifter assignment changed, now each gear has its own entry

New features!

* Custom first person driving camera with physics effects! Install the
`DismembermentASI.asi` file by CamxxCore for a clean, first person camera.
* Synchronized steering animations with wheel rotation
* Support mouse steering with Enhanced Custom Steering
* New alternate automatic transmission mode, by
[Nyconing](https://github.com/Nyconing)!
* Add UDP telemetry support for programs like SimHub.
DiRT 4 format thanks to [guilhermelimak](https://github.com/guilhermelimak).
* Add a simulated limited slip differential, thanks to [any333](https://www.gta5-mods.com/users/any333).

Improvements and bug fixes:

* Engine and transmission
  * Add visual and physical feedback to stalling
  * Improve stalling with additional settings
  * Improve clutch creep with additional settings
  * Fix engine revving when locked up due to direction
  * Fix delta-gear ratio for automatic transission downshifts
  * Fix ABS affecting automatic transmission shifting
  * Fix NPCs not shifting for forward/reverse
  * Fix clutch creep revving engine when a driven wheel is off the ground
  * Change engine braking to use negative throttle instead of the brakes
* Wheel button to keyboard mapping
  * Add mouse input support: LMB, RMB, MMB, X1 and X2
  * Add support for 1 button -> multiple keyboard keys
  * Add dpad support: 8 directions mappable to keyboard keys
* Settings
  * Add UI settings to Vehicle Configs
  * Add ClutchShift options to Vehicle Configs
  * Add descriptions to Vehicle Configs
  * Fix swapped up/downshift cut/blip settings in menu
  * Fix typo for `DRIVE_ASSISTS` ini section
* Driving assists
  * Add buttons to switch between or toggle driving assists sets
  * Apply TCS, ESC and ABS simultaneously
  * Improve ESC: Also use rear wheels at large corrections
* Custom steering
  * Add adjustable input speeds for CustomSteering
  * Use system timer in CustomSteering, fixes slow steering in slowmotion
  * Improve responsiveness while countersteering
  * Improve CustomSteering for amphibious cars in water
  * Disable CustomSteering for boats
  * Disable CustomSteering for cutscenes
  * Fix CustomSteering deactivation timing for better ACSPatch control
* UI
  * Improve menu layout
  * Add DashHook support for ABS light and other warning lights when stalled
  * Add color options for speedo, gear and shift mode UI elements
  * Add option to turn off UI font outlines
  * Show enabled but untriggered assists as black on dashboard indicators
  * Show triggered lights for at least 300 milliseconds before turning off
  * Show stalling progress in RPM bar
  * Fix throttle-based TCS not firing dashboard indicator icon
* General
  * Add wheel size in debug (e.g. 265/35R21)
  * Improve G-force graph smoothness
  * Fix control acquire/release timing (now only when ped has control of car)
  * Fix controller engine on/off hold trigger
  * Fix an issue for VehicleConfig, where an unspecified option used the class defaults instead of the active main settings

## 4.7.1

New features:

* Add ESC (Electronic Stability Control)
* Add ABS/TCS/ESC/Handbrake warning lights HUD element

Fixes and improvements:

* Fix controller settings not updated properly
* Fix controller hold time settings not being used
* Fix ABS acting on all wheels instead of an individual one
* Apply normal brake forces on wheels unaffected by ABS
* Allow keyboard shifting while using controller
* Add option to disable NPC scripts when MT or assists are active
* Fix automatic gearbox not shifting when a non-driven wheel is locked up
* Fix disabled scoop on dozers while custom steering is active

Wheel fixes and improvements:

* Add back misshift sounds - only works for wheels in H-pattern mode
* Fix burnout using wheel pedals not fully engaging
* Disable USB detection, caused crashes while game is paused

Force feedback changes:

* Add gamma and speed limit parameters. Gamma helps increasing FFB response at low
  speeds and decreases wobble when nearing the speed limit.  
* Fix "bump" when FFB changes direction
* Fix FFB continue playing when paused or ejected from car (for real this time!)
* Improve understeer calculations

## 4.7.0

Warning: Many options are relocated. The script will generate missing entries, so no action is needed, but affected settings will be reset to their default values. However, it's recommended to start with clean default settings.

New Steering Wheel features and improvements:

* Overhaul force feedback model, eliminating oscillation and fishtailing
* Add option to limit force feedback power
* Add anti-deadzone support on force feedback power
* Add damper effect to soft lock for a harder cutoff
* Add "Park" and "Neutral" positions for H-shifter automatic gear selection
* Synchronize the game steering wheel with the physical wheel

New general features and improvements:

* Add per-vehicle configuration, overriding a selection of main settings
* Add an enhanced custom steering mode for keyboard and controller
* Add an option for custom steering mode to override in-game steering wheel angle
* Add Traction Control as driving assist
* Add a G-force graph
* Add configurable speed timers
* Upgrade AI shifting logic to player logic, allowing AI to cruise at higher gears and select lower gears on demand more dynamically
* Fix an issue where a burnout condition is triggered when rolling back in a forward gear despite the clutch being fully held
* Fix an issue where ABS reduced brakes to all wheels instead of just affected wheels
* Reorder assist/feature priorities to: Burnout -> Engine lock -> ABS -> Traction Control -> Engine braking

Wheel fixes:

* Fix an issue where Logitech LEDs caused force feedback to get stuck to last command
* Fix an issue causing no throttle being applied when rolling back in a forward gear
* Fix an issue where rear-steered vehicle have a reversed force feedback direction
* Improve wheel axis mapping, now registers axis after lifting off the control significantly
* Support alternative input method vehicles (Deluxo, Stromberg)
* Monitor for device plugin and window switching, this should always re-gain wheel focus
* Increase wheel rotation to 1440 degrees in the menu

Other changes and fixes:

* Add support for ignoring Simple Trainer menu inputs when it's open (10.0+)
* Add an option to use manual input switching (Debug submenu, change in main menu)
* Add an option to disable player hiding, for compatibility with scripts also toggling player visibility
* Allow NPC debug info to show in the current vehicle, when being a passenger
* Improve automatic gearbox downshift conditions for gears that are spaced further from each other
* Fix an issue where assigning keyboard gears for 8, 9 , 10 wrongly assign to 7
* Fix an issue where keyboard assignment wrongly warns about a menu button being used
* Fix an issue where the update check freezes the game momentarily (run in another thread)
* Fix an issue where the update check crashes the game when no network is present
* Fix an issue causing a crash of the calling application when MT_GetShiftIndicator is called without a valid `vehData`
* Delay initial update check with 10 seconds so the notification is visible
* Remove usage of decorators for cross-script information

## 4.6.7

* Update for 1.0.1737.0 (Thank you Sparten for the help!)
* Add throttle cut and blip on up and downshifts, both toggleable
* Add an option to change clutch change rate on shifts
* Add an option to change steering wheel gamma
* Fix some vehicles unable to upshift to 2 properly with the automatic mode
* Fix vehicle flags offsets for older game versions
* Fix an issue where visibility was constantly set, causing an equipped parachute to always appear as deployed
* Fix an issue where H-pattern shifter setup didn't display text for 8th gear and up
* Change the order of drawn throttle, brake and clutch axes to match pedal layout
* Improve shift indicators in API
* Improve debug info

## 4.6.6

* Revise automatic transmission shifting behavior
* Revise clutch creep and stalling behavior
* Revise sequential/auto shift clutch behavior
* Add experimental scripted ABS
* Add update checker, allows to be turned off or to ignore an update
* Add C-style exported functions, replaces decorators and adds AI blacklisting
* Fix an issue for FiveM other players where other vehicles are not shifting
* Fix support for 10th gear
* Fix an issue where steering deadzone is not saved properly
* Fix an issue where force feedback continues playing when ejected from a car
* Fix an issue where hazard lights cancelled when turning, like normal signal lights
* Fix an issue where turning lights would override hazard lights
* Fix an issue where script re-locks `DECOR_REGISTER` if something else unlocked it
* General code improvements

## 4.6.5
* Update for v1.0.1604.0 (Arena War)
* Add support for 8th gear (added in Arena War)
* Other bug fixes and general improvements

## 4.6.4
* Improve soft lock transition
* Improve version detection
* Fix FiveM compatibility
* Fix a bug where the menu gets messed up after 10 options scrolling.
* Fix a bug where the non-Xinput controller settings loads the wrong setting for shifting mode.
* Fix AI not shifting properly when shifting is patched

## 4.6.3
* Fix a few missing vehicle offset searches.

## 4.6.2
* Add a hard rev limiter
* The settings directory is now created if the user forgot to install it
* Fix a bug where the speedometer would not show up properly
* Fix a bug where some vehicles are stuck when the automatic look back option is enabled
* Fix a few bugs related to keeping vehicle state
* Other bug fixes and general improvements
* Update for b1493. Additionally it should be a tiny bit more future-proof.

## 4.6.1
* Add H-pattern shifter assignments for automatic gearbox drive/neutral selection
* Add a workaround for FiveM version mismatch issues
* Change understeer force feedback impact
* Change how a burnout-while-reversing works in realistic pedals mode
* Fix shift up patch for pre-b1365
* Fix a crash when getting out of a car when using a wheel and then using a controller
* Fix an issue where assigning a button resets current menu session changes
* Fix an issue where the auto gearbox can't shift out of neutral

## 4.6.0
* Add support for b1365
* Add support for [Custom Camera V Plus](https://www.gta5-mods.com/scripts/custom-camera-v-plus) via decorators
* Add cinematic cam to blockable controls
* Add player hiding in FPV option
* Add throttle and brake button inputs for steering wheel
* Tweak stalling parameters
* Bug fixes and general improvements

## 4.5.3
* Fix external shiftmode setter.

## 4.5.2
* Improve clutch catching
* Add option to allow H-pattern for controller
* Fix crash on pre-v1.0.1290.1 (caused by steering patching)
* Fix crash on invalid pool handles

## 4.5.1
* Update for v1.0.1290.1
* Fix starting of damper effect
* Fix effect params

## 4.5.0
Config changes:
None to worry about

Changes:
* Improve redline detection and consistency across vehicles
* Re-write auto gearbox to accomodate new redline detection
* Fix NPC gearboxes not shifting properly
* Fix bikes shifting up without input
* Fix big trucks being weird in general
* Allow brake to switch input modes too
* Some bug fixes

Wheel changes:
* Add frontal collission force feedback
* Add proper damper effect
* Add brake gamma curve option
* Fix camera pitch change on looking sideways

## 4.4.2
* Fix a button assignment issue

## 4.4.1
Config changes:
No incompatible changes.

Changes:
* Add option to block button for controller clutch
* Add option to ignore shifts in menus (controller)
* Add option for deadzone with steering wheels
* Add Manual Transmission title above notifications
* Add warning to configure driving devices
* Improve boat and amphibious force feedback
* Scale ffb effects to steering wheel degrees
* Fix crash for steering wheels without force feedback
* Fix crash on setting up wheel buttons
* Remove support for non-custom decorators
* Small improvements here and there

## 4.4.0
Config changes:
Change force feedback related options.

Changes:  
* Revamp force feedback, it's much more accurate now!
* Add engine restart if stalling is active (push start "sim")
* Add "Custom Steering" for keyboard/controller too
* Fix wheel ffb not resetting after exiting car while effect is active
* Fix engine over-rev from miss-shift damage (it completely trashes your engine now)
* Fix downshift in auto gearbox when engine braking
* Fix clutch catch restarting engine
* Fix engine stalling parameters

## 4.3.10
No config changes

Changes:
* Fix engine sound muted on clutch press
* Fix models without dashboard shifting problems
* Fix models without dashboard engine lock problems
* Improve steering wheel setup
* Change steering wheel's toggle mod button to a "button held" instead of "button pressed" for more consistent mod toggling

## 4.3.9
Thanks for your bug reports!
Config changes: Updated defaults. No need to replace your config: it's just a more comprehensive start for new users/setups.

Fixes:
* Fix brake non-responsive while engine braking
* Fix engine locking and engine braking conflict

## 4.3.8
Config changes:
* Everything should be still compatible but I did clean up things here and there.

Updates:
* Support for v1.0.1180.2 (Smuggler's Run)

Other changes:
* Add wheels locking up due to mis-shifts and going faster than the gear supports
* Add options to show/hide individual HUD elements
* Add option to manually assign action blocking
* Re-write engine braking to work with a specified rev range, clutch+throttle dependent
* Re-write automatic gearbox downshift, now should shift down with higher revs depending on throttle input
* Implement custom rev limiter
* Fix cars moving in neutral by throttle (clutch values)
* Fix other scripts not being informed of these things (Neutral gear in speedos, etc)
* Fix car wheel steering jitteryness. Includes two codepaths, one using natives (slower, but reliable/fallback) and one just setting the value directly in memory after some patches.
* Fix reverse gear not locking up things
* Fix clutch being detected when not assigned
* Use values from game controls for steering wheel rotation picture
* Shift mode indicator is now blue when manual transmission isn't active
* Register decorators to prevent crashes  (Thanks to Unknown Modder!)
* Register new decorators, check GitHub for details
* A bunch of refactoring and cleanup

Decorator changes:
* Change decorator names to be mod-specific. Check GitHub!

## 4.3.7
Config changes:
* Menu position changed again (sorry!) for different resolutions and safe space support!
* For the rest of the files, no incompatible changes. Just additions.

Additions:
* Add throttle and brake anti-deadzone 
* Add steering wheel and pedal input HUD
* Separate stalling for H-patt. shift and seq. shift
    * Seq. clutch catch behaves like auto now, regarding brakes

Fixes:
* Properly disable steering wheel
* Draw wheel debug info boxes with less jerkiness
* Check if decorators exist before using them (prevent FiveM crash)


## 4.3.6
No incompatible config changes:
* Add `AlwaysHUD` to `[HUD]`
* Add `DisplayWheelInfo` to `[DEBUG]`
* `AlwaysHUD` allows using the mod's HUD/speedo even when the manual transmission part itself is off
* `DisplayWheelInfo` is decoupled from `ShowInfo` and can be shown on its own to prevent UI clutter

Fixes:
* Fix crash on exit/enter/switch vehicle and enable/disable mod

## 4.3.5
No incompatible config changes
* Add support for b877
* Fix menu title sizing
* Fix wrong version number being displayed

## 4.3.4
No incompatible config changes:
* Added "ALLOW_KEYBOARD" to wheel H-Shifter options

Menu config changes:
* Changed MenuY position to visually match original position

Fixes:  
* Update b1103 offsets
* Fix patches not applied when changing cars
* Fix steering lock not saved
* Fix global FFB settings readout typo
* Fix axis calibration not instantly applying

Improvements:  
* Use wheel speed data for stalling decision
* Use drive bias data for stalling decision
* Add fancy debug info about wheels (suspension, speed, locking up)
* Use "tap" detection for XInput (need help with blocking default buttons)
* Add wheel address logging
* Update menu usage

## 4.3.3
No config changes

Critical bug fix:
* Fix crashing when entering anything without wheels

Menu changes:
* Added details pane for many options!

Gameplay changes:
* Only reset vehicle stats when entering a new vehicle, not when just stepping out

4.3.3 hotfix:
* Change how steering multiplier is applied

## 4.3.2
Configuration changes:
* Added [CLUTCH_BUTTON] for wheel
* Added steering multiplier for wheel
You don't need to replace your existing configs. New values are added when set.

Menu changes:
* Update menu look and feel
* Allow to assign controller combo to open menu

Gameplay changes:
* Throttle is now considered for engine stalling
* Fix throttle only revving but not applying force when rolling back in first gear or higher
* Make clutch catching stronger again and force idle RPM when it's active.
* Significantly improve stalling mechanism

Wheel changes:
* Add support for button-based clutch
* Add option to change steering multiplier

## 4.3.1
No configuration changes

Menu changes:
* Fix subtitles not showing up on wheel button configuration
* Re-order wheel buttons and show TO_KEYBOARD buttons being pressed

Gameplay changes:
* Fix brakes being applied when rolling back in neutral
* Fix gear rattle not playing when popping out of reverse w/o clutch
* Re-enable persistent shift toggle switch
* Use normal speed for speedometer if vehicle dashboard speedometer is missing
* Fix characters being corrupted for some system configurations
* Fix cinematic cam disabled after menu close

Wheel changes:
* Use a separate force feedback calculation for planes

## 4.3.0
Configuration changes:
* Added `settings_menu.ini`
* `settings_wheel.ini`
  * `[FORCE_FEEDBACK]` section uses decimal numbers now
  * `[LOOK_LEFT]` and `[LOOK_RIGHT]` are added
  * Version updated to `430` to reflect this.
* `settings_general.ini`
  * Added `[CONTROLLER_LEGACY]` for non-Xinput stuff
  * In `[HUD]`, added `HUDFont`.
  * In `[CONTROLLER]`, `TriggerValue` uses decimal numbers now
  * Version updated to `430` to reflect this.
* `*.ini` files can now also have `true` or `false` instead of `0` or `1`. Both still work.

Additions:
* Added a menu. By default accessible with `[{` key. Pretty much everything in the config files is changeable here :)
  * Change mod options
  * Change HUD settings
  * Change wheel options, keyboard controls and controller controls
  * Assign controls in-game
  * Show existing control assignments
* Add gear rattle sound on miss-shift, clears on clutch press or proper gear change
* Support for non-Xinput controllers again (specify in .ini)

FiveM:
* Fixed speed display for b505
* Still investigating crashes?
* Not sure what happened to the rev limiter :thinking:

Wheel changes:
* Remove nonsense combined input code. Original code was plenty good to handle combined axes and I'm an idiot for making it complex...
* Delay DirectInput setup from construction to main\(\)
* Rewrite ingame pedal logic :'(. It was NOT fun.
* Input detection uses clutch too, now
* Add look left/right buttons
* Significantly increase wheel resistance when engine is off. This simulates no power steering!
* Decrease averaging delay for force feedback, for a faster FFB response

Other changes:
* Revert to GetAsyncKeyState() because ScriptHookV's API doesn't do mouse and/or differentiate between left/right shift so that's included now.
* Added normal +/-/,/. to keymap
* Make clutch grab self-accelerate a bit more tame, stop revving over 0.3
* Set engine RPM display to 0 when engine is off.
* More aggressive custom revving
* Pretty debug info
* Pretty HUD font/text

## 4.2.0 - Release
Configuration changes:
* Separate [HUD] section for HUD stuff
  * settings_general.ini: VERSION is 420R now

Wheel changes:
* Always initialize DirectInput, just to be safe

Changes:
* Add HUD elements. You can move these around freely or disable them
    * Shift mode indicator (H/S/A)
    * RPM indicator with redline options
    * Dashboard-data speedometer (kph, mph, m/s)
* Rename DIUtil to WheelConfigurator
* Gears.log is made in the ManualTransmission directory now.

FiveM support:
* Fix getWheelCompressions for FiveM
* Fix steering corrections patching for FiveM
* Ignore CrossScript for FiveM

Fixes 'n stuff:
* Adjust engine braking to be more reliable
* Adjust hill start effect so it's not affected by throttle position
* Fix a few instances where the brake/throttle are swapped while reversing
* Fix a few instances where the engine is wrongfully revved while braking
* Minor code optimizations
* Version strings for 1.0.1032.1

## 4.2.0 - beta 2
No configuration changes

Changes:
* Fix messed up clutch for single-axis throttle/brake settings
* Fix lag when patching fails: Limit patching attempts
* Update strings for game version 1.0.1011.1
* Fix vehicle moving before clutch catch point if throttle is pressed

Wheel changes:
* Fix crash when FFB applied on a null device after re-initialization
* Cleaner DirectInput Force Feedback code
* Cleaner wheel compression retrieval method for FFB detail
* Improve radio changing
* Add hold radio buttons to turn radio off

DIUtil changes:
* Fix a crash when refreshing after removing all devices
* Add dynamic device & axis detection
* Add dynamic device & button detection
* Add current function display for buttons
* Add wheel->keyboard input blocking option

## 4.2.0 - beta 1
Big changes to configuration!

Structural changes:
* Add a rudimentary configuration tool
* Restructured settings to ManualTransmission folder
* Separated wheel settings and normal settings
* Show warning for incorrect .ini versions

Wheel changes:
* Add support for multiple DirectInput devices
* Add countersteering and reduction patch
* Add axis support for handbrake
* Add back G27/G29 LEDs
* Add option for global FF multiplier
* Rework Force Feedback completely
* Tweaks to soft lock
* Add 7th gear support for H-shifter
* Disable controller rumble while wheel is active

Changes:
* Add option to turn on/off throttle+clutch engine starting
* Add option for clutch shifting in sequential
* Only stall if all wheels are on the ground
* Remove 8th gear for numpad
* General bug fixes

## 4.1.3
No changes to Gears.ini
*   Fix LeFix Speedometer compatibility

## 4.1.2
No changes to Gears.ini
*   Fix clutch not read in car when SimpleBike is on
*   Fix gearbox control still active if player isn't the driver

## 4.1.1
No changes to Gears.ini
*   Fix wheel used in bicycles

## 4.1.0
__This update changes Gears.ini__

Feature changes:
*   Add wheel soft lock options
*   Add FFB effects for burst tyres
*   Add detail FFB effects for quads
*   Add AutoGear1 for sequential as option
*   Add gearbox change to Controller layout
*   Experimental: Add support for boats and planes

Changes/Fixes:
*   Fix version printing
*   Fix engine revving while braking with steering wheel while rolling back in a non-reverse gear
*   Fix engine revving while braking with steering wheel in reverse near stop
*   Decrease CenterForce faster during oversteer: 0% centerforce @ 20% oversteer
*   Remove dampener adjustment for oversteer
*   Change README format

## 4.0.4
*   Fix Dpad press register on toggle
*   Print version number

## 4.0.3
*   Revert XInput 1.4 usage
*   Fix support for older game versions
*   Fix default settings
*   Increase version number for clarity

## 4.0.2

New things

*   Implement Cross-Script shift mode setting
*   Restore auto look back in reverse gear as option
*   Add color for top gear indicator
*   Change .ini to use human readable keyboard keys
*   Make blinkers self-cancel
*   Use smoother oversteer transition

More small fixes

*   Support for Bikers DLC
*   Fix FFDisable ignored
*   Fix key presses being detected with GTA V in the background
*   Fix Neutral gear still selected when car is in auto in a high gear
*   Fix clutch pedal read in automatic mode (ignore it now)
*   Fix non-working H-shifter mode for motorbikes (skip over it)
*   Replace normal-stop handbrake with another way to keep the car on its place

## 4.0.1

A few small fixes

*   Fix crash when force feedback can't be initialized
*   Fix wheel radio button toggled on reset
*   Fix wheel jerk on reset
*   Fix jerky reverse animation
*   Disable automatic reversal on motorbikes if `SimpleBike` is turned off
*   Add an option to only show the Neutral gear `N`
*   Change gear `0` display to `R`

## 4.0

This release is primarily focused on making the mod compatible with all wheels.

*   Add DirectInput for almost ALL wheels! All inputs should be supported:
    *   All analog input axes
    *   All buttons
    *   8 Dpad directions
    *   Support for combined pedals
    *   Analog ranges fully configurable
    *   Clutch can be disabled for 2-pedal wheels
*   Use wheel for driving without manual transmission
    *   Force feedback enabled!
*   Add wheel to keyboard assignments
*   Rewrote FFB from scratch
    *   More force feedback details on wheel level
    *   Better responsiveness
    *   Understeer and oversteer conditions for force feedback parameters
*   Included a tool to read the raw axes, to help you configure your wheel
    *   A G27 with separate axes example available in Gears.ini
    *   A G27 with combined axes example available in Gears.ini
*   Properly re-acquire wheel on mod toggle
*   Removed Logitech specific wheel support

Non-wheel related changes

*   Add an automatic gear box, implemented as R-N-D.
*   Add a hill starting effect, compensating for GTA V’s automatic brakes
*   Allow turning off cross-script communication for CitizenFX-based mods
*   Dropped SimpleReverse
*   Disable engine damage for electric vehicles
*   Add user defined controller analog-as-button trigger value
*   Change onscreen debug text format
*   Increase engine braking strength
*   Enable clutch catching for reverse (driver might play animation weirdly)
*   Fix clutch catching when car is upside down
*   Add engine off toggle for engine button
*   Disable low RPM power loss

## 3.0.2

* Fix keys not being disabled properly from wheel
* Add cross-script communication support (Decorator)
* Tweak truck speed limiter to be less harsh when shifting early

## 3.0.1

* Fix handbrake staying engaged when disabling the mod

## 3.0

* Add Logitech Wheel support
    * Steering wheel support
    * Pedal input support
    * Force Feedback, physics-based and fully configurable
    * 0-delay steering input (controller input is/was still smoothed out)
    * Useful vehicle functions available and mappable on wheel
* Add automatic input detection and isolation
    * Switch to sequential on controller input
* Change reverse in first gear behavior to do a burnout if the vehicle is strong enough
* Add separate neutral to be in between R and 1st: R-N-1-2-3... for sequential shifting
* Add clutch requirement option for H-shifting
* Add engine restart by pressing throttle
* Fix specific first gear only vehicles having a nonfunctional neutral (remove neutral for these)
* Rework neutral/clutch revving to be more gradual and natural
* Change clutch slipping in higher gears to be gradual
* Cleaner vehicle swap/leave procedure
* Fix vehicle change detection
* Fix other cars not moving when clutch pressed
* Minimize patching of game functions
    * Less risky
    * No AI impact
    * Performance improvement
* Change .ini format

## 2.0.2
* Disable clutch catching in reverse gear
* Enable free revving for trucks

## 2.0.1
* Add check for invalid configurations
* Improve requirement for AutoReverse
* Remove lockup when choosing reverse, going forwards

## 2.0
Additions:
* Add neutral gear functionality
* Add exception for motorcycles
* Add Clutch catching mechanism
Changes:
* Switch to XInput controller readout
* Change engine braking to use apply force
* Engine braking locks up engine (handbrake) when going forward in reverse gear
* Reworked engine stalling with awareness of current gear (old version still available)
* Rework revving in higher gears
Fixes and tweaks:
* RPM Disparity at high gear low RPM fixed
* Fix Utility Trucks not being recognized
* Tweaked engine braking and allow for trucks
* Remove engine stalling on lots of damage
Misc:
* Add configuration presets

## 1.8.2
* Only conditionally patch lower-end full clutch

## 1.8.1
* Fix keyboard-controller detection
* Fix ToggleH button being overwritten instead of EnableH

## 1.8
* Refactor project
* Add controller button for toggle (Dpad Right for 0.5s)
* Add keyboard key for toggle H-shifter (default: }] key)
* Fix clutch fully depressed vehicle still moving
* Disable patches and manual control as passenger
* Tweak notifications behavior

## 1.7
* Add engine braking
* Fix big trucks accelerating infinitely
* Reset gear on changing vehicle
* Turn on engine when toggling mod

## 1.6.2
* Better compatibility: Dropped Microsoft Visual C++ Redistributable 2015 (x64) requirement

## 1.6.1
* Fix wrong key in Gears.ini
* Update readme.txt properly

## 1.6
* Enable Logging to Gears.log to clean up notifications
* Improve clutch control patching and restore
* Improve clutch control at very low speeds
* Change stalling conditions
* Turn on reverse light in reverse gear

## 1.5
* Top gear known (Thank you, kagikn!)
* Full clutch control (Thank you, leftas!)
* Allow analog clutch control
* Add engine damage
* Add engine stalling
* Changed build parameters - should work for more people now

## 1.4
* Enable accelerator to be used for reverse acceleration and brake pedal to only brake and not reverse. Like a real car.

## 1.3.2
* Enable/Disable manual gears by default in .ini
* Persistent enable/disable manual gears
* Enable/Disable engaging first gear on stopping
* Enable/Disable automatic reverse gear

## 1.3.1
* Full compatibility with LeFix Speedometer for gear shift indicators
* Restore 1.2 overrevving behavior
* Tweak low-RPM low-throttle behavior

## 1.3
* Full keyboard support
* H-Shifter support - mapped to keyboard buttons
* Preliminary compatiblity with LeFix Speedometer for gear shift indicators
* Disable/enable manual notification
* Changes in ini and default config

## 1.2
* Fix version 350 compatibility issues
* Tweaked reverse behavior
* Fix overrevving behavior

## 1.1
* Transmission optimizations
* Disabled for bicycles
* Fixed motorcycle support

## 1.0
* Initial release
