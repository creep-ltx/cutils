# which - find where a command comes from (replaces C:Which)

## SYNOPSIS
    which [-a] name ...
    which FILE/A,NORES/S,RES/S,ALL/S

## DESCRIPTION
Tells where the Shell would find a command when you type its name. It
takes the place of the Amiga's own C:Which, so the Amiga form with its
keywords still works, and it also takes the Unix form.

It looks where the Shell looks, in the same order: the resident list,
the current drawer, the command path (as set with Path), and C:. The
first place that has the command is printed:

    RES Assign              made resident (Resident)
    INTERNAL cd             built into the Shell
    Workbench:C/List        a file: its full name

A file counts only when it has the e (execute) bit, as the Shell needs.
A name with a `:` in it is looked up just as it is; a volume or assign
(`C:`) gives its drawer. Nothing is printed for a name that is not
found. Several names are looked up one after another.

## OPTIONS
- **-a** every place that has the command, not only the first. The same
  file may show twice, through C: and through the path.

## AMIGA FORM
The Amiga template is `FILE/A,NORES/S,RES/S,ALL/S`. The keywords may be
mixed with the Unix form, in upper or lower case:
- **FILE** the keyword may stand before the name (`FILE List` or
  `FILE=List`), or be left out.
- **NORES** do not look in the resident list.
- **RES** look only in the resident list.
- **ALL** every place that has it, as **-a**.

As with the Amiga's Which 45.2, with ALL a command that is only
internal (built into the Shell) gives return code 5: then only a
resident command or a file counts. NORES with RES finds nothing (5).
The Amiga's Which takes one name; this one takes several.

## EXAMPLES
    which List
    which -a ls
    which dir NORES
    which ALL Sort
    which -a sort which date

## RETURN CODES
0 found, 5 not found (WARN, as the Amiga's; with several names, when
one of them was not found), 10 bad arguments, 20 stopped with Ctrl-C.

## NOTES
In a script, `which name >NIL:` followed by `IF WARN` tests whether a
command is there.

`which ?` shows the usage line.

## SEE ALSO
`find`, `ls`, `stat`, Which, Path, Resident
