# PsdErrorlog - show the messages the Poseidon USB stack has collected

## SYNOPSIS
    PsdErrorlog [NOFLUSH] [DEBUG] [NOTIMESTAMPS]

## DESCRIPTION
Poseidon keeps its own log of information, warnings and errors: devices
plugged in and out, classes binding, things that went wrong. PsdErrorlog
prints that log, each line with its time, level and origin, and then
empties it, so the next run shows only what is new.

## ARGUMENTS
- **NOFLUSH** print the log but keep it.
- **DEBUG** print extra debugging detail. If Poseidon seems to hang,
  this is the output its author asked for.
- **NOTIMESTAMPS** leave out the date and time in front of each line.
  **NOTS** is the short form.

## EXAMPLES
    PsdErrorlog
    PsdErrorlog NOFLUSH
    PsdErrorlog NOTS >RAM:usb.log

## NOTES
Part of Poseidon, the USB stack by Chris Hodges; on this machine Poseidon
6.0 for AmigaOS 3.2 (2026). NOTIMESTAMPS is new in 6.0; the rest is
described in `HELP:Poseidon.guide`. Trident shows the same log in a
window.

## SEE ALSO
`PsdDevLister`, `PsdStackLoader`
