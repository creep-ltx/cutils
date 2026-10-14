# tman - a test page for man, with every mark

## SYNOPSIS
    tman [-ab] [-k POS1[,POS2]] FILE ...

## DESCRIPTION
This paragraph is written on
three short lines that man joins into one and wraps to the width.
**bold** and `code` and *under* and \*stars\* stay.

## OPTIONS
- **-a** the first option.
- **-k** POS1[,POS2] an option with an argument, and a text long enough
  to go on to a second line in an eighty column window, indented.
- **--a-very-long-option-name** a long term goes on its own line.
- a plain item, no term.

## EXAMPLES
    tman -a Work:
      deeper indented example

## SEE ALSO
`man`
