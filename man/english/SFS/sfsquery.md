# SFSquery - show the settings of a Smart Filesystem partition

## SYNOPSIS
    SFSquery DEVICE

## DESCRIPTION
Prints what an SFS (Smart Filesystem) partition is set to: the SFS
version, where the partition starts and ends on the disk, how the file
system talks to the disk (NSD 64-bit, SCSI direct or standard), block size
and number of blocks, the read-ahead cache (lines, size, copyback or
write-through, accesses and misses), the flush timeouts, the longest
allowed file name, the number of DOS buffers, and whether the partition is
case sensitive and has a .recycled drawer.

It only reads; nothing is changed. A partition that does not use SFS is
reported as "not SFS".

## ARGUMENTS
- **DEVICE** the partition, for example `DH0:`. Needed.

## EXAMPLES
    SFSquery DH0:
    SFSquery Work:

## NOTES
SFSquery 1.1 (2001), part of SmartFilesystem by John Hendrikx, later
developed by Joerg Strohmayer (Aminet `disk/misc/SFS`). `SFSconfig` shows
the same and can also change some of it.

## SEE ALSO
`SFSconfig`, `SFSformat`, `SFSdefrag`, `Info`
