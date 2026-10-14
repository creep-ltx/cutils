# tee - copy the input to the output and into files

## SYNOPSIS
    tee [-a] [file ...]

## DESCRIPTION
Passes what comes in through to the output unchanged, and writes it into
each file named as well. Like a T-piece in a pipe: you can save what
goes through a pipe and still see it, or pass it on.

    List | tee RAM:list | grep foo

## OPTIONS
- **-a** add to the end of the files instead of replacing them.
- **-i** accepted (GNU's "ignore Ctrl-C") but does nothing.

## EXAMPLES
    List SYS:C | tee RAM:c-list.txt
    dir Work: | tee RAM:a RAM:b | wc -l
    date | tee -a RAM:log.txt
    find Work: -name #?.c | tee RAM:sources | xargs grep -l main

## RETURN CODES
0 fine, 10 a file could not be opened or written (the others go on),
20 stopped with Ctrl-C.

## NOTES
At most 32 files at a time.

`tee ?` shows the usage line.

## SEE ALSO
`cat`, `head`, `tail`, `xargs`
