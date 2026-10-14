# cat - show files, or join them one after another

## SYNOPSIS
    cat [-bnsEv] [file | pattern | - ...]

## DESCRIPTION
Sends files to the output one after another, as they are. With one file
it shows that file in the window (as Type does); with several it joins
them, and with `>` the result goes into a new file.

With no file, or `-`, cat reads its input, so it can sit in a pipe. With
no options the bytes go through untouched, in big blocks, so it is fast
and safe for any kind of file.

## OPTIONS
- **-n** number every line.
- **-b** number only the lines that are not empty.
- **-s** squeeze: where there were several empty lines, show one.
- **-E** show a `$` at the end of each line (to see spaces at the end).
- **-v** show control characters as `^X` (Tab and the line end are kept),
  and characters 128 and up as `M-` followed by the character.

## EXAMPLES
    cat S:Startup-Sequence
    cat -n S:User-Startup
    cat RAM:part1 RAM:part2 RAM:part3 >RAM:whole
    cat Work:Docs/#?.txt | grep -i amiga
    cat -v -E Work:odd.txt

## RETURN CODES
0 fine, 5 a file could not be read or a pattern matched nothing,
10 bad arguments, 20 stopped with Ctrl-C.

## NOTES
Patterns may be written the Amiga way (`#?.txt`) or the Unix way
(`*.txt`); upper and lower case are the same. The names a pattern stands
for are taken in natural order (`file9` before `file10`).

Line numbers count on across all the files given.

`cat ?` shows the usage line.

## SEE ALSO
`head`, `tail`, `tee`, `wc`, Type, Join
