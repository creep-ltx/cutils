$ man -l tman.md
NAME
    tman - a test page for man, with every mark

SYNOPSIS
      tman [-ab] [-k POS1[,POS2]] FILE ...

DESCRIPTION
    This paragraph is written on three short lines that man joins into one and
    wraps to the width. bold and code and under and *stars* stay.

OPTIONS
    -a          the first option.
    -k POS1[,POS2]
                an option with an argument, and a text long enough to go on to
                a second line in an eighty column window, indented.
    --a-very-long-option-name
                a long term goes on its own line.
    - a plain item, no term.

EXAMPLES
      tman -a Work:
        deeper indented example

SEE ALSO
    man
? 0
$ man -l troff.1
NAME
    troff - a test page in the Unix man format

SYNOPSIS
    troff [-ab] [FILE...]

DESCRIPTION
    This paragraph is written on two lines, joined and wrapped. Bold words and
    underlined ones and inline bold, a dash - and quotes "like this".

    A second paragraph.

OPTIONS
    -a      the first option.

    --long, -l
            a long term goes on its own line.

    o   a bullet item

        inner   an indented item

  Examples
      troff -a Work:
        kept as it is

SEE ALSO
    man(1)
$ MakeDir >NIL: {T}/Man {T}/Man/english {T}/Man/english/Test
$ Copy tman.md troff.1 {T}/Man/english/Test/ QUIET
$ Assign MAN: {T}/Man
$ man -w tman
{TV}/Man/english/Test/tman.md
$ man tman | head -n 2
NAME
    tman - a test page for man, with every mark
$ man -k "every mark"
tman          - a test page for man, with every mark
$ man -w troff
{TV}/Man/english/Test/troff.1
$ man -k "Unix man format"
troff         - a test page in the Unix man format
$ man -L svenska -w tman
{TV}/Man/english/Test/tman.md
$ man -w sort
{SYS}Locale/Help/english/Sys/Commands/sort.help
$ man sort | head -n 2
NAME
        SORT - Alphabetically sorts the lines of a file.
$ man -k "lines of a file"
sort          - Alphabetically sorts the lines of a file.
$ man nothere
man: nothere: no page (man -k word searches)
? 5
$ man
man: which page? (man ? for help)
? 10
$ man -x
man: -x: unknown option
? 10
$ Assign MAN: REMOVE
$ man -l thash.md
NAME
    hang - a test page

DESCRIPTION
    #?.c files are C sources. #x too.

    A normal paragraph.
$ man -l tlong.md | tail -n 1
    After it.
