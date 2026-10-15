# UnARJ - take the file_id.diz out of an .arj archive

## SYNOPSIS
    UnARJ archive[.arj]

## DESCRIPTION
In spite of its name this UnARJ does one thing only: it looks in an ARJ
archive for a file called file_id.diz and unpacks just that, into the
current drawer. Nothing else in the archive is touched.

file_id.diz is the short description that uploaders put into archives for
bulletin boards (BBSes), which read it to describe the file in their
lists.

To unpack a whole .arj archive, use `ARJ` (UNARJ 2.41).

## EXAMPLES
    CD RAM:
    UnARJ Work:Upload/program.arj
    Type RAM:file_id.diz

## NOTES
UnArj_FID 1.02 (1996-97) by Florian Heer, made from Robert K. Jung's UNARJ
2.41a source with his permission; public domain. Aminet
`util/pack/UnArj_FID`. It calls itself a special version for "MOS"; its
readme does not say more about that.

## SEE ALSO
`ARJ`, `DMSDescript`
