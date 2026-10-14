# cut - pick fields, characters or bytes out of each line

## SYNOPSIS
    cut -f LIST [-d SEP] [-s] | -c LIST | -b LIST [--complement] [file ...]

## DESCRIPTION
Prints only some parts of each line: fields (columns split by a
separator, a tab unless told otherwise), or characters at given
positions. Useful for taking one column out of a table or a list.

LIST is one or more numbers counted from 1, joined with commas:
- **N** just that one.
- **N-M** from N to M.
- **N-** from N to the end of the line.
- **-M** from the start to M.

The parts may be given in any order; they always come out in the order
they have in the line.

With no file, or `-`, cut reads its input, so it can sit in a pipe.

## OPTIONS
- **-f** LIST
  the fields in LIST. They are put out joined by the same separator.
  A line without any separator comes out whole (see **-s**).
- **-d** SEP
  the field separator, one character (`\t` is a tab). The default is a
  tab.
- **-s** leave out lines that have no separator.
- **-c** LIST
  the characters in LIST.
- **-b** LIST
  the bytes in LIST. The same as **-c**: Latin-1 has one byte per
  character.
- **--complement**
  everything except what LIST names.
- **--output-delimiter=**S
  join the fields with the text S instead of the separator.

## EXAMPLES
    cut -d : -f 1 S:Startup-Sequence
    cut -c 1-20 Work:Docs/list.txt
    List RAM: LFORMAT "%N;%L" | cut -d ";" -f 2
    cut -d , -f 2,4 Work:Data/table.csv
    cut -d , -f 1 --complement Work:Data/table.csv

## RETURN CODES
0 fine, 5 a file could not be read or a pattern matched nothing,
10 bad arguments, 20 stopped with Ctrl-C.

## NOTES
Only one of **-f**, **-c** and **-b** may be used at a time.

Files may be named with patterns (`#?` or `*`).

`cut ?` shows the usage line.

## SEE ALSO
`grep`, `sort`, `uniq`, `tr`, `wc`
