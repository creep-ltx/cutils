# bzip2 - pack files smaller with bzip2, or unpack them

## SYNOPSIS
    bzip2 [-cdfkqrtv1..9] [-S .suf] [file | pattern | - ...]

## DESCRIPTION
Packs each file in the bzip2 format: usually smaller than gzip, but much
slower, and it needs much more memory. `bzip2 file` replaces file with
file.bz2; the new file gets the old one's date, protection bits and
file note. `bzip2 -d file.bz2` (or `bunzip2`) turns it back.

With no file, or `-`, bzip2 packs its input to its output. Packed data
is never written to a window unless **-f** is given.

Unpacking takes streams one after another (as `cat` or pbzip2 make
them) as one file, and checks every block's CRC.

Memory: packing needs 400 KB and 8 times the block: 7.6 MB at -9 (the
default), 1.2 MB at -1. A file smaller than the block gets a smaller one
by itself (the packed bytes are the same, only the level in the header
differs), so small files pack in little memory. Unpacking a -9 file
needs about 3.7 MB; with less, bzip2 goes on in the library's small mode
(about 2.3 MB, slower) by itself.

## OPTIONS
- **-c** write to the output, keep the files (`--stdout`).
- **-d** unpack (`--decompress`): the same as `bunzip2`.
- **-k** keep the input files (`--keep`).
- **-f** force: replace a file that is there already, pack a .bz2 again,
  write packed data to a window; with **-dc**, pass on what is not
  bzip2 as it is.
- **-1..-9**
  the block: 100 KB to 900 KB (9 unless told). `--fast` is the same
  as -1, `--best` as -9.
- **-t** test: unpack and check, write nothing (`--test`).
- **-r** into drawers (`--recursive`).
- **-S** .suf
  use .suf instead of .bz2.
- **-s** taken for bzip2's small mode (`--small`); it is used by itself
  when memory is short.
- **-v** say how much smaller each file got, on the error stream.
- **-q** no warnings (`--quiet`).

## EXAMPLES
    bzip2 RAM:big.log
    bzip2 -1 Work:Data/#?.csv
    bzip2 -d Downloads:source.tar.bz2
    bzip2 -dc Downloads:notes.bz2 | more
    bzip2 -t Downloads:#?.bz2

## RETURN CODES
0 fine, 5 a file was skipped (there already, a wrong suffix, a drawer
without **-r**, could not be read) or had bytes after its end, 10 bad
arguments, not bzip2, damaged data, not enough memory or a failed write
(the half-written file is deleted), 20 stopped with Ctrl-C.

## NOTES
Unpacking takes .bz2, .bz, .tbz2 and .tbz (the last two become .tar).
On a 14 MHz 68020 bzip2 packs about 5 KB a second at -1 and unpacks
about 9 KB a second in small mode: gzip is far quicker on a plain
Amiga.

`bzip2 ?` shows the usage line.

## SEE ALSO
`bunzip2`, `gzip`, `unxz`, `unzstd`, `tar`
