# chmod - change protection bits, in Unix words or Amiga letters

## SYNOPSIS
    chmod [-Rvc] MODE file | pattern ...

## DESCRIPTION
Changes the protection bits of files and drawers, as Protect does, but
with the Unix way of writing them (`+x`, `go-r`, `755`) as well as the
Amiga's own letters (`+s`, `+p`, `=rwed`).

MODE is one or more changes joined with commas. Each change is an
optional *who*, then `+` (turn on), `-` (turn off) or `=` (set exactly),
then letters. Or MODE is an octal number such as `755`.

The letters:
- **r** read.
- **w** write, and delete too: on AmigaOS deleting has its own bit, so
  Unix's w means both.
- **x** or **e** execute.
- **d** delete alone.
- **s** script, **p** pure (may be made Resident), **a** archived,
  **h** hold (bit 7).

Who:
- **u** the owner: the bits AmigaOS itself uses. This is also what is
  changed when no who is given.
- **g** group, **o** other: kept by the file system for networks and
  MultiUser.
- **a** all three.

`=` sets the owner's r w e d and the h s p a bits to exactly the letters
given, as Protect does; with g or o only theirs. In octal the first digit
is the owner (4 read, 2 write and delete, 1 execute), then group and
other; the h s p a bits are kept as they were.

## OPTIONS
- **-R** also everything inside drawers, all the way down. Links are
  left as they are.
- **-v** say what happened to each file: `file: ----rwed -> ----rw-d`.
- **-c** like **-v**, but only for files that really changed.

## EXAMPLES
    chmod +x Work:Tools/MyProg
    chmod +s S:MyScript
    chmod -w Work:Docs/Important.txt
    chmod =rwed RAM:#?
    chmod -R -c -a Work:Projects
    chmod 755 Work:Tools/MyProg

## RETURN CODES
0 fine, 5 a pattern matched nothing or a soft link was left alone,
10 a bit could not be set or the MODE is wrong, 20 stopped with Ctrl-C.

## NOTES
A MODE starting with `-` is read as a mode, not an option, so
`chmod -w file` works as it does on Unix. Options must come before the
MODE.

The protection bits as List shows them: `hsparwed`. A letter shows a bit
that is on (h s p a) or allowed (r w e d); `-` shows it off.

A soft link named on the command line is not changed (rc 5); with **-R**
links inside drawers are skipped silently.

Patterns may be written `#?` or `*`; upper and lower case are the same.

`chmod ?` shows the usage line.

## SEE ALSO
`ls`, `stat`, `find`, Protect, List
