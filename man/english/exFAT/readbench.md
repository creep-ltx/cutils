# readbench - read a file from start to end and show the speed

## SYNOPSIS
    ReadBench FILE [BUF BYTES]

    Template: FILE/A,BUF/N

## DESCRIPTION
ReadBench reads one file from the start to the end, as fast as it can,
and shows how fast it goes. It was made to test the exFAT file system
with very big files, so that slow spots, stalls and the crossing of the
4 GB mark can be watched while they happen.

It first says which file it reads and in what size of pieces. Then,
every 128 MB, it prints one line: the megabytes read so far, the speed
over the last stretch, and the average speed since the start (both in
KB a second). At the end it prints the total: megabytes read, seconds
taken and the average speed, and how the reading ended: at the clean
end of the file, or with a read error (the error is named).

Nothing is written; the file is only read.

## OPTIONS
- **FILE** the file to read. It must be given.
- **BUF** BYTES
  how much to read at a time, in bytes. The default is 131072 (128 KB).
  A value under 512 is ignored and the default is used.

## EXAMPLES
    ReadBench Work:big.iso
    ReadBench Work:backup.tar BUF 1048576

What it prints (the numbers are only an example):

    reading 'Work:big.iso' in 131072 byte chunks
         128 MB   last 128 MB: 10922 KB/s   total avg: 10922 KB/s
    ...
    done: 4500 MB read in 410 s (avg 11238 KB/s)
    ended with clean EOF (Read returned 0)

## RETURN CODES
0 the whole file was read to its end, 10 bad arguments, 20 anything
else: not enough memory for the buffer, the file could not be opened,
a read error, or stopped with Ctrl-C.

## NOTES
Ctrl-C stops the reading at once. ReadBench says "aborted by Ctrl-C"
and still prints the total.

Time is counted in whole seconds and sizes in whole megabytes, so the
speeds are rough for short runs. Use a big file.

The speed includes everything between ReadBench and the disk: the file
system, the device driver and the disk itself.

## SEE ALSO
DiskWatch
