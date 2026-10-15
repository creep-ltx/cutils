# lha - pack files into .lha archives, list and unpack them (LhA)

## SYNOPSIS
    lha [-options] command archive[.lha] [[homedir/] file ...]
        [@listfile] [destdir/]

## DESCRIPTION
LhA is the Amiga's standard archiver: most Amiga software, Aminet
included, comes as .lha (or .lzh) files. It packs files and drawers into
one archive, keeping their names, dates, protection bits and file notes,
and unpacks them again.

The first word that is not an option is the command (only its first letter
counts, so `list` works as `l`); the next is the archive (the .lha ending
may be left out, and for most commands it may be a pattern, to act on many
archives). Then come the files to act on, which may be patterns (`#?.c`,
`*.c`, `dir/#?`). With no files given, the command acts on the whole
archive, or when adding, on everything in the current drawer.

A name ending in `/` or `:` after the archive is a drawer: when unpacking
it is where the files go (created if missing); when adding it is a *home
drawer*: the files after it are taken from there, and the home drawer's
own path is not stored:
`lha -r a src Work:Code/ #?.c` stores `game/main.c`, not
`Work:Code/game/main.c`.

Options may stand anywhere on the line. `-x` turns an option on, `-x0`
turns it off, and several may be bundled: `-rx0m`. Options that take a
number need a space after it: `-b64 -m`. Defaults can be put in the
variable LHAOPTS (`SetEnv LHAOPTS -N`); `-I` ignores it.

## COMMANDS
- **a** add files (replacing ones with the same name).
- **u** update: add files that are new or newer than in the archive.
- **f** freshen: replace only files already in the archive that have
  changed.
- **m** move: add files, then delete the originals.
- **r** replace: like u, but replaces files whatever their dates.
- **d** delete files from the archive.
- **e** extract files into one drawer, without their paths.
- **x** extract files with their paths, making drawers as needed.
- **l** list (short); **lq** only the names, without paths.
- **v** list with more detail; **vq** quietly; **vv** everything.
- **t** test: unpack the whole archive without writing anything, to
  check it is sound (return code 5 if not).
- **p** print files to the output.
- **c** concatenate: append other archives to this one.
- **h** hunt: compare the archive with the files on disk and list the
  differences (date, size, protection, file note, gone).
- **y** copy the archive with new options (for example header level).

## OPTIONS
The most used ones; `lha` alone lists them all, and many more (including
the `-Q` set) are in the guide.
- **-r** add drawers recursively (implies -x).
- **-x** keep paths: store them when adding, use them when extracting.
  `-x0` drops them.
- **-a** keep protection bits (on by default when adding).
- **-e** also store empty drawers.
- **-m** ask nothing: answer every question with yes (overwrite).
- **-q** quiet.
- **-N** no progress display.
- **-n** no byte count in the progress display.
- **-k** keep partly unpacked files after an error.
- **-c** ask before each file.
- **-w** DRAWER
  work drawer for temporary files.
- **-W** PATTERN
  leave out files matching PATTERN when adding.
- **-t** when extracting: never overwrite an existing file; **-T**
  only new files and files newer than those on disk.
- **-l** names in lower case; **-u** in upper case.
- **-z** store files without packing them.
- **-Z** also try to pack files that are packed already (.lha, .zip,
  .dms, .gif, .jpg, ... are normally stored as they are).
- **-0** pack with the old -lh1- method of LhArc 1.x; **-2** with
  -lh5- (the default); **-3** with -lh6-.
- **-H** N
  header level 0, 1 (the default) or 2.
- **-b** KB
  the size of the I/O buffer.
- **-i** FILE
  read the names of the files to act on from FILE.

## EXAMPLES
    lha x archive.lha
    lha x archive.lha RAM:unpacked/
    lha l archive.lha
    lha t archive.lha
    lha e archive.lha #?.readme RAM:
    lha -r a RAM:backup Work:Projects/
    lha -r a RAM:src Work:Code/ #?.c #?.h
    lha -r -e a RAM:all.lha Work:Docs

## NOTES
LhA 2.15 (2011), the 68040 build. LhA was written by Stefan Boberg, later
kept up by Jim Cooper and David Tritscher, and since 2004 by Sven
Ottemann; it is freeware. Aminet `util/arc/lha_68k`; the full manual is
LhA.guide in that archive.

## SEE ALSO
`lzx`, `unzip`, `tar`, `gzip`
