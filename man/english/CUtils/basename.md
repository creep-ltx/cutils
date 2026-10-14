# basename - the last part of a path (file name without drawer)

## SYNOPSIS
    basename NAME [SUFFIX]
    basename -a [-s SUFFIX] NAME ...

## DESCRIPTION
Prints the last part of a path: what comes after the last `/` or `:`.
`Work:dir/file.c` gives `file.c`. A slash at the end is left out first, so
`Work:dir/` gives `dir`. A volume or assign alone stays as it is: `Work:`
gives `Work:`, because that is the name of the place.

When a SUFFIX is given, it is taken off the end of the name too, as long
as the name is longer than the suffix: `basename file.c .c` gives `file`.

basename works on the text only. Nothing is looked up on the disk, so the
file does not have to exist. It is most useful in scripts, to turn a full
path into a plain name.

## OPTIONS
- **-a** every word is a NAME; one line is printed for each.
- **-s** SUFFIX
  take SUFFIX off the end of each name. Implies **-a**.

## EXAMPLES
    basename Work:Projects/game.c
    basename Work:Projects/game.c .c
    basename -s .info Work:Tools/a.info Work:Tools/b.info
    find Work:Projects -name #?.c | xargs basename -a

## RETURN CODES
0 fine, 10 bad arguments (no name, or more than one name without **-a**).

## NOTES
A path with only drawers at the end, such as `/` or `Work:/`, is printed
as it is: on the Amiga a lone `/` means the parent drawer.

`basename ?` shows the usage line.

## SEE ALSO
`dirname`, `find`, `xargs`
