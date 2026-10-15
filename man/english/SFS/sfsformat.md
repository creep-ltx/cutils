# SFSformat - quick-format a Smart Filesystem partition

## SYNOPSIS
    SFSformat DEVICE DRIVE: NAME VOLUMENAME [CASESENSITIVE] [NORECYCLED]
              [SHOWRECYCLED]

## DESCRIPTION
Erases a partition that uses SFS (Smart Filesystem) and gives it a new,
empty file system with the name given: a quick format. EVERYTHING ON THE
PARTITION IS LOST. It asks first: Return formats, Ctrl-C stops.

It only works on partitions that already use SFS; other file systems are
left alone. It does not test the disk surface: for that, and for a full
format, use the system's `Format`. It also sets two things that can only
be chosen when formatting: case sensitive names and the .recycled drawer.

## ARGUMENTS
- **DEVICE** DRIVE:
  the partition to format, for example `DH2:`. Needed; the keyword
  must be typed. **DRIVE** is another name for it.
- **NAME** VOLUMENAME
  the name of the new volume. Needed; the keyword must be typed.
- **CASESENSITIVE** names that differ only in upper and lower case are
  different files (`foo`, `Foo` and `FOO` are three files), as on Unix.
  Without it they are the same file, as with FFS.
- **NORECYCLED** no .recycled drawer (where SFS keeps recently deleted
  files). It cannot be added later.
- **SHOWRECYCLED** show the .recycled drawer in directory listings; it
  is hidden by default.

## EXAMPLES
    SFSformat DRIVE DH2: NAME Empty
    SFSformat DRIVE DH3: NAME Unix CASESENSITIVE

## NOTES
SFSformat 1.1 (1999), part of SmartFilesystem by John Hendrikx, later
developed by Joerg Strohmayer (Aminet `disk/misc/SFS`, the file
SFSformat.txt there). For partitions larger than 4 GB, or beyond the first
4 GB of a disk, the SFS documentation points to the system's Format or
HDInstTools instead.

## SEE ALSO
`SFSquery`, `SFSconfig`, `SFSdefrag`, `SFSobject`, `Format`
