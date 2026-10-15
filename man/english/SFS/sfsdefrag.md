# SFSdefrag - defragment a Smart Filesystem partition

## SYNOPSIS
    SFSdefrag DEVICE [ADDBUFFERS N] [NOWINDOW] [DEFAULTFONT]
              [SHOWRESULT] [WINLEFT X] [WINTOP Y] [WINWIDTH W]
              [WINHEIGHT H] [DEBUG]

## DESCRIPTION
Starts the defragmenter that is built into SFS (Smart Filesystem) on a
partition: it moves the parts of files together, and the free space too,
so the disk reads faster. It starts at once and does not wait for the disk
to be idle, so other work on that disk is slow meanwhile. The partition
stays usable while it runs, and because SFS writes safely, a crash or
power cut during defragmenting loses nothing.

A window shows a map of the disk with the defragmenter's reads and writes
(other programs' writes are not shown). Closing the window stops it;
running SFSdefrag again carries on where it stopped. It needs memory for
the disk's bitmap, about 0.25% of the partition's size.

## ARGUMENTS
- **DEVICE** the partition, for example `DH1:`. Needed.
- **ADDBUFFERS** N
  give the partition N more buffers while defragmenting (and take them
  away afterwards); it goes faster. 1000 to 4000 is suggested.
- **NOWINDOW** no map window; only a small window is shown.
- **DEFAULTFONT** with NOWINDOW: use the default font for the small
  window instead of the screen's font.
- **SHOWRESULT** at the end, print how many blocks were moved out of how
  many, and the percentage.
- **WINLEFT** X
  the map window's left edge.
- **WINTOP** Y
  the map window's top edge.
- **WINWIDTH** W
  the map window's width (nothing with NOWINDOW).
- **WINHEIGHT** H
  the map window's height (nothing with NOWINDOW).
- **DEBUG** debugging output; not documented further.

## EXAMPLES
    SFSdefrag DH1:
    SFSdefrag DH1: ADDBUFFERS 2000 SHOWRESULT
    SFSdefrag DH1: ADDBUFFERS 500 WINLEFT 14 WINTOP 16 NOWINDOW

## NOTES
SFSdefragment 1.4 (2006), installed here as SFSdefrag; part of
SmartFilesystem by John Hendrikx and Joerg Strohmayer (Aminet
`disk/misc/SFSdefrag`, also in `disk/misc/SFS`). It needs SFS 1.83 or
newer.

## SEE ALSO
`SFSquery`, `SFSconfig`, `SFSformat`
