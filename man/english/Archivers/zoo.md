# ZOO - pack files into .zoo archives, list and unpack them

## SYNOPSIS
    zoo {acDeglLPTuUvVx}[modifiers] archive[.zoo] [file ...]
    zoo -add|-extract|-move|-test|-print|-delete|-list|-update|
        -freshen|-comment archive[.zoo] [file ...]

## DESCRIPTION
Zoo is an archiver from the late 1980s that was used on Unix, DOS and the
Amiga before LhA took over, so some old Amiga software still comes as .zoo
files. Its archives can keep several *generations* (older versions) of a
file, and deleted entries stay in the archive until it is *packed*.

The first word is a command letter followed by modifier letters, all
written together: `zoo a`, `zoo xO`, `zoo lv`. For beginners there are
also whole words with a dash (`zoo -extract archive`).

## COMMANDS
- **a** add files. Modifiers: **u** only newer files, **n** only files
  not yet in the archive, **f** no packing, **h** the better (slower)
  packing method, **M** move (delete the originals), **c** ask for a
  comment for each file, **C** for the archive, **:** store names
  without paths, **P** pack the archive afterwards, **I** read the
  names from the input, **+** keep generations.
- **x** extract files (**e** is the same). Modifiers: **:** into the
  current drawer, ignoring paths, **/** with paths (the default), **//** making drawers
  as needed, **.** make absolute paths relative, **O** overwrite
  without asking (**OO** read-only files too), **S** overwrite newer
  files, **N** extract to nowhere (a test: `zoo xN`), **p** to the
  output, **d** also deleted files (**dd** only those), **q** quiet.
- **l** list the archive; **L** several archives; **v** in detail with
  the archive comment; **V** with file comments too. Modifiers: **C**
  show the CRC, **a** the archive name, **m** file modes, **d** deleted
  files, **q** no headers, **f** names only in columns (**1** one
  column).
- **c** change the comments of the listed files (**A** only the
  archive's).
- **D** delete files from the archive (**P** pack afterwards).
- **U** undelete files.
- **P** pack: remove deleted and replaced entries for good (leaves a
  .bak file; **E** erases it).
- **u** add only files that are newer.
- **T** give the archive the date of its newest file.
- **g** generation settings (**l** a file's limit, **A** the archive's).
- **f** filter: pack (**c**) or unpack (**u**) the input to the output.

## EXAMPLES
    zoo l archive.zoo
    zoo x archive.zoo
    zoo x// archive.zoo
    zoo xN archive.zoo
    zoo a RAM:backup Work:Docs/#?
    zoo ah RAM:docs #?.doc
    zoo -extract archive.zoo readme

## NOTES
AmigaZoo 2.1 Beta 3 (1991), the Amiga port of Rahul Dhesi's Zoo (with J.
Brian Waters); free for non-commercial use. This page follows the help
texts inside the program, which are meant to be shown by `zoo h` and `zoo
H` (in a test under emulation they printed nothing; `zoo` alone prints the
usage). The Unix Zoo 2.10 and its manual are on Aminet as
`misc/unix/zoo-2.10`.

## SEE ALSO
`lha`, `unzip`, `lzx`
