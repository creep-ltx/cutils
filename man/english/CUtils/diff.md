# diff - show the lines that differ between two files or drawers

## SYNOPSIS
    diff [-uqsiwbBNra] [-U N] [--color=WHEN] FROM TO

## DESCRIPTION
Compares two text files and shows which lines were changed, added or
taken away, to turn FROM into TO. Useful to see what changed between two
versions of a script, a configuration or a program's source.

The normal output is Unix diff's: a line such as `3c3` says where, then
`< ` before the old lines and `> ` before the new ones. With **-u** it
uses the unified form that patch and git use:

    --- a.txt   2026-10-14 18:36:41
    +++ b.txt   2026-10-14 18:40:02
    @@ -1,4 +1,4 @@
     context
    -old line
    +new line

In a window the lines are coloured as git does: removed lines red, added
lines green, `@@` lines cyan.

`-` as a name is the input (a pipe). A drawer and a file: the file of the
same name in that drawer is compared. Two drawers: the files in them are
compared, name by name, and what is only on one side is said
(`Only in D: name`).

A file with a 0 byte in its first 8 KB is taken as binary, and only
`Binary files A and B differ` is said (unless **-a**). A missing line
end at the end of a file is said: `\ No newline at end of file`.

## OPTIONS
- **-u** the unified form, with 3 lines of context.
- **-U** N
  the unified form with N lines of context.
- **-q** only say whether the files differ: `Files A and B differ`.
- **-s** also say when they are the same: `Files A and B are identical`.
- **-i** upper and lower case are the same.
- **-w** ignore all white space (spaces and tabs).
- **-b** ignore changes in the amount of white space, and white space at
  line ends.
- **-B** ignore changes that are only empty lines (with **-b** or **-w**
  also lines of only white space).
- **-a** treat every file as text.
- **-r** with two drawers: compare all the way down into the drawers
  inside. Without it, only the files in the two drawers are compared and
  drawers both have are just named.
- **-N** with drawers: a file that is only on one side is compared with an
  empty file, so its whole text is shown.
- **--color=**WHEN
  `always` colours also into a file or a pipe, `never` not at all,
  `auto` (the default) only in a window. `--color` alone is `always`;
  `--colour` works too.

## EXAMPLES
    diff S:Startup-Sequence S:Startup-Sequence.bak
    diff -u Work:old/game.c Work:new/game.c
    diff -u Work:old/game.c Work:new/game.c >RAM:game.patch
    diff -rq Work:Projects DH1:Backup/Projects
    diff -i -w S:User-Startup RAM:User-Startup
    List SYS:C | diff - RAM:list-before

## RETURN CODES
0 the files are the same, 5 they differ, 10 trouble (a file that could
not be read, bad arguments), 20 stopped with Ctrl-C.

## NOTES
On Unix, diff returns 1 when the files differ. On the Amiga 5 is the
"not quite" code (WARN), so a script can test it with `IF WARN`, as with
grep's "no match".

With **-r**, links to drawers are not gone into.

diff is quick on ordinary files: two files of 10000 lines with 200
changes take about 4 seconds on an A1200 (68020). Very different files
are cut short with a shortcut rather than taking minutes.

`diff ?` shows the usage line.

## SEE ALSO
`grep`, `wc`, `sort`, `cut`, Search
