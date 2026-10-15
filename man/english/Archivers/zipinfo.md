# zipinfo - list what is in a .zip archive, in detail

## SYNOPSIS
    zipinfo [-1smlvht] zipfile[.zip] [file ...]

## DESCRIPTION
Lists the contents of a .zip archive in the style of Unix `ls -l`:
protection, versions, sizes, packing method, dates and names, more or less
of them depending on the option. It never unpacks anything. With file
names (patterns work) only those entries are listed.

## OPTIONS
- **-1** only the names, one a line (for use in scripts).
- **-s** short `ls -l` style lines (the default).
- **-m** medium: also the packing ratio.
- **-l** long: also the packed size.
- **-v** everything that is stored about every entry, over many lines.
- **-h** a header line with the archive's name, size and file count.
- **-t** totals for the files listed.

## EXAMPLES
    zipinfo archive.zip
    zipinfo -1 archive.zip
    zipinfo -v archive.zip readme.txt
    unzip -Z -l archive.zip

## NOTES
ZipInfo 1.0 (August 1992) by Newtware and Info-ZIP, an old Amiga build. It
needs **ixemul.library** version 39 or newer, which was not found on this
A1200 (checked 2026-10-15), so it does not start here. `unzip -Z` gives
the same listings without ixemul: UnZip 5.1 has a zipinfo mode built in.

## SEE ALSO
`unzip`, `zip`
