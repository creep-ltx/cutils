# PowManTool - switch the sockets of a GemBird USB power strip

## SYNOPSIS
    PowManTool [SOCKET N] [ON] [OFF] [TOGGLE] [STATUS] [UNIT N]

## DESCRIPTION
Switches the mains sockets of a GemBird SIS-PM "Silvershield" power
manager (a power strip controlled over USB) on and off, or checks them.
Only devices with USB vendor 0x04B4 and product 0xFD11 are supported.

These strips claim to be HID devices but do not speak HID, so the first
time PowManTool runs it removes the HID class's binding to them.

## ARGUMENTS
- **SOCKET** N
  the socket, 1 to 4. Without it, the command applies to all four.
  **OUTLET** is another name for it.
- **ON** switch the socket(s) on. With STATUS: check that they are on.
- **OFF** switch the socket(s) off. With STATUS: check that they are
  off.
- **TOGGLE** switch each socket the other way. With STATUS: toggle
  first, then check that they are now on.
- **STATUS** check the socket(s) and set the return code; see below.
  Alone it returns 5 if any of them is off.
- **UNIT** N
  which power strip, when several are connected (0 is the first).

## EXAMPLES
    PowManTool 1 ON
    PowManTool 2 OFF
    PowManTool 3 TOGGLE
    PowManTool OFF
    PowManTool 4 STATUS
    PowManTool OFF STATUS

The last two: 5 if socket 4 is off; 5 if any socket is on.

## RETURN CODES
With STATUS: 0 if the check holds, 5 if not (STATUS or ON STATUS: 5 when a
socket is off; OFF STATUS: 5 when a socket is on).

## NOTES
Part of Poseidon, the USB stack by Chris Hodges (version 1.0 from 2008).
Described in `HELP:Poseidon.guide`.

## SEE ALSO
`UPSTool`, `PsdDevLister`
