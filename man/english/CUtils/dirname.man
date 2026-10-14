# dirname - a path without its last part (the drawer a file is in)

## SYNOPSIS
    dirname NAME ...

## DESCRIPTION
Prints everything before the last part of a path, the Amiga way:
`Work:a/b` gives `Work:a`, `Work:a` gives `Work:`, and `Work:` stays
`Work:`. A path without a volume works too: `a/b` gives `a`. A slash at
the end is left out first, so `a/b/` gives `a`.

A name with no drawer in it, such as `file`, gives an empty line. On the
Amiga the current drawer is the empty name "" (Unix says ".", but that is
an ordinary file name here). That way the result can always be used with
CD, or joined with `/` in a script.

dirname works on the text only; nothing is looked up on the disk. One
line is printed for each NAME.

## OPTIONS
dirname has no options.

## EXAMPLES
    dirname Work:Projects/game.c
    dirname SYS:C/List DH1:Music/mod.intro
    CD "`dirname Work:Projects/game.c`"
    dirname "Work:My Projects/game.c"

## RETURN CODES
0 fine, 10 bad arguments (no name given, or an option).

## NOTES
A path starting with `/` (the parent drawer) or `Work:/` keeps its slash:
`/b` gives `/`.

`dirname ?` shows the usage line.

## SEE ALSO
`basename`, `find`, `which`
