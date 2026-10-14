# seq - print a sequence of numbers (count from FIRST to LAST)

## SYNOPSIS
    seq [-w] [-s SEP] [-f FORMAT] [FIRST [INC]] LAST

## DESCRIPTION
Counts from FIRST to LAST, one number a line. FIRST and the step INC
are 1 unless given. INC may be negative to count down: `seq 10 -2 1`.

Decimals work too: `seq 0 0.25 1` gives 0.00 0.25 0.50 0.75 1.00, every
number with as many decimals as the most precise one typed. Negative
numbers are numbers, not options (`seq -3 3`). When FIRST is already
past LAST, nothing is printed.

seq is handy in scripts and pipes, to number things or to run something
a set number of times with `xargs`.

## OPTIONS
- **-w** equal width, padded with leading zeros: 008 009 010.
- **-s** SEP
  put SEP between the numbers instead of a new line. `\t` and `\n` in
  it are a tab and a new line, `\\` a backslash.
- **-f** FORMAT
  printf-like: one `%d`, `%f` or `%g` (also `%i`, `%F`, `%G`), with the
  flags `0`, `-`, `+`, a width and a .precision (`%03d`, `%.2f`,
  `%5g`). Text around it is printed as it is; `%%` is a %.

## EXAMPLES
    seq 5
    seq 10 -1 1
    seq -w 1 100
    seq -s ", " 1 10
    seq -f "RAM:part%02d" 1 5 | xargs mkdir
    seq 0 0.5 3

## RETURN CODES
0 fine, 10 bad arguments (not a number, a step of 0, a bad FORMAT),
20 stopped with Ctrl-C.

## NOTES
The last number always ends with a new line, also with **-s**.

Inside quotes the Shell also turns `*n` into a new line, so
`-s "*n"` works as well.

`seq ?` shows the usage line.

## SEE ALSO
`xargs`, `sort`, `wc`
