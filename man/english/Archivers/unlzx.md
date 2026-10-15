# unlzx - list, test and unpack .lzx archives (extract only)

## SYNOPSIS
    unlzx [-options] command archive[.lzx] [file ...] [destdir/]

## DESCRIPTION
A small, unpack-only version of LZX: it lists, tests and unpacks .lzx
archives but cannot make them. The command line is the same as `lzx`'s:
the command, then the archive (the .lzx ending may be left out), then the
files to act on (patterns work), and a drawer to unpack into, ending in
`/` or `:`.

## COMMANDS
- **e** extract files into one drawer, without their paths.
- **x** extract files with their paths.
- **l** list (short); **v** list with detail.
- **t** test the archive.

## OPTIONS
- **-a** keep protection bits.
- **-c** ask before each file.
- **-C** clear the archive (A) bit of unpacked files.
- **-E** give unpacked files the current date.
- **-f** ignore file notes.
- **-F** faster progress display.
- **-k** keep partly unpacked files after an error.
- **-l** names in lower case; **-u** in upper case.
- **-m** ask nothing.
- **-p** wait for a key after loading (to swap floppies first).
- **-P** N
  task priority.
- **-q** print nothing.
- **-R** look for archives in drawers below too.
- **-X** how the .lzx ending is added.
- **-bi** KB
  the input buffer size.
- **-bo** KB
  the output buffer size.
- **--** no options after this.

## EXAMPLES
    unlzx x game.lzx
    unlzx x game.lzx RAM:game/
    unlzx l game.lzx
    unlzx t game.lzx

## NOTES
UNLZX 1.21 (1995) by Data Compression Technologies (Jonathan Forbes, Tomi
Poutanen), the 68000 version; part of the LZX archive, Aminet
`util/arc/lzx121r1`. Run without arguments it shows its usage and then a
contact page, after a [More] prompt that waits for a key. The options are
the same as LZX's; see LZX.guide.

## SEE ALSO
`lzx`, `lha`
