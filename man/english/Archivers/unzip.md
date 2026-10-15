# unzip - list, test and unpack .zip archives (Info-ZIP UnZip 5.1)

## SYNOPSIS
    unzip [-cflptuvz] [-ajnoqUV] zipfile[.zip] [file ...] [-x xfile ...]
          [-d drawer]
    unzip -Z [zipinfo options] zipfile[.zip] [file ...]

## DESCRIPTION
Unpacks .zip archives, the format used on Windows, Linux and the Mac.
Without options it unpacks every file in the archive into the current
drawer, with their paths; with file names (patterns work) only those. The
zipfile may itself be a pattern, to unpack several archives.

## OPTIONS
What to do (one of them, or none to extract):
- **-l** list the files (short).
- **-v** list the files with sizes, packing method and ratio.
- **-t** test the archive: unpack in memory and check every file.
- **-z** show the archive's comment.
- **-c** unpack files to the screen, each with its name.
- **-p** unpack files to the output with no messages, for piping.
- **-f** freshen: only replace files that already exist and are older.
- **-u** update: replace older files and create new ones.
- **-d** DRAWER
  unpack into DRAWER.
- **-x** FILE ...
  leave out these files (patterns work).
- **-Z** act as `zipinfo` (see that page); the rest of the line is
  zipinfo's.

Modifiers:
- **-o** overwrite existing files without asking.
- **-n** never overwrite existing files.
- **-j** junk the paths: unpack every file into the one drawer.
- **-q** quiet; **-qq** quieter.
- **-a** convert text files' line ends; **-aa** treat all files as text.
- **-U** do not turn names into lower case.
- **-V** keep VMS version numbers in names.

## EXAMPLES
    unzip archive.zip
    unzip -l archive.zip
    unzip -t archive.zip
    unzip archive.zip -d RAM:unpacked
    unzip archive.zip -x #?.exe
    unzip -o archive.zip ReadMe
    unzip -p archive.zip notes.txt | more

## NOTES
UnZip 5.1 (February 1994) from Info-ZIP, an old Amiga build. Being from
1994 it does not know the later additions to the format (files of 4 GB or
more, AES encryption, the Deflate64 and newer methods), and it does not
convert UTF-8 file names. Newer Amiga builds are on Aminet
(`util/arc/UnZip` and others).

## SEE ALSO
`zip`, `zipinfo`, `unrar`, `lha`, `tar`
