# stat - show everything the file system says about a file

## SYNOPSIS
    stat [-L] [-t] [-c FORMAT] file | pattern ...

## DESCRIPTION
Shows all that is known about each file or drawer: its type, size, how
many blocks it takes, its protection bits, its date with the weekday,
and its filenote (comment) when it has one:

       File: S:Startup-Sequence
       Type: file
       Size: 51 bytes, 2 blocks
    Protect: ----rw-d
       Date: 2026-08-15 10:53:32 (Saturday)
    Comment: a filenote, when there is one

The Type is file, drawer, volume (a root such as `Work:`), soft link
(with where it points), hard link to a file, or hard link to a drawer.
A soft link is described itself, not what it points to, unless **-L**
is given.

Protect shows the Amiga's eight bits as List does: h (hold), s
(script), p (pure), a (archived), then r w e d (read, write, execute,
delete); a `-` means the bit is off, or the action not allowed.

## OPTIONS
- **-L** follow a soft link: describe what it points to.
- **-t** terse: one line each, `name size blocks protect date`.
- **-c** FORMAT
  print only FORMAT for each file, with these filled in:
  `%n` the name, `%N` the name and a soft link's target, `%s` the
  size, `%b` the blocks, `%A` the protection bits, `%F` the type, `%y`
  the date (2026-08-15 10:53:32), `%Y` the date in seconds since 1970,
  `%C` the filenote, `%%` a %. `\n` and `\t` are a new line and a
  tab. A new line is added at the end.

## EXAMPLES
    stat S:Startup-Sequence
    stat -t SYS:C/#?
    stat -c "%s %n" Work:Pictures/#?.iff
    stat -c "%A %n" SYS:Libs/#? | grep -- "-p-"
    stat -L C:sumi

## RETURN CODES
0 fine, 5 a file was not found or a pattern matched nothing (said),
10 bad arguments, 20 stopped with Ctrl-C.

## NOTES
Patterns may be written `#?` or `*`; upper and lower case are the same.

`stat ?` shows the usage line.

## SEE ALSO
`ls`, `file`, `chmod`, `touch`, `du`, List
