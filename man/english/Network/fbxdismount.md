# FbxDismount - cleanly unmount a filesysbox (FUSE) file system

## SYNOPSIS
    FbxDismount DEVICE

## DESCRIPTION
Unmounts a file system that runs on filesysbox.library - the Amiga library
that ported FUSE file systems run on, such as exFAT - and stops its
handler. It works even while files are still open or programs wait for
changes on the drive: filesysbox's lock handler takes over those locks and
notify requests, so nothing is left pointing at the gone handler. When the
handler has ended, the device entry is removed too.

This is not the same as the system's `Assign DISMOUNT`, which only removes
the name and leaves the file system running in the background.

## ARGUMENTS
- **DEVICE** the device, with or without the colon (`EXFAT:`).
  Needed.

## EXAMPLES
    FbxDismount EXFAT:

## RETURN CODES
0 unmounted; 10 no such device, or the handler refused (its reason is
printed).

## NOTES
FbxDismount 54.1 (2026) by Fredrik Wikstrom, part of filesysbox.library
(AROS Public License; github.com/salass00/filesysbox). On this A1200 the
exFAT drive (exfat-aos3, see `DiskWatch`) uses filesysbox.

## SEE ALSO
`NFSDismount`, `DiskWatch`, `Assign`
