# AddUSBClasses - load all USB class drivers into Poseidon

## SYNOPSIS
    AddUSBClasses [QUIET] [REMOVE]

## DESCRIPTION
Looks through SYS:Classes/USB and gives every class driver found there
(hid.class, massstorage.class, hub.class, ...) to the Poseidon USB stack,
or with REMOVE takes them all out again. The class drivers are what make a
device useful: a keyboard, a memory stick, a network adapter. Files ending
in .info, .dbg or .elf are skipped.

At boot this is done by `PsdStackLoader`; AddUSBClasses is for doing it by
hand, for example after copying a new class into SYS:Classes/USB.

If no controller driver keeps poseidon.library open, the library and its
classes may be thrown out of memory again. So the order is: first
`AddUSBHardware`, then AddUSBClasses.

## ARGUMENTS
- **QUIET** print nothing.
- **REMOVE** take all classes out of the stack instead of adding them.

## EXAMPLES
    AddUSBClasses
    AddUSBClasses QUIET
    AddUSBClasses REMOVE

## NOTES
Part of Poseidon, the USB stack by Chris Hodges; on this machine Poseidon
6.0 for AmigaOS 3.2 (2026). Its manual is `HELP:Poseidon.guide`.

## SEE ALSO
`AddUSBHardware`, `PsdStackLoader`, `PsdDevLister`
