# zip - pack files into a .zip archive (Info-ZIP Zip 2.0.1)

## SYNOPSIS
    zip [-options] [-b drawer] [-t mmddyy] [-n suffixes] zipfile
        [file ...] [-x pattern ...] [-i pattern ...]

## DESCRIPTION
Makes .zip archives, the format used on Windows, Linux and the Mac, or
adds files to one, replaces them, or deletes them. Without options it adds
the files named, replacing entries of the same name. `-` as a file name
packs the standard input; with no zipfile and no files, zip packs its
input to its output.

The archive gets the .zip ending if it has none. Drawers are only entered
with **-r**.

## OPTIONS
- **-r** take drawers with everything in them.
- **-j** store only the file names, not their paths.
- **-u** update: only files that are new or have changed.
- **-f** freshen: only replace changed files already in the archive.
- **-d** delete the named entries from the archive.
- **-m** move: delete the files after packing them.
- **-0** store only; **-1** pack faster; **-9** pack best.
- **-q** quiet; **-v** tell more.
- **-T** test the new archive.
- **-x** PATTERN ...
  leave out files that match.
- **-i** PATTERN ...
  take only files that match.
- **-n** SUFFIXES
  do not pack files with these endings (store them), for example
  `.zip:.lha:.jpg`.
- **-t** MMDDYY
  only files dated on or after that day.
- **-D** do not add entries for drawers.
- **-c** ask for a one-line comment for each file.
- **-z** ask for a comment for the whole archive.
- **-l** turn LF line ends into CR LF (for text going to DOS);
  **-ll** the other way.
- **-k** make the archive as PKZIP on DOS would (8.3 upper case names).
- **-o** give the archive the date of its newest entry.
- **-g** grow an existing archive instead of writing a new one.
- **-b** DRAWER
  where to make the temporary file.
- **-@** read the names of the files to pack from the input.
- **-F** try to repair a damaged archive; **-FF** try harder.
- **-h** the help text; **-L** the licence.

## EXAMPLES
    zip RAM:docs Work:Docs/#?.txt
    zip -r RAM:project.zip Project
    zip -r -9 RAM:project.zip Project -x #?.o
    zip -d RAM:project.zip Project/old.c
    zip -u RAM:project.zip Project/#?

## NOTES
Zip 2.0.1 (1993) from Info-ZIP (Mark Adler, Richard B. Wales, Jean-loup
Gailly, John Bush, Paul Kienitz and others), an old Amiga build. Its
archives open with every unzip; being from 1993 it cannot store files of 4
GB or more. `zip -h` prints the short help this page is based on.

## SEE ALSO
`unzip`, `zipinfo`, `lha`, `tar`
