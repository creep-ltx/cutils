# head - show the first lines (or bytes) of files

## SYNOPSIS
    head [-n N | -N] [-c N] [-qv] [file | pattern | - ...]

## DESCRIPTION
Shows the beginning of each file: the first 10 lines, unless told
otherwise. Handy for a quick look into a long file, or to keep only the
top of another command's output.

When several files are shown, each comes under a line
`==> name <==`. With no file, or `-`, head reads its input.

head stops reading as soon as it has what it needs, so a pipe into head
ends at once, however big the input.

## OPTIONS
- **-n** N
  the first N lines.
- **-N** the same, shorter: `head -5` is `head -n 5`.
- **-c** N
  the first N bytes instead of lines. N may end in k or M (1024s).
- **-q** never show the file names.
- **-v** always show the file names, even for one file.

## EXAMPLES
    head S:Startup-Sequence
    head -5 Work:Docs/#?.txt
    head -c 512 DF0:mod.intro | cat -v
    ls -lt Work:Downloads | head -3
    sort -n -r Work:scores.txt | head -n 1

## RETURN CODES
0 fine, 5 a file could not be read or a pattern matched nothing,
10 bad arguments, 20 stopped with Ctrl-C.

## NOTES
Patterns may be written `#?` or `*`; upper and lower case are the same.

`head ?` shows the usage line.

## SEE ALSO
`tail`, `cat`, `wc`, Type
