F.A.Q.
======

## Q: Why doesn't it work?!

A: What exactly doesn't work? Does your wheel/controller/keyboard not behave like expected, can't you shift, does Gears.asi crash? Being specific helps me help you :)

Please post settings_wheel.ini, settings_general.ini and Gears.log along with what didn't work and what you tried when you can't figure it out.

## Q: How do I install this?

A: There's an extensive [README](https://github.com/E66666666/GTAVManualTransmission/blob/master/doc/README.md). I'm sorry, but you do need to put some effort in configuring it, especially if you use a steering wheel. Until I can cook up something more user-friendly, DIUtil should be okay.

The explanation of all options and features is in the Wiki, you can jump back and forth using the Table of Contents ;)

## Q: Can I play GTA:Online?

A: No. Running mods online is a bannable offense.

## Q: Can I play FiveM with this?

A: No.

## Q: Why doesn't my controller work?

A: For controllers, this mod uses XInput, basically what Xbox 360 and Xbox One controllers use. If you use other controllers to play the game, you might want to make use of [X360CE](http://www.x360ce.com/). 

## Q: Why is my steering wheel not detected?

A: Ensure you have the correct GUID in settings_wheel.ini. You can find it in DIUtil.log or Gears.log when you first run the mod or the tool with a wheel or controller connected.

Also, ensure the first device starts with DEV0 and if you use multiple devices, the device numbers have to be contiguous.

## Q: Why is my steering wheel misbehaving?

A: Check if it shows up correctly (common issue for older Logitech wheels) and verify it in some other game. 

## Q: Why does the steering wheel oscillate when driving fast?

A: Due to how I implemented force feedback, this effect happens. Increasing dampening (making the steering heavier) or decreasing the physics effects help, if this is excessive. I'm still looking for a more accurate way :)

## Q: Hey! Why didn't you implement feature X?

A: Let me know and I'll be happy to address it. Suggestions are always good ^^

