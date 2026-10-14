# ln - make links: hard links or soft links (MakeLink)

## SYNOPSIS
    ln [-sfv] TARGET [LINK]
    ln [-sfv] TARGET ... DRAWER

## DESCRIPTION
Makes a link: a second name for a file or drawer, as the Amiga's
MakeLink does.

By default the link is a *hard link*, as on Unix: a second name for the
very same file, on the same volume. Change the file through either name
and both see it.

With **-s** it is a *soft link* instead: a small entry that holds the
path TARGET. A soft link may point to another volume, and the target
need not exist yet. The path is kept as you typed it, so a relative
TARGET is taken from the drawer the link is in.

With no LINK, the link gets the target's name, in the current drawer.
When LINK is an existing drawer, or several targets are given, a link
of each target's name is made in that drawer.

## OPTIONS
- **-s** make soft links.
- **-f** replace a LINK that is already there (never a drawer, and never
  the target itself).
- **-v** say each link made: `link -> target`.

## EXAMPLES
    ln -s Work:Tools/Sumi C:sumi
    ln Work:Docs/notes.txt Work:Desk/notes.txt
    ln -s Work:Projects RAM:p
    ln -sv Work:Tools/a Work:Tools/b C:
    ln -sf Work:Tools/Sumi-0.3 C:sumi

## RETURN CODES
0 fine, 10 a link could not be made, or bad arguments, 20 stopped with
Ctrl-C.

## NOTES
A hard link cannot go to another volume (use **-s**). A hard link to a
volume or assign root (`RAM:`) is refused too: it would make a loop no
tool could walk. A hard link to a drawer on the same volume works (the
Amiga allows it); CUtils commands never walk into one.

Some file systems do not support links at all, or only some kinds; the
message then says so.

ln takes no patterns: name each target.

`ln ?` shows the usage line.

## SEE ALSO
`ls`, `stat`, `cp`, `rm`, MakeLink
