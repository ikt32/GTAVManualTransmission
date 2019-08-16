# Notes

This document is for some light notekeeping about decisions that in the
future could be forgotten.

## XInput 1.3

Linking to XInput 1.3 is needed for compatibility reasons. Linking to the
XInput 1.4, that the Windows SDK does by default, will result in a CTD on
unloading the manual transmission script.

For more information, refer to [this GTAForums.com thread](https://gtaforums.com/topic/844374-cxinput-crash-on-script-reload/).

DirectInput is unaffected.
