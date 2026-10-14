# gunzip - unpack gzip files (gzip -d)

## SYNOPSIS
    gunzip [-cfklqrtv] [-S .suf] [file | pattern | - ...]

## DESCRIPTION
The same as `gzip -d`: each file.gz is replaced by file (file.tgz by
file.tar), with the packed file's date, protection bits and file note.
`gunzip foo` finds foo.gz. With no file, or `-`, it unpacks its input to
its output: `gunzip <RAM:notes.gz | more`.

gunzip carries only the unpacking part of gzip, so it is smaller
(27 KB). See `man gzip` for everything else.

## OPTIONS
- **-c** to the output, keep the files.
- **-k** keep the .gz files.
- **-f** replace files that are there already; with **-c**, pass on
  what is not gzip as it is.
- **-t** test: unpack and check, write nothing.
- **-l** list: packed size, plain size, ratio, name.
- **-r** into drawers.
- **-S** .suf
  also take files ending in .suf.
- **-v** say how much each file grew, on the error stream.
- **-q** no warnings.

## EXAMPLES
    gunzip Downloads:source.tar.gz
    gunzip -c Downloads:notes.gz | more
    gunzip -t Downloads:#?.gz

## RETURN CODES
0 fine, 5 a file was skipped (there already, no .gz suffix, could not be
read) or had bytes after its end, 10 bad arguments, not gzip, damaged
data or a failed write (the half-written file is deleted), 20 stopped
with Ctrl-C.

## SEE ALSO
`gzip`, `bunzip2`, `unxz`, `unzstd`
