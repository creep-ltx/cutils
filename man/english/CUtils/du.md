# du - how much space the files in a drawer take, drawer by drawer

## SYNOPSIS
    du [-achs] [-d N] [path | pattern ...]

## DESCRIPTION
Adds up the sizes of the files in a tree. Every drawer under each path
gets a line with its size and its path; the drawers inside come before
the drawer that holds them, so the last line is the whole tree. A path
that is a file gets its own line. With no path, the current drawer is
used (shown as "").

Sizes are in KB (1024 bytes). Each file counts its length rounded up to a
whole KB (an empty file counts 0); drawers and links count 0. So the
numbers show what the files hold: close to, but not exactly, the blocks
they use on the disk.

## OPTIONS
- **-a** also a line for each file, not only for drawers.
- **-s** only one line for each path: its total (the same as **-d 0**).
- **-d** N
  lines only for drawers down to N levels below the path. The sizes still
  count everything below.
- **-h** sizes that are easy to read: 1.5K, 12K, 3.4M, 2.0G (rounded up).
- **-c** a last line with the grand total of all paths: `N  total`.

## EXAMPLES
    du -s -h Work:Projects
    du -d 1 -h SYS:
    du -a RAM:
    du -s -c Work:Games/#?
    du -d 1 Work: | sort -n

## RETURN CODES
0 fine, 5 something could not be read or a pattern matched nothing,
10 bad arguments, 20 stopped with Ctrl-C.

## NOTES
Links, soft or hard, are never gone into, so a file is not counted twice.

Patterns may be written `#?` or `*`; upper and lower case are the same.

Very deep trees are no problem: du does not use up the Shell's stack.

`du ?` shows the usage line.

## SEE ALSO
`df`, `ls`, `find`, `sort`, List
