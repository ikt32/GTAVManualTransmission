DIUtil
==============================

This DIUtil tool (DirectInputUtil) is to be used (temporarily) for 
configuration of the main mod. `DIUtil.exe` should be in 
`<GTA V Dir>\ManualTransmission\` along with `settings_general.ini`
and `settings_wheel.ini`.

Usage is two-step. First, just launch `DIUtil.exe`. A console window will show,
but nothing of value is shown. Just 0-values. Exit, and check `DIUtil.log`. It
will contain devices it found that can be used. Insert these under 
`INPUT_DEVICES` in `settings_wheel.ini` for usage, like so

## 1. First run output:
```
[02:20:41.855] Manual Transmission v4.2.0 - DirectInput utility
[02:20:41.860] Initializing steering wheel
[02:20:41.882] Found 2 device(s)
[02:20:41.882] Device: Controller (XBOX 360 For Windows)
[02:20:41.882] GUID:   {0D50F5F0-7C13-11E4-8001-444553540000}
[02:20:41.882] Device: Logitech G27 Racing Wheel USB
[02:20:41.882] GUID:   {F69653F0-19B9-11E6-8002-444553540000}
[02:20:41.892] Initializing force feedback device
[02:20:41.892] Force feedback device not found
```

## 2. Add to `settings_wheel.ini`
```
[INPUT_DEVICES]
DEV0 = Logitech G27 Racing Wheel USB
GUID0 = {F69653F0-19B9-11E6-8002-444553540000}
```

Upon re-running, it will show your last device's info. This can be used to get
the axis positions and button numbers.

When adding more devices, repeat these two steps. The last entry in 
`[INPUT_DEVICES]` is used for display. The `DIUtil.log` also shows if the 
chosen steering axis has force feedback.

# Plans
This is a rather work-intensive way of configuring, so I'm making a GUI at the
moment but things are not going well.

Some Qt, C++/CLI or whatever C++ GUI framework help is very welcome. As for
now, this should suffice to get things working at all.

