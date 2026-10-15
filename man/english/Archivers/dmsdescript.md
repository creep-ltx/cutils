# DMSDescript - add, take out or remove the description of a .dms file

## SYNOPSIS
    DMSDescript a DESCRIPTIONFILE ARCHIVE.dms
    DMSDescript x DESCRIPTIONFILE ARCHIVE.dms
    DMSDescript d ARCHIVE.dms

## DESCRIPTION
A .dms disk archive can carry a short description of what is on the disk,
of the kind bulletin boards (BBSes) show in their file lists (a
FILE_ID.DIZ). DMSDescript manages that description without unpacking or
repacking the disk:

- **a** add the text in DESCRIPTIONFILE to the archive as its
  description.
- **x** extract the archive's description into DESCRIPTIONFILE.
- **d** delete the description from the archive.

It checks that the file really is a DMS archive and reports checksum
errors in it.

## EXAMPLES
    DMSDescript a RAM:file_id.diz Work:Upload/demo.dms
    DMSDescript x RAM:desc.txt Work:Upload/demo.dms
    DMSDescript d Work:Upload/demo.dms

## NOTES
DMSDescript 1.1 (January 1994) by ReMo of Mystic and X-Design (the version
string spells it with slashes, `Re/Y\o`), a tool from the Amiga BBS scene.
No documentation was found; this page is written from the program's usage
line, `<a|x|d> [<Description>] <DMS Archive>`, and its messages. That the
description is a file, and the meanings of a, x and d, are taken from
PKDDescript (Aminet `util/arc/PKDDescript`), a later tool for PackDev
archives that "uses the same arguments" and documents them; not tested
with DMSDescript itself.

## SEE ALSO
`DMS`, `UnARJ`
