# unrar - list, test and unpack RAR archives

## SYNOPSIS
    unrar command [-switch ...] archive[.rar] [file ...] [@listfile]
          [destdir/]

## DESCRIPTION
Unpacks RAR archives (.rar, also split into several volumes, and RAR
self-extracting files), the format of WinRAR on Windows. It is the
official unpack-only RAR program by Alexander Roshal, ported to the Amiga.
It cannot make RAR archives.

The command comes first, then switches (each starting with `-`), the
archive, the files to act on (patterns work), and a drawer to unpack into.
Switches can also be kept in the variable RAR or in the file S:rar.conf.

## COMMANDS
- **x** extract with full paths.
- **e** extract everything into one drawer, without paths.
- **l** list the contents; **lt** with technical detail; **lb** bare
  names only.
- **v** list verbosely; **vt** and **vb** as with l.
- **t** test the archive's files.
- **p** print files to the output.

## OPTIONS
The most used switches; `unrar` alone lists them all.
- **-o+** overwrite existing files without asking.
- **-o-** never overwrite existing files.
- **-or** rename unpacked files automatically when the name is taken.
- **-y** answer yes to every question.
- **-p** PASSWORD
  the password for an encrypted archive, written straight after the
  p: `-pSecret`. **-p-** asks for none.
- **-x** PATTERN
  leave out files matching PATTERN (`-x#?.txt`); `-x@FILE` reads the
  patterns from FILE.
- **-n** PATTERN
  only files matching PATTERN (`-n#?.txt`).
- **-r** look in drawers inside the archive (for patterns).
- **-ep** leave the paths out of the names.
- **-ad** unpack into a drawer named after the archive.
- **-kb** keep files that are broken.
- **-inul** print nothing; **-idq** print less.
- **-v** list all volumes of a split archive.
- **-ierr** send all messages to the error output.

## EXAMPLES
    unrar x archive.rar
    unrar x archive.rar RAM:unpacked/
    unrar l archive.rar
    unrar t archive.rar
    unrar e -o+ archive.rar #?.txt RAM:
    unrar x -pSecret archive.part1.rar

## NOTES
UnRAR 6.11 (2022), the classic Amiga port by Marcin Labenski of Alexander
Roshal's UnRAR (Aminet `util/arc/UnRAR-6.11`; source at
github.com/mplpl/unrar_amiga). This build needs **ixemul.library** version
48 and about 4 MB of memory. On this A1200 ixemul.library was not found
(checked 2026-10-15), so it cannot start until ixemul is installed (Aminet
`util/libs/ixemul-48.0`); the same Aminet archive also has a small "mini"
build, unrar_aos_mini, that needs no ixemul.

Character sets for names are taken from the variable Charset, CODEPAGE or
RAR_CODEPAGE, or guessed from the Language variable.

## SEE ALSO
`unzip`, `lha`, `lzx`, `tar`
