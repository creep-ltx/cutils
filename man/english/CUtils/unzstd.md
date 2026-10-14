# unzstd - unpack zstd files

## SYNOPSIS
    unzstd [-cfklqrtv] [--rm] [-S .suf] [file | pattern | - ...]

## DESCRIPTION
Unpacks files made by zstd (Zstandard): each file.zst gives file
(file.tzst gives file.tar), with the packed file's date, protection
bits and file note. As zstd itself does, the .zst file is kept unless
**--rm** is given. With no file, or `-`, it unpacks its input to its
output.

Frames one after another come out as one file, skippable frames are
passed over, and each frame's checksum is checked when it has one.

Memory: the window the file was packed with: about the file's size for
small files, 8 MB for most others, at most 128 MB (files packed with
`zstd --long` beyond it are refused: "needs too much memory").

There is no zstd packer on the Amiga (yet): use gzip or bzip2. Frames of
zstd 0.7 and older are not read.

## OPTIONS
- **-c** to the output.
- **-k** keep the .zst files (so it is anyway).
- **--rm** delete each .zst file once it is unpacked.
- **-f** replace files that are there already; with **-c**, pass on
  what is not zstd as it is.
- **-t** test: unpack and check, write nothing.
- **-l** list: packed size, plain size (by unpacking), ratio, name.
- **-r** into drawers.
- **-S** .suf
  also take files ending in .suf.
- **-v** say how much each file grew, on the error stream.
- **-q** no warnings.

## EXAMPLES
    unzstd Downloads:source.tar.zst
    unzstd --rm Downloads:#?.zst
    unzstd -c Downloads:notes.zst | more

## RETURN CODES
0 fine, 5 a file was skipped (there already, no .zst suffix, could not
be read), 10 bad arguments, not zstd, damaged data ("damaged: checksum
wrong", "damaged: bad data", "truncated"), not enough memory or a failed
write (the half-written file is deleted), 20 stopped with Ctrl-C.

## NOTES
On a 14 MHz 68020 unzstd unpacks about 50 KB a second.

`unzstd ?` shows the usage line.

## SEE ALSO
`gunzip`, `bunzip2`, `unxz`, `tar`
