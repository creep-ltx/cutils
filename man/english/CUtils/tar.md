# tar - bundle files into one archive, list it, unpack it

## SYNOPSIS
    tar -c [-vzjJa] [-f ARCHIVE] [-C DRAWER] [--exclude PAT] name ...
    tar -x [-vkmO] [-f ARCHIVE] [-C DRAWER] [--strip-components N]
           [member ...]
    tar -t [-v] [-f ARCHIVE] [member ...]

## DESCRIPTION
tar puts many files and drawers into one file (an archive, a "tarball"),
with their names, dates and protection bits, and takes them out again. It
is GNU tar's command: the same letters, the same file format, so an
archive made here unpacks on Linux, macOS or Windows, and one made there
unpacks here.

The first word says what to do: **c** create, **x** extract (unpack),
**t** list. It may be written with or without a dash and bundled with
the other letters, as everyone types it: `tar xzvf file.tgz` is the same
as `tar -x -z -v -f file.tgz`. In the bundle without a dash, the letters
that take a value (f, C, T) take the next words in turn. Options may also
come after the names.

A squeezed archive (.tar.gz, .tgz, .tar.bz2, .tar.xz, .tar.zst) is
recognised by its first bytes when reading, so `tar xf` unpacks any of
them; **z** and the others are only needed when creating.

## OPTIONS
- **-c** create a new archive from the names: files, drawers with all
  that is in them, and patterns (`*` works as `#?`).
- **-x** extract. With member names (or patterns) only those, and
  everything in a drawer named.
- **-t** list the members. With **-v** the long form, as GNU tar shows it:
  mode, owner/group, size, date, name.
- **-f** ARCHIVE
  the archive file. `-` (also the default) is the input when reading,
  the output when creating (`tar cf - dir >file.tar`).
- **-v** say each name as it goes; twice (**-vv**) the long form.
- **-z** gzip, **-j** bzip2, **-J** xz, **--zstd** zstd: squeeze the new
  archive with that. Not needed for reading.
- **-a** choose the squeezer from the archive's name (.tgz, .tar.gz,
  .tar.bz2, ...). Without -a and without z or j an archive is plain,
  whatever its name, as in GNU tar.
- **-C** DRAWER
  unpack into DRAWER (it must exist). When creating, the names after
  it are taken from DRAWER, and stored without it: `tar cf x.tar -C
  Work:Projects Game` stores `Game/...`.
- **--strip-components** N
  drop the first N parts of each member's name when unpacking:
  `project-1.0/src/main.c` with N=1 becomes `src/main.c`. Members with
  no more than N parts are left out.
- **-k** keep existing files: never replace one (each is said, return
  code 5). **--keep-old-files** is the same; **--skip-old-files** keeps
  them quietly.
- **--exclude** PAT
  leave out what matches PAT: a member's whole name or any run of its
  parts, so `--exclude #?.o` leaves out every .o file and `--exclude
  CVS` every drawer called CVS. May be given many times.
- **-O** unpack the files' contents to the output instead of to files
  (`tar xOf x.tar readme`).
- **-m** do not restore the dates: unpacked files get the time of
  unpacking.
- **-p** accepted for GNU tar's sake: protection is always kept.
- **-T** FILE
  the names to store, one a line, from FILE (`-` the input).
- **--no-recursion** store a drawer itself, not what is in it.

Long forms: --create --extract (--get) --list --file --directory
--verbose --gzip --bzip2 --xz --auto-compress --to-stdout --touch
--files-from --preserve-permissions; `--name=value` or `--name value`.

## AMIGA
What is kept, both ways:

**Protection bits**: the Unix mode's r w x are the Amiga's r, w (with
d) and e; a drawer is stored as 0755. What Unix has no place for - the
h s p a bits, a w without d - and the file note are stored in two pax
keys, `AMIGA.protect` ("----rwed", as List shows it) and
`AMIGA.comment`, but only when they say more than the mode does. So an
archive made on the Amiga comes back exactly, and other tars simply
ignore the keys (GNU tar says "Ignoring unknown extended header
keyword" once for each; `--warning=no-unknown-keyword` silences it).

**Dates** to the second. The Amiga's clock has no time zone, so it is
stored as if it were UTC (as Amiga tools have always done): an archive
from a machine two hours ahead of UTC shows times two hours "in the
future" on Linux.

**Names** are stored in UTF-8 and turned back into the Amiga's Latin-1
when unpacking (a character Latin-1 does not have becomes `_`). Names
that are not UTF-8 (old Amiga archives) are taken as they are.

**Soft links** are stored as symlinks: an Amiga "/" (one drawer up)
becomes "../", ":x" (the volume's root) becomes "/x", and back again.

**Hard links**: a file met twice in the tree (MakeLink) is stored once,
the other name as a link to it, and unpacked as a hard link again (a
copy where the file system has none).

**Volume names** are not stored: `tar cf x.tar Work:Projects` stores
`Projects/...` and says it removed `Work:` (as GNU tar removes a
leading `/`). A leading `/` (up) is resolved or removed the same way.

**Long names**: up to 255 bytes fit the plain format, longer ones get a
pax header; paths deeper than 255 bytes work both ways. A name longer
than the file system allows (FFS: 30 letters) is said and skipped,
never cut short silently.

Safety when unpacking: nothing is ever written outside the drawer.
Leading `/`, volume names and anything up to a `..` are removed from the
names (and said); a `:` inside a name becomes `_`. Soft links are made
last, so no member can be written through one. An existing file is
deleted and replaced (one without the d bit is kept and said); a drawer
is never replaced by a file. Drawers get their dates and protection at
the end, after their contents.

Formats read: v7, ustar, GNU (long names and links), pax (also global
headers). Sparse files and devices are said and skipped; a member over
4 GB stops the reading (said, return code 10).

## EXAMPLES
    tar xf archive.tar
    tar xzvf program-1.0.tar.gz
    tar xf program-1.0.tar.gz -C Work:Src
    tar xf program-1.0.tar.gz --strip-components 1 -C Work:Program
    tar tf archive.tar
    tar tvf archive.tar.bz2
    tar xf archive.tar docs/readme
    tar xOf archive.tar docs/readme
    tar czf RAM:backup.tgz Work:Projects
    tar cjf RAM:backup.tar.bz2 Work:Projects
    tar caf RAM:backup.tar.gz Work:Projects
    tar cvf RAM:src.tar -C Work:Projects Game --exclude #?.o
    tar cf - Work:Docs >PIPE:docs
    tar xf - <RAM:upload.tar

## RETURN CODES
0 fine, 5 something was skipped and said (a name too long for the file
system, an existing file kept with -k, a member asked for but not in the
archive, a device), 10 trouble (a damaged or cut archive, a file that
could not be written, bad arguments), 20 stopped with Ctrl-C.

## NOTES
A damaged header is said, and tar goes on with the next good one, as GNU
tar does. An archive without its end (two zero blocks) is fine.

tar will not write an archive to the console or read one from it: give
**-f**, or redirect.

The archive being written is never stored into itself.

`tar ?` shows the usage line.

## SEE ALSO
`gzip`, `cp`, `find`, `ls`, LhA
