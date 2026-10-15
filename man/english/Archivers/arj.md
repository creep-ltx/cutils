# ARJ - list, test and unpack .arj archives (UNARJ 2.41)

## SYNOPSIS
    ARJ archive[.arj]
    ARJ e|l|t|x archive[.arj]

## DESCRIPTION
In spite of its name this is not the ARJ archiver but UNARJ, Robert K.
Jung's free unpacker for ARJ archives, the format that was common on DOS
in the early 1990s. It cannot make archives.

It always works on the whole archive: individual files cannot be picked,
and there is no destination drawer - files are unpacked into the current
drawer. A file that already exists there is not overwritten. Empty drawers
and volume labels are not supported.

## COMMANDS
- **e** extract every file into the current drawer, without paths.
- **x** extract every file with its path. Missing drawers are NOT
  made: create them first (`t` shows the paths stored).
- **l** list the archive (also what happens with no command). The last
  column, BTPMGVX, shows per file: backup flag, type (text, binary,
  drawer, label), path stored, packing method, garbled (encrypted),
  continued on another volume, part of a larger file.
- **t** test the archive: check every file's CRC ("CRC OK" or "CRC
  error!"); also shows the full paths stored.

## EXAMPLES
    ARJ archive.arj
    ARJ t archive.arj
    CD RAM:unpacked
    ARJ x Work:Download/archive.arj

## NOTES
UNARJ 2.41 (1991-93) by Robert K. Jung, in an Amiga build for the 68030 by
"HEAVEN MB" (1994). Its documentation is unarj.doc in the original UnARJ
2.41 archive (on Aminet inside `util/arc/UnARJ241`, a different Amiga port
by Andreas Kleinert).

## SEE ALSO
`UnARJ`, `lha`, `unzip`
