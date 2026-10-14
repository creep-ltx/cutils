# free - show how much memory is free

## SYNOPSIS
    free [-h | -b | -k | -m]

## DESCRIPTION
Shows the memory of the Amiga in three rows: chip memory (the memory the
custom chips can see: screens, pictures, sound), fast memory (everything
else), and both together. For each: the size, what is in use, what is
free, and the largest free block.

The largest free block is the biggest single piece a program can get.
Memory broken into many small pieces is free but of little use: a
program that asks for one big block may fail even when "free" is high.

## OPTIONS
- **-k** kilobytes (1024 bytes), the default.
- **-m** megabytes.
- **-b** bytes.
- **-h** human: K, M or G, whichever fits.

## EXAMPLES
    free
    free -h
    free -m

## RETURN CODES
0 fine, 10 bad arguments.

## SEE ALSO
`ps`, `uptime`, Avail
