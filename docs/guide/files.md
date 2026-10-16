# Files and drawers

ls, cp, mv, rm, mkdir, ln, touch, chmod, stat, file, find, du, df, basename, dirname.
Back to the [README](../../README.md).

![ls, ls -la, ls -lhS, du and df](../images/files.png)

## Amiga paths, Unix habits

The commands take what you would type on Linux, and the paths you type on the Amiga:

- **Paths** are Amiga paths: `Work:Projects/game.c`, `RAM:`, `/` for the parent
  drawer. The current drawer is the empty name `""`; there is no `.`.
- **Patterns**: `#?` and `*` both mean "anything", upper and lower case are the same.
  The commands match the patterns themselves, so they work in the ROM Shell as well as in csh.
- **Protection bits** are shown as List shows them, `hsparwed`: a letter where a bit
  is set (h s p a) or an action is allowed (r w e d).
- **Options** may come after the names (`ls Work: -la`); `--` ends them.
- A disk that is not in its drive gives a message, never an "insert volume" requester.

## Careful by default

| | |
|---|---|
| cp, mv | never replace a file that is there: they skip it and list it under "not copied:" / "not moved:" at the end; `-f` to replace |
| cp | keeps protection bits, date and filenote, as Copy CLONE; a copy that breaks off (full disk, Ctrl-C) is deleted |
| mv | between volumes a drawer moves whole or not at all: copied first, the original deleted only when every file copied |
| rm | a drawer only with `-r`; a delete-protected file only with `-f`; a volume or assign never (`rm -rf DH0:` deletes nothing); a link as a link |
| mkdir | names as typed, never patterns |

## Seeing what is there

**ls** lists in columns that fit the window, in natural order (`file9` before
`file10`), and colours each name by its kind: drawers, links, programs, scripts,
archives, pictures, sounds, sources, text, icons, hidden files. The colours are in
`ENV:FileColours` (written with the defaults the first time); Ahiru and Maki read the
same file. Into a pipe or a file, ls writes one name a line, no colours.

    ls -la Work:            icons and hidden files too, long form
    ls -lhS SYS:Libs        biggest first, sizes in K and M
    ls -lt Work:Downloads   newest first
    ls -1 RAM: | wc -l      how many

**file** looks inside, not at the name: AmigaOS programs and libraries, icons (with
their kind), IFF pictures and sounds with their size and rate, MODs with channels and
title, ADF images by their bootblock, LhA, LZX, Zip, gzip, tar and more, PNG, GIF, JPEG,
and text by its character set.

![file and tar](../images/archives.png)

**stat** shows everything about one file: type (also soft and hard links, and where they
point), size and blocks, bits, date with the weekday, filenote.

**du** and **df**: how big a tree is (drawer by drawer, the whole last), how full each
volume is. Both take `-h`.

## Finding

**find** walks trees as GNU find does: `-name`, `-type`, `-size`, `-mtime`, `-newer`,
`-empty`, with `!`, `-o` and `( )`, and the actions `-print`, `-exec`, `-delete`. Links
are never followed, and its list of drawers is kept in memory, so no tree is too deep.

    find Work: -name "#?.bak" -delete
    find Work:Projects -name "#?.c" -newer Work:Projects/game -print
    find SYS: -type f -size +100k | xargs ls -lh

On the Amiga an unquoted `;` starts a comment: after `-exec` write `";"` or `\;`, or end
the line there.

## Changing

**chmod** writes the bits the Unix way (`+x`, `go-r`, `755`) or the Amiga's (`+s`,
`+p`, `=rwed`). `w` means write and delete, since AmigaOS has a bit for each.

**ln** makes hard links by default (as Unix), `-s` for soft ones; a hard link never
crosses volumes. **touch** dates a file now (`-d` a date, `-r` another file's), and
makes it when it is missing.

**basename** and **dirname** work on the text alone, the Amiga way: `dirname Work:a/b`
gives `Work:a`, `dirname file` gives the empty name, so the result always works with CD.

Every option of every command: its manual page, `man NAME`.
