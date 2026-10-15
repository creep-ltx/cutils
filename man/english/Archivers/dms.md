# DMS - pack a whole floppy disk into a .dms file, and write it back

## SYNOPSIS
    DMS Read FILE[.DMS] [FROM DF0:] [TEXT textfile] [CMODE mode]
             [LOW track] [HIGH track] [NOVAL] [NOZERO]
             [ENCRYPT password] [PC] [HD] [DIZTEXT textfile]
    DMS Append FILE[.DMS] [FROM DF0:] [TEXT textfile] [CMODE mode]
             [LOW track] [HIGH track] [NOVAL] [NOZERO] [DIZTEXT file]
    DMS Write FILE[.DMS],,, [TO DF0:] [LOW track] [HIGH track]
             [NOVAL] [NOVERIFY] [NOTEXT] [NOPAUSE]
             [DECRYPT password] [PC] [HD]
    DMS Repack FILE[.DMS] [TO FILE[.DMS]] [LOW track] [HIGH track]
             [CMODE mode]
    DMS View FILE[.DMS],,, [FULL]
    DMS Text FILE[.DMS],,,
    DMS Test FILE[.DMS],,,
    DMS ?

## DESCRIPTION
DMS (the Device-Masher System) packs a complete floppy disk, track by
track, into one file, and writes such a file back onto a disk. Unlike a
file archiver it keeps the disk exactly as it was, boot block and all, so
it works for games and demos that boot from their own disk. Countless
Amiga disks were spread as .dms files. A text (a "banner") can be stored
with the archive and is shown when the disk is written back.

The first word is the command. The .DMS ending may be left out. `,,,`
means several files may be given, separated by commas, to be handled one
after another.

**DMS Write overwrites the whole disk in the drive.** Check which drive TO
names before pressing Return.

## COMMANDS
- **Read** read the disk in the drive FROM into a new .dms file.
- **Append** read tracks and add them to an existing .dms file.
- **Write** write the .dms file(s) onto the disk in the drive TO,
  erasing what was on it.
- **Repack** pack an existing .dms file again, for example with another
  CMODE or only some tracks, into the same or another file.
- **View** show the archive's information (tracks, packing, the DMS
  version and Kickstart it was made with); **FULL** lists every
  track.
- **Text** show the text stored with the archive.
- **Test** unpack in memory and check every track's checksums.
- **?** show the usage.

## OPTIONS
- **FROM** DRIVE
  the floppy drive to read from (DF0: to DF3:).
- **TO** DRIVE
  the floppy drive to write to; with Repack, the new file.
- **LOW** N
  the first track to handle (0 is the first of a disk).
- **HIGH** N
  the last track to handle (79 is the last of a normal disk).
- **CMODE** MODE
  how hard to pack. The program's own error message lists BEST, NONE,
  HEAVY1 and HEAVY2; its listings also know SIMPLE, QUICK, MEDIUM and
  DEEP from older archives.
- **TEXT** FILE
  store the text in FILE as the archive's banner. **NOTEXT** on Write:
  do not show the banner.
- **DIZTEXT** FILE
  store FILE as the archive's FILE_ID.DIZ description.
- **ENCRYPT** PASSWORD
  lock the archive with a password. It cannot be repacked then.
- **DECRYPT** PASSWORD
  the password, to write a locked archive.
- **NOVERIFY** do not read each track back after writing it.
- **NOPAUSE** do not wait after showing the banner.
- **NOVAL** not documented: neither the program nor any documentation
  found explains it.
- **NOZERO** not documented either.
- **PC** an MS-DOS (PC) disk, through a mounted PC0: style device.
- **HD** a high density disk (needs an HD drive and disk).

## FMS COMMANDS
DMS also handles FMS archives (files instead of tracks):
- **Makesfx** FILE make a self-extracting program from an FMS archive.
- **A** FILE add files to a DMS/FMS archive.
- **E** FILE extract files from it.
- **V** FILE list its files with drawers; **L** FILE without drawers.

## EXAMPLES
    DMS View game.dms
    DMS Test game.dms
    DMS Write game.dms TO DF0:
    DMS Write disk1.dms,disk2.dms TO DF1:
    DMS Read RAM:mydisk FROM DF0: CMODE HEAVY2 TEXT RAM:about.txt
    DMS Repack old.dms TO new.dms CMODE BEST

## NOTES
This copy calls itself "The DMS/FMS-Masher Professional, evaluation copy,
command line version 2.30 Extra Turbo", copyright 1994-95 PanCom Software
(Denmark), shareware. Run with a missing or unknown command it first shows
several pages of registration text, each waiting for Return. No manual for
this version was found; this page is written from the program's own usage
and messages.

## SEE ALSO
`DMSDescript`, `lha`, `lzx`
