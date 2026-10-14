# rm - delete files and drawers, carefully

## SYNOPSIS
    rm [-rfv] file | pattern ...

## DESCRIPTION
Deletes files, and with **-r** whole drawers. It does C:Delete's job
with Unix options, and with some safety catches:

- A drawer is refused without **-r** (Delete ALL would go ahead).
- A file that is delete-protected (its d bit is off: the Amiga's own
  safety catch) is skipped, and all such files are listed under
  "not deleted:" at the end. **-f** takes the protection off and deletes
  it anyway. A file another program has open is never deleted.
- A volume or assign itself is always refused: `rm -rf DH0:` deletes
  nothing.
- A link is deleted as a link: the link goes, never what it points at.

With **-r**, each drawer is read whole before anything in it is
deleted, and drawers go deepest first. If something inside will not go,
the drawers holding it stay, each saying "directory not empty".

## OPTIONS
- **-r** delete drawers and everything in them.
- **-R** the same as **-r**.
- **-f** force: take off the delete protection where it is set, and say
  nothing about a name or pattern that is not there.
- **-v** print each name as it is deleted.

## EXAMPLES
    rm RAM:old.txt
    rm Work:Projects/#?.o
    rm -r RAM:Temp
    rm -rf Work:Build
    rm -v T:#?
    find Work: -name #?.bak | xargs rm

## RETURN CODES
0 fine, 5 something was skipped (delete-protected) or not there (said),
10 something failed (a drawer without **-r**, a volume root, a file in
use), 20 stopped with Ctrl-C (checked between entries).

## NOTES
There is no undo, and rm does not ask before it deletes: check a
pattern first with `ls` (`ls Work:Projects/#?.o`).

Patterns may be written `#?` or `*`; upper and lower case are the same.

`rm ?` shows the usage line.

## SEE ALSO
`find`, `xargs`, `chmod`, `mv`, `ls`, Delete, Protect
