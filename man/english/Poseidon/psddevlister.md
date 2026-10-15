# PsdDevLister - list every USB device Poseidon knows, in detail

## SYNOPSIS
    PsdDevLister [SHOWROOT] [QUICK] [STRINGS] [TOPOLOGY]

## DESCRIPTION
Prints a report on each USB device connected: its name, maker and
vendor/product numbers, serial number, speed, address and the hub port it
is on, power supplied and drawn, its configurations, interfaces and
endpoints, and which class driver has bound to it. This is the report to
include when asking for help with a USB problem.

## ARGUMENTS
- **SHOWROOT** also list the root hubs (the controllers' own ports).
  They are skipped by default because they say little.
- **QUICK** a shorter report: leaves out some of the detail.
- **STRINGS** also read out the text strings stored in each device.
  Off by default because some devices crash when asked.
- **TOPOLOGY** at the end, also draw the tree of hubs and ports: which
  device hangs on which port of which hub.

## EXAMPLES
    PsdDevLister
    PsdDevLister QUICK TOPOLOGY
    PsdDevLister >RAM:usb-report.txt

## NOTES
Part of Poseidon, the USB stack by Chris Hodges; on this machine Poseidon
6.0 for AmigaOS 3.2 (2026). TOPOLOGY is new in 6.0; the rest is described
in `HELP:Poseidon.guide`.

## SEE ALSO
`PsdErrorlog`, `AddUSBHardware`, `AddUSBClasses`
