# The guide

The commands by group, with examples and pictures. The full list is in the
[README](../../README.md); every option of every command is in its manual page
([man/english/CUtils](../../man/english/CUtils), or `man NAME` on the Amiga).

| Page | Commands |
|---|---|
| [Files and drawers](files.md) | ls cp mv rm mkdir ln touch chmod stat file find du df basename dirname |
| [Text](text.md) | cat head tail grep sed cut tr sort uniq wc diff tee xargs seq |
| [Archives and packing](archives.md) | tar gzip gunzip bzip2 bunzip2 unxz unzstd |
| [The system](system.md) | ps kill top ctop free uptime date which |
| [The network](network.md) | wget |
| [The manual](man.md) | man |

## The pictures

They are in [docs/images](../images), made in FS-UAE: a 3.2.1 A1200 as a 68030
(`amiga-test a1200-32-mui5 --cpu 030`), CTerm on its own 256-colour screen with the Nord
theme, csh as the Shell, topaz 8. To make them again after a command's output changes:

    tests/shots/all.sh                    every picture (starts the rig, port 1255)
    tests/shots/shot.sh NAME WAIT CMD...  one picture, on a rig that is up
    tests/shots/rig.sh stop

`rig.sh` is CTerm's test rig (`~/Projects/CTerm/tests/rig.sh`) with this tree's `bin/`
and pages added; `demo.sh` makes the files the pictures show. Each `shot.sh` types the
commands in a new CTerm, grabs the screen with wasabi, cuts off the empty rows, and ends
CTerm. man's picture is taken last, because ending CTerm with man open resets the Amiga
([TODO.md](../../TODO.md), entry 2); `shot.sh` stops when a Software Failure requester is up.
