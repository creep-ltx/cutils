# DiskWatch - make a USB stick's volume appear when it is plugged in

## SYNOPSIS
    DiskWatch DEVICE [USBDEVICE name] [UNIT N] [INTERVAL S]

## DESCRIPTION
A file system mounted on a USB stick through Poseidon (for example the
exFAT drive EXFAT: on usbscsi.device) cannot notice when a stick is
plugged in: Poseidon only creates the device unit while a stick is there,
and the file system is only started when something uses the drive. So the
stick's icon would not appear by itself.

DiskWatch bridges that. It waits, using no CPU time, for Poseidon to
report that a USB device has been bound; then it checks for a few seconds
whether the unit has come up, and when it has, it touches the DOS device
once (with requesters turned off). That starts the file system, and the
volume appears on the Workbench a few seconds after the stick goes in.
Plugging in any USB device causes one such check; touching a drive that is
already mounted does no harm.

Without poseidon.library (removable media on another bus) it falls back to
looking every INTERVAL seconds whether the device can be opened.

It runs until stopped with Ctrl-C (`Break` on its process).

## ARGUMENTS
- **DEVICE** the DOS device to touch, for example `EXFAT:`. Needed.
- **USBDEVICE** NAME
  the exec device to watch; usbscsi.device if not given.
- **UNIT** N
  its unit; 0 if not given.
- **INTERVAL** S
  seconds between looks, in the fallback mode only; 3 if not given.

## EXAMPLES
    Run >NIL: DiskWatch EXFAT:
    Run >NIL: DiskWatch FAT0: UNIT 1

On this A1200 S:User-Startup starts it as `run >nil: c:diskwatch exfat:`.

## NOTES
DiskWatch 1.1 (2026), part of exfat-aos3, the exFAT file system for 68k
AmigaOS 3 (github.com/creep-ltx/exfat-aos3), GPL 2. Works with any
Poseidon version, 4.x or later.

## SEE ALSO
`FbxDismount`, `PsdDevLister`, `Mount`
