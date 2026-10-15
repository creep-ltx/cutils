# PsdStackLoader - start the Poseidon USB stack as set up in Trident

## SYNOPSIS
    PsdStackLoader

## DESCRIPTION
Starts the whole USB stack in one go: it reads the Poseidon settings
(ENVARC:Sys/poseidon.prefs, written by the Trident preferences editor),
starts the host controllers listed there and loads the class drivers, so
the USB devices come up. It is the one command the startup needs;
`AddUSBHardware` and `AddUSBClasses` do the same jobs by hand.

On this A1200 it is run from S:User-Startup as `ENVARC:PsdStackloader`.

It takes no arguments.

## EXAMPLES
    PsdStackLoader

## NOTES
Part of Poseidon, the USB stack by Chris Hodges; on this machine Poseidon
6.0 for AmigaOS 3.2 (2026). "Error loading poseidon.prefs!" means the
settings file could not be read.

In the older Poseidon 4.x, ENVARC:PsdStackloader was a program written by
Trident with the settings built into it, and could be made resident with
`PsdLoadModule` so that USB worked before the disk booted; the manual
(`HELP:Poseidon.guide`, "RomTags") describes that.

## SEE ALSO
`AddUSBHardware`, `AddUSBClasses`, `PsdLoadModule`, `PsdErrorlog`
