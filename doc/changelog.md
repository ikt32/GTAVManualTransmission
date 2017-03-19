# Changelog

## 4.2.0 - beta 1 -> beta 2
No configuration changes

Changes:
* Fix messed up clutch for single-axis throttle/brake settings
* Fix lag when patching fails: Limit patching attempts
* Update strings for game version 1.0.1011.1

Wheel changes:
* Fix crash when FFB applied on a null device after re-initialization
* Fix vehicle moving before clutch catch point if throttle is pressed
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
