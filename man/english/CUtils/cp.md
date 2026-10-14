# cp - copy files and drawers, never overwriting by default

## SYNOPSIS
    cp [-fr] from ... to

## DESCRIPTION
Copies files, and with **-r** whole drawers. The last name is where the
copies go (TO); the names before it are what is copied (names or
patterns). With one plain source, TO may be the name of the copy. With
several sources or a pattern, TO must be an existing drawer, and each
copy goes into it under its own name.

cp is careful: a file that already exists at the target is *not*
replaced. It is skipped, and all skipped files are listed under
"not copied:" at the end. Use **-f** to replace them.

Every copy keeps its protection bits, date and filenote, as Copy CLONE
does. Unix's -p is not needed: keeping them is the Amiga way.

## OPTIONS
- **-f** replace an existing target file. cp first makes sure the target
  is not the source itself, so `cp -f file file` can never destroy your
  only copy. The old file is deleted before the copy is made: if the copy
  then fails, the old target is gone.
- **-r** copy drawers and everything in them. An existing target drawer
  is merged into; the files in it follow the same skip or **-f** rule.
- **-R** the same as **-r**.

## EXAMPLES
    cp Work:Docs/letter.txt RAM:
    cp Work:Docs/letter.txt RAM:letter-copy.txt
    cp Work:Pictures/#?.iff DH1:Backup/Pictures
    cp -r Work:Projects DH1:Backup/Projects
    cp -rf Work:Projects DH1:Backup/Projects

## RETURN CODES
0 fine, 5 something was skipped (an existing file, a link to a drawer,
a pattern that matched nothing; each is said), 10 something failed (a
drawer without **-r**, a file could not be copied), 20 stopped with
Ctrl-C.

## NOTES
Links: a soft link to a file is copied as the file it points to. A link
to a drawer is skipped (and said), never followed.

A drawer is never replaced by a file, and never copied into itself or a
drawer below itself.

A copy that breaks off (a full disk, Ctrl-C) is deleted, so no half
files are left behind. Ctrl-C is checked between files and every 128 KB
within a file.

Patterns may be written `#?` or `*`; upper and lower case are the same.

`cp ?` shows the usage line.

## SEE ALSO
`mv`, `rm`, `ln`, `mkdir`, Copy
