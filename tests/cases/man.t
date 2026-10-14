$ man -l tman.man
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
$ MakeDir >NIL: {T}/Man {T}/Man/english {T}/Man/english/Test
$ Copy tman.man {T}/Man/english/Test/ QUIET
$ Assign MAN: {T}/Man
$ man -w tman
{TV}/Man/english/Test/tman.man
$ man tman | head -n 2
NAME
    tman - a test page for man, with every mark
$ man -k "every mark"
tman          - a test page for man, with every mark
$ man -L svenska -w tman
{TV}/Man/english/Test/tman.man
$ man -w sort
AmigaOS3.2:Locale/Help/english/Sys/Commands/sort.help
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
