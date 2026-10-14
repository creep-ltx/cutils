# bunzip2 - unpack bzip2 files (bzip2 -d)

## SYNOPSIS
    bunzip2 [-cfkqrtv] [-S .suf] [file | pattern | - ...]

## DESCRIPTION
The same as `bzip2 -d`: each file.bz2 (or .bz) is replaced by file,
file.tbz2 and file.tbz by file.tar, with the packed file's date,
protection bits and file note. With no file, or `-`, it unpacks its
input to its output.

bunzip2 carries only the unpacking part of bzip2, so it is smaller
(36 KB). A -9 file needs about 3.7 MB; with less, bunzip2 goes on in
the small mode (about 2.3 MB, slower) by itself. See `man bzip2`.

## OPTIONS
- **-c** to the output, keep the files.
- **-k** keep the .bz2 files.
- **-f** replace files that are there already; with **-c**, pass on
  what is not bzip2 as it is.
- **-t** test: unpack and check, write nothing.
- **-r** into drawers.
- **-S** .suf
  also take files ending in .suf.
- **-s** taken (the small mode is used by itself).
- **-v** say how much each file grew, on the error stream.
- **-q** no warnings.

## EXAMPLES
    bunzip2 Downloads:source.tar.bz2
    bunzip2 -c Downloads:notes.bz2 | more

## RETURN CODES
0 fine, 5 a file was skipped (there already, no .bz2 suffix, could not
be read) or had bytes after its end, 10 bad arguments, not bzip2,
damaged data, not enough memory or a failed write (the half-written
file is deleted), 20 stopped with Ctrl-C.

## SEE ALSO
`bzip2`, `gunzip`, `unxz`, `unzstd`
