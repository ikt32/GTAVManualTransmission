# Changelog

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