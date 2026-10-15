# PsdRestart - restart the Poseidon USB stack (obsolete)

## SYNOPSIS
    PsdRestart

## DESCRIPTION
Releases every binding between USB devices and their class drivers and
then lets Poseidon look for class drivers again, as if every device had
been plugged in anew. It prints "Restarting stack..." and takes no
arguments.

It was made for the time when Poseidon was started from ROM before
dos.library existed: running PsdRestart later in the boot restarted the
class drivers as proper DOS processes. The class drivers now adapt to that
by themselves, so the Poseidon manual calls the command obsolete.

## EXAMPLES
    PsdRestart

## NOTES
Part of Poseidon, the USB stack by Chris Hodges; this copy is version 1.2
from 2003. See `HELP:Poseidon.guide`, "PsdRestart".

Because all bindings are released, every USB device is let go for a
moment, the keyboard and mouse too. Do not run it while a USB drive is
being written to.

## SEE ALSO
`PsdStackLoader`, `AddUSBClasses`
