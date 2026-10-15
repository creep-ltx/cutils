# UPSTool - ask a USB UPS whether the mains is on, and control it

## SYNOPSIS
    UPSTool [RAW CMD] [ID] [QUERY] [TOGGLEBEEPER] [SHUTDOWN]
            [STARTBATTERYTEST] [STOPBATTERYTEST] [UNIT N]

## DESCRIPTION
Talks to an uninterruptible power supply (UPS) on USB. Only devices with
USB vendor 0x0F03 and product 0x0001 are looked for. The tool sends short
text commands to the UPS and prints what it answers; the commands are
those of the serial protocol used by many cheap UPSes (often called the
Megatec protocol).

With no arguments it checks the status and says "On line." when the UPS
runs from the mains, or "On battery!" when the mains has gone, and sets
the return code to match - so a script can shut the Amiga down in time.

## ARGUMENTS
- **RAW** CMD
  send the command CMD as it is and print the answer.
- **ID** print the UPS's identification (command `I`).
- **QUERY** print the raw status line (command `Q1`): voltages, load,
  frequency, battery and status bits.
- **TOGGLEBEEPER** switch the UPS's alarm beeper on or off (command
  `Q`). **TG** is the short form.
- **SHUTDOWN** tell the UPS to switch its output off (command `S01`).
  The Amiga loses power when it does.
- **STARTBATTERYTEST** start a battery test that runs until the battery
  is low (command `TL`). **STARTBT** is the short form.
- **STOPBATTERYTEST** stop a battery test (command `CT`). **STOPBT** is
  the short form.
- **UNIT** N
  which UPS, when several are connected (0 is the first).

## EXAMPLES
    UPSTool
    UPSTool QUERY
    UPSTool ID
    UPSTool RAW Q1

## RETURN CODES
Without arguments: 0 on mains ("On line."), 5 on battery. Other codes are
not documented.

## NOTES
Part of Poseidon, the USB stack by Chris Hodges (version 1.0 from 2007).
It is not described in Poseidon's manual; this page is written from the
tool's source code (in the AROS sources, rom/usb/shellapps/UPSTool.c). How
long the UPS waits after SHUTDOWN depends on the UPS.

## SEE ALSO
`PowManTool`, `PsdDevLister`
