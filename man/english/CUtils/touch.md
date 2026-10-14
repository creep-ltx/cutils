# touch - set a file's date to now, or make an empty file

## SYNOPSIS
    touch [-c] [-d date | -r file] file | pattern ...

## DESCRIPTION
Gives each file or drawer named the date and time of now, as SetDate
does. A name that does not exist yet becomes a new, empty file.

Useful to mark a file as changed (so a backup or a build sees it as
new), to make an empty file a script looks for, or to give files the
date of another file.

## OPTIONS
- **-c** never make a new file; only date the ones that are there.
- **-d** DATE
  use this date instead of now: `2026-10-14`, `"2026-10-14 13:45"`,
  `2026-10-14T13:45:10`, the Amiga's own `"14-Oct-26 13:45:10"`,
  Today, Yesterday, a weekday name, or a time alone (`13:45`, today).
- **-r** FILE
  use the date FILE has.

## EXAMPLES
    touch RAM:marker
    touch Work:Projects/game.c
    touch -d "2026-01-01 12:00" Work:Docs/#?.txt
    touch -r Work:old.txt Work:new.txt
    touch -c Work:Projects/#?.c

## RETURN CODES
0 fine, 5 a pattern matched nothing (said), 10 a date could not be set
or a file could not be made, or a bad **-d** or **-r**, 20 stopped with
Ctrl-C.

## NOTES
A pattern only dates the files that are there; it never makes new ones.
Patterns may be written `#?` or `*`; upper and lower case are the same.

Quote a date with a space in it.

`touch ?` shows the usage line.

## SEE ALSO
`date`, `stat`, `ls`, `find`, SetDate
