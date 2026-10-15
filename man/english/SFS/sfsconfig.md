# SFSconfig - change the cache and timing settings of an SFS partition

## SYNOPSIS
    SFSconfig DEVICE [MAXNAMELENGTH N] [ACTIVITYFLUSH S]
              [INACTIVITYFLUSH S] [RECYCLEDENTRIES N] [LINES N]
              [READAHEAD BYTES] [COPYBACK | NOCOPYBACK] [NOQUERY]

## DESCRIPTION
Sets some running parameters of an SFS (Smart Filesystem) partition - the
read-ahead cache, when changes are written to disk, the longest file name
- and then prints all its settings, as `SFSquery` does.

None of these settings is kept: after a reboot SFS has forgotten them. To
keep them, put the SFSconfig line in S:Startup-Sequence (early, for the
boot partition) or S:User-Startup.

Whether a partition is case sensitive or has a .recycled drawer is shown,
but cannot be changed here; that is chosen by `SFSformat`.

## ARGUMENTS
- **DEVICE** the partition, for example `DH0:`. Needed.
- **MAXNAMELENGTH** N
  the longest allowed file name, 30 to 120 characters. **MAXNAME** is
  the short form.
- **ACTIVITYFLUSH** S
  write pending changes to disk at the latest after S seconds even while
  the disk is busy; 5 to 120.
- **INACTIVITYFLUSH** S
  write pending changes after S seconds of the disk being idle; 1 to 5.
- **RECYCLEDENTRIES** N
  how many deleted files the .recycled drawer keeps (35 when not set).
  Not in the original documentation; read from the source.
- **LINES** N
  the number of read-ahead cache lines.
- **READAHEAD** BYTES
  the size of each cache line in bytes. The cache takes LINES times
  READAHEAD bytes of memory. **READAHEADSIZE** is the long form.
- **COPYBACK** copyback mode for the cache: writes are held in it too.
- **NOCOPYBACK** write-through mode: writes go to the disk at once.
- **NOQUERY** set the values but do not print the settings afterwards.

## EXAMPLES
    SFSconfig DH0:
    SFSconfig DH1: MAXNAME 30
    SFSconfig DH1: LINES 100 READAHEAD 32768
    SFSconfig SF0: ACTIVITYFLUSH 120 NOQUERY

The third gives a fast disk about 3 MB of cache.

## NOTES
SFSconfig 1.5 (2004) by Martin Steigerwald and Gunther Nikl, GPL, for
SmartFilesystem by John Hendrikx and Joerg Strohmayer (Aminet
`disk/misc/SFSconfig`, and in `disk/misc/SFS` with its source).

## SEE ALSO
`SFSquery`, `SFSformat`, `SFSdefrag`, `SFSobject`
