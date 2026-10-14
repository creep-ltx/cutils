# df - how full the volumes (disks) are

## SYNOPSIS
    df [-h] [path ...]

## DESCRIPTION
Shows how much room there is on your disks. Each mounted volume gets a
line: its size, the space used and the space free, how full it is in
per cent (Use%, rounded up) and its name. Sizes are in KB (1024 bytes).

Name one or more paths to see only the volumes they are on. With no path,
every volume in a drive is shown. A disk that is not in a drive never
brings up an "insert volume" requester: df says so instead.

## OPTIONS
- **-h** sizes that are easy to read: 1.5K, 12K, 3.4M, 2.0G.

## EXAMPLES
    df
    df -h
    df -h Work: RAM:
    df SYS:C/List
    df -h | grep DH

## RETURN CODES
0 fine, 5 a path or volume could not be asked, 10 bad arguments,
20 stopped with Ctrl-C.

## NOTES
A volume that reports no size (some handlers do) shows `-` for Use%.

`df ?` shows the usage line.

## SEE ALSO
`du`, `ls`, Info
