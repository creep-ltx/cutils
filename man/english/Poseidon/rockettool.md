# RocketTool - aim and fire a USB foam missile launcher

## SYNOPSIS
    RocketTool [LEFT] [RIGHT] [UP] [DOWN] [FIRE] [TIME N] [JOYPORT N]
               [UNIT N]

## DESCRIPTION
Drives one of the USB toy rocket launchers that were sold everywhere in
the 2000s: turns it, tilts it and fires its foam missiles. Only launchers
with USB vendor 0x1130 and product 0x0202 are supported.

These launchers claim to be HID devices but do not speak HID, so the first
time RocketTool runs it removes the HID class's binding to them.

## ARGUMENTS
- **LEFT** turn left for a moment.
- **RIGHT** turn right for a moment.
- **UP** tilt up.
- **DOWN** tilt down.
- **FIRE** fire one of the three missiles.
- **TIME** N
  how long to move, in ticks (50 ticks are a second).
- **JOYPORT** N
  steer it live with the joystick or joypad in port N instead (a USB
  joypad works too). Ctrl-C ends this.
- **UNIT** N
  which launcher, when several are connected (0 is the first).

## EXAMPLES
    RocketTool LEFT TIME 100
    RocketTool RIGHT UP TIME 50 FIRE
    RocketTool FIRE
    RocketTool JOYPORT 1

## NOTES
Part of Poseidon, the USB stack by Chris Hodges (version 1.1 from 2007).
Described in `HELP:Poseidon.guide`.

## SEE ALSO
`PsdDevLister`
