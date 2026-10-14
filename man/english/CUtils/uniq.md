# uniq - show repeated lines once, or count them

## SYNOPSIS
    uniq [-cdui] [-f N] [-s N] [-w N] [IN [OUT]]

## DESCRIPTION
Reads lines and writes each run of equal lines that follow each other
only once. Equal lines that are apart are not seen as repeats, so sort
the lines first when you want every duplicate gone:
`sort Work:names.txt | uniq`.

With **-c** it counts how often each line came, which together with
sort makes a quick tally.

IN is the file to read; without it, or with `-`, uniq reads its input.
When OUT is given, the result is written to that file instead of shown.

## OPTIONS
- **-c** put the count in front of each line.
- **-d** only the lines that came more than once.
- **-u** only the lines that came once.
- **-i** small and capital letters are the same (Latin-1 too).
- **-f** N
  the first N fields (blanks, then non-blanks) do not count when
  comparing.
- **-s** N
  nor do the first N characters (after those fields).
- **-w** N
  only the first N characters (after those) count.

## EXAMPLES
    sort Work:names.txt | uniq
    sort Work:words.txt | uniq -c | sort -n -r | head
    sort Work:list.txt | uniq -d
    uniq -i Work:log.txt RAM:log-short.txt
    List SYS:C LFORMAT "%A" | sort | uniq -c

## RETURN CODES
0 fine, 5 IN could not be read, 10 bad arguments or OUT could not be
written, 20 stopped with Ctrl-C.

## NOTES
uniq takes one file at most and no patterns; to go through several,
join them first: `cat Work:a Work:b | sort | uniq`.

`uniq ?` shows the usage line.

## SEE ALSO
`sort`, `wc`, `cut`, `grep`
