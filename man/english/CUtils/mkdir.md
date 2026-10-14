# mkdir - make new drawers (directories)

## SYNOPSIS
    mkdir [-p] drawer ...

## DESCRIPTION
Makes one new drawer for each name given, as MakeDir does.

Without **-p** each drawer is made directly: the drawer it goes in must
exist already, and a drawer that is there already is an error, as on
Unix.

## OPTIONS
- **-p** make every missing drawer along the path: `mkdir -p a/b/c`
  makes a, a/b and a/b/c. A drawer that is there already is fine. A part
  of the path that is a file is an error, and that part is named. The
  volume (the part before `:`) is never made, only gone into.

## EXAMPLES
    mkdir RAM:Work
    mkdir -p Work:Projects/Game/gfx
    mkdir RAM:T1 RAM:T2 RAM:T3
    seq -f RAM:T%d 1 3 | xargs mkdir

## RETURN CODES
0 fine, 10 a drawer could not be made (the others still are),
20 stopped with Ctrl-C.

## NOTES
Names are taken as typed, never as patterns: `mkdir "#?"` makes a
drawer called `#?`.

No icon (.info) is made for the new drawer; Workbench shows it with
"Show All Files".

`mkdir ?` shows the usage line.

## SEE ALSO
`rm`, `cp`, `mv`, `ls`, MakeDir
