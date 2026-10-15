# lzx - pack files into .lzx archives, list and unpack them (LZX)

## SYNOPSIS
    lzx [-options] command archive[.lzx] [file ...] [destdir/]

## DESCRIPTION
LZX packs files tighter than LhA, and was popular on the Amiga in the
second half of the 1990s; many games and demos on Aminet are .lzx files.
It keeps names, dates, protection bits and file notes.

Its *file merging* packs several small files together as one stream, which
packs better; the price is that deleting or replacing one of them means
repacking the group.

The command line works as LhA's: the first word that is not an option is
the command, then the archive (the .lzx ending may be left out), then the
files, which may be patterns, and for extracting a drawer to unpack into,
ending in `/` or `:`. Options may stand anywhere on the line; `-x0` turns
an option off. To give a file name that starts with `-`, put `--` before
it: after `--` nothing is taken as an option.

## COMMANDS
- **a** add files.
- **u** update: add files that are new or newer.
- **f** freshen: replace changed files already in the archive.
- **r** replace files.
- **d** delete files from the archive.
- **e** extract files into one drawer, without paths.
- **x** extract files with their paths.
- **l** list (short); **v** list with detail.
- **t** test the archive.

## OPTIONS
A selection; `lzx` alone lists them all, each marked with whether it
matters when adding, extracting or both.
- **-r** look for matching files in drawers below too (turns on -x;
  add `-x0` after it to store no paths).
- **-x** store paths when adding.
- **-a** keep protection bits.
- **-e** also store empty drawers.
- **-m** ask nothing.
- **-q** print nothing (`-q1`; `-q0` is normal output).
- **-k** keep partly unpacked files after an error.
- **-c** ask before each file.
- **-o** YYYY/MM/DD
  only files dated on or after the day.
- **-O** YYYY/MM/DD
  only files dated on or before the day.
- **-0** store only; **-1** fast; **-2** normal (the default);
  **-3** more; **-9** the most, and slow (registered copies only, as
  this one is). Unpacking is equally fast for all.
- **-M** set limits for the size of a merged group.
- **-Y** also try to pack files that are packed already (.lha, .zip,
  .dms, .jpg, ...; normally stored as they are).
- **-w** DRAWER
  work drawer for temporary files.
- **-bi** KB
  the input buffer size.
- **-bo** KB
  the output buffer size.

## EXAMPLES
    lzx x game.lzx
    lzx x game.lzx RAM:game/
    lzx l game.lzx
    lzx t game.lzx
    lzx -r a RAM:backup.lzx Work:Projects/#?
    lzx -9 -r a RAM:small.lzx Work:Docs/#?

## NOTES
LZX 1.21 (1995), registered, the 68040/68060 build, by Jonathan Forbes and
Tomi Poutanen, Data Compression Technologies. LZX was shareware; its
author later released a generic key file for free, which this copy uses
(L:LZX.Keyfile). Aminet `util/arc/lzx121r1`, with the full manual
LZX.guide.

## SEE ALSO
`unlzx`, `lha`, `unzip`
