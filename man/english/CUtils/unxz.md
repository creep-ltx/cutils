# unxz - unpack xz files

## SYNOPSIS
    unxz [-cfklqrtv] [-S .suf] [file | pattern | - ...]

## DESCRIPTION
Unpacks files made by xz (XZ Utils), the format most source code comes
in now: each file.xz is replaced by file (file.txz by file.tar), with
the packed file's date, protection bits and file note. With no file, or
`-`, it unpacks its input to its output.

Streams one after another (and the zero padding xz allows between them)
come out as one file. Every check xz writes is verified: CRC32, CRC64
(xz's default) and SHA-256. Files packed with a BCJ filter (x86, ARM,
ARM-Thumb, ARM64, RISC-V, PowerPC, IA-64, SPARC) unpack too.

Memory: the window the file was packed with, at most: 8 MB for xz -6
(the default), 64 MB for xz -9. unxz starts with 64 KB and doubles it
only as the data needs, so a file smaller than its window needs about
its own size (and half that again while growing).

There is no xz packer on the Amiga (yet): use gzip or bzip2.

## OPTIONS
- **-c** to the output, keep the files.
- **-k** keep the .xz files.
- **-f** replace files that are there already; with **-c**, pass on
  what is not xz as it is.
- **-t** test: unpack and check, write nothing.
- **-l** list: packed size, plain size (by unpacking), ratio, name.
- **-r** into drawers.
- **-S** .suf
  also take files ending in .suf.
- **-v** say how much each file grew, on the error stream.
- **-q** no warnings.

## EXAMPLES
    unxz Downloads:source.tar.xz
    unxz -c Downloads:notes.xz | more
    unxz -t Downloads:#?.xz

## RETURN CODES
0 fine, 5 a file was skipped (there already, no .xz suffix, could not be
read), 10 bad arguments, not xz, damaged data ("damaged: data or check
wrong", "truncated"), not enough memory or a failed write (the
half-written file is deleted), 20 stopped with Ctrl-C.

## NOTES
On a 14 MHz 68020 unxz unpacks about 30 KB a second.

`unxz ?` shows the usage line.

## SEE ALSO
`gunzip`, `bunzip2`, `unzstd`, `tar`
