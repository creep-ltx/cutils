# SFSobject - hide a file on SFS, or make it impossible to delete

## SYNOPSIS
    SFSobject OBJECT [HIDE | SHOW] [UNDELETEABLE | DELETEABLE]

## DESCRIPTION
Sets two attributes that only SFS (Smart Filesystem) has, on a file or
drawer:

A hidden object does not show up in directory listings, but can still be
used by typing its name.

An undeletable object cannot be deleted even when its protection bits
allow it; trying gives "object is in use".

## ARGUMENTS
- **OBJECT** the file or drawer. Needed.
- **HIDE** hide it.
- **SHOW** show it again. **UNHIDE** is another name.
- **UNDELETEABLE** make it impossible to delete. **UNDELABLE** for
  short.
- **DELETEABLE** make it deletable again. **DELABLE** for short.

HIDE and SHOW cannot be given together, nor DELETEABLE and UNDELETEABLE.
The .recycled drawer cannot be made deletable.

## EXAMPLES
    SFSobject Work:Private HIDE
    SFSobject Work:Private SHOW
    SFSobject DH0:C UNDELETEABLE

## NOTES
SFSobject 1.6 by Martin Steigerwald, 68k version by Jean-Marie Coat
(2007), GPL (Aminet `disk/misc/SFSobj1.6_68k`). Remember what you hid: a
hidden object is only found again by its exact name.

## SEE ALSO
`Protect`, `SFSquery`, `SFSconfig`
