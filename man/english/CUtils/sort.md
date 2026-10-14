# sort - sort lines of text (replaces C:Sort)

## SYNOPSIS
    sort [-bdfhiMnrsuVcC] [-k POS1[,POS2]] [-t SEP] [-o FILE] [file ...]
    sort FROM [TO] [COLSTART n] [CASE] [NUMERIC] [LEXICOGRAPHIC]
         [DESCENDING] [SKIPBLANKS]

## DESCRIPTION
Puts lines in order. All the files given (with no file, or `-`, its
input) are read, sorted together, and written out. The plain order is
by character code, as GNU sort's in the C locale: capitals before small
letters. Options give number order, reverse order, sorting on a column
and more.

sort takes the place of the Amiga's own C:Sort, so the Amiga form with
FROM, TO and keywords still works; see AMIGA FORM below.

It is quick: a few MB of lines sort in seconds.

## OPTIONS
- **-n** numeric: by the number at the start (-12, 3.5); a line without
  a number counts as 0.
- **-g** the same as **-n**.
- **-h** human sizes: 2K before 1M (as `du -h` and `ls -lh` print).
- **-V** version or natural order: `file9` before `file10`.
- **-M** by month names, JAN to DEC.
- **-f** fold case: small and capital letters the same (Latin-1 too).
- **-d** dictionary: only blanks, letters and digits count.
- **-i** only printable characters count.
- **-b** blanks at the start do not count.
- **-r** reverse the order.
- **-u** unique: only one of each run of equal lines (equal as the keys
  see them).
- **-s** stable: equal lines stay in the order they came in. Without it,
  as GNU sort, the whole lines decide in the end.
- **-k** POS1[,POS2]
  sort on a key: from field POS1 to field POS2 (or the end of the
  line). A position is F or F.C, field F and character C, counted from
  1. Option letters may follow a position: `-k2n`, `-k1.3,1.5r`,
  `-k3,3fb`. With several **-k** the first decides and ties go on to
  the next. A key with no letters takes the options given on their own.
- **-t** SEP
  fields are split at SEP (`\t` is a tab). Without it, each field
  starts at a run of blanks.
- **-o** FILE
  write the result to FILE. It is written after everything is read, so
  FILE may be one of the inputs.
- **-c** check only: is the input in order? The first line out of order
  is told, and the return code is 5.
- **-C** the same as **-c**, without the message.
- **-S**, **-T** accepted (for scripts written for GNU sort) and ignored.

## AMIGA FORM
The Amiga template is

    FROM,TO,COLSTART/K,CASE/S,NUMERIC/S,LEXICOGRAPHIC/S,DESCENDING/S,
    SKIPBLANKS/S,BUFSIZE/K/N

sort uses it when any of these keywords is there (upper or lower case,
`KEY=value` too), or when exactly two names are given with no options:
`sort a b` sorts the file a into the file b, as C:Sort did. (One name
is the Unix way: it is sorted and shown.)

It then works as Sort 46.4 does (checked side by side):
- **FROM** the file to sort.
- **TO** the file to write; nothing is shown. Without TO the result is
  shown.
- **COLSTART** n
  compare from column n (1 is the start).
- **CASE** small and capital letters differ (without it they are the
  same).
- **NUMERIC** by the number at the start (blanks, -, digits); lines
  without one come first.
- **LEXICOGRAPHIC** a dictionary order: accents and case only break
  ties (CASE is then ignored).
- **DESCENDING** the whole order turned round.
- **SKIPBLANKS** skip blanks at COLSTART first.
- **BUFSIZE** ignored: memory grows as needed.

Equal lines stay in the order they came in. The end of a line sorts as
a new line would, so "ab" followed by a tab comes before "ab". Unlike
C:Sort, empty input is not remarked on, and numbers longer than 32 bits
still compare right.

## EXAMPLES
    sort Work:names.txt
    sort -n -r Work:scores.txt
    sort -u -f Work:words.txt -o Work:words.txt
    sort -t , -k 2n Work:Data/table.csv
    du -d 1 Work: | sort -n
    sort Work:names.txt Work:sorted.txt
    sort FROM Work:names.txt TO RAM:sorted.txt CASE

## RETURN CODES
0 fine, 5 a file could not be read or **-c** found a line out of order,
10 bad arguments or the output could not be written (in the Amiga form
also a file that could not be read), 20 out of memory or stopped with
Ctrl-C.

## NOTES
Files may be named with patterns (`#?` or `*`); all the matches are
sorted together.

A 0 byte ends a line's text.

`sort ?` shows the usage line.

## SEE ALSO
`uniq`, `cut`, `grep`, `wc`, `ls`, Sort
