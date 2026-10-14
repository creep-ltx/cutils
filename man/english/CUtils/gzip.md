# gzip - pack files smaller with gzip, or unpack them

## SYNOPSIS
    gzip [-cdfklnqrtv1..9] [-S .suf] [file | pattern | - ...]

## DESCRIPTION
Packs each file into a smaller one in the gzip format, the one every
Unix machine, every web server and most archives use. `gzip file`
replaces file with file.gz; the new file gets the old one's date,
protection bits and file note. `gzip -d file.gz` (or `gunzip`) turns it
back.

With no file, or `-`, gzip packs its input to its output, so it works in
a pipe: `tar cf - Work:Project | gzip >RAM:project.tar.gz`. Packed data
is never written to a window unless **-f** is given.

Unpacking takes files made by any gzip: several members one after
another (as `cat a.gz b.gz` makes them) come out as one file, and every
member's CRC-32 and length are checked. `gzip -d foo` finds foo.gz;
foo.tgz becomes foo.tar.

Memory: packing takes about 400 KB, unpacking about 110 KB, whatever the
file's size.

## OPTIONS
- **-c** write to the output, keep the files (`--stdout`).
- **-d** unpack (`--decompress`): the same as `gunzip`.
- **-k** keep the input files (`--keep`).
- **-f** force: replace a file that is there already, pack a file that
  ends in .gz again, write packed data to a window; with **-dc**, pass on
  what is not gzip as it is (as zcat -f).
- **-1..-9**
  fast (and bigger) to slow (and smaller); 6 unless told. `--fast` is
  the same as -1, `--best` as -9.
- **-n** no name and date in the gzip header (`--no-name`): two packings
  of the same bytes are then the same file.
- **-t** test: unpack and check, write nothing (`--test`).
- **-l** list each packed file: packed size, plain size, ratio, name
  (`--list`). The plain size is read from the file's end, as GNU gzip
  does (for a file of several members: the last one's).
- **-r** into drawers: every file below a drawer named (`--recursive`).
- **-S** .suf
  use .suf instead of .gz.
- **-v** say how much smaller each file got (`--verbose`), on the error
  stream: `name:  53.0% -- replaced with name.gz`.
- **-q** no warnings (`--quiet`); the return code still counts them.

## EXAMPLES
    gzip RAM:big.log
    gzip -9k Work:Docs/#?.txt
    gzip -d Downloads:source.tar.gz
    gzip -dc Downloads:notes.gz | more
    gzip -tv Downloads:#?.gz
    gzip -l Downloads:#?.gz
    tar cf - Work:Project | gzip -9 >RAM:project.tgz

## RETURN CODES
0 fine, 5 a file was skipped (there already, a wrong suffix, a drawer
without **-r**, could not be read) or had bytes after its end
("decompression OK, trailing garbage ignored"), 10 bad arguments, not
gzip, damaged data or a failed write (the half-written file is deleted),
20 stopped with Ctrl-C.

## NOTES
The words for damaged files: "damaged: CRC wrong", "damaged: length
wrong", "damaged: bad deflate data", "truncated", "not in gzip format".

The gzip header gets the file's name and date (unless **-n**) and the
OS byte for the Amiga. **-N** is taken and does nothing: the name in the
header is never used for the unpacked file.

Patterns may be written `#?` or `*`; upper and lower case are the same.

`gzip ?` shows the usage line.

## SEE ALSO
`gunzip`, `bzip2`, `unxz`, `unzstd`, `tar`
