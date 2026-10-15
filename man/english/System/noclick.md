# NoClick - stop the empty floppy drives from clicking

## SYNOPSIS
    NoClick

## DESCRIPTION
An Amiga floppy drive with no disk in it clicks every few seconds, because
the system keeps checking for a new disk. trackdisk.device (version 36 and
later, AmigaOS 2.0 on) has a NoClick setting that avoids this on drives
that support it. NoClick switches that setting on for every floppy unit
there is, and ends. It takes no arguments.

If a drive still clicks afterwards, do not use NoClick with it: its author
warns that it may then damage the drive's mechanics.

## EXAMPLES
    NoClick

On this A1200 it runs from S:Startup-Sequence as `run >nil: c:noclick`.

## NOTES
NoClick 1.1 (2016) by Patrik Axelsson, Aminet `disk/misc/NoClick`
(github.com/patrikaxelsson/NoClick). Version 2.0 there can also be made
resident with a LoadModule-style tool, so it works even when booting
without the startup-sequence.

## SEE ALSO
`PsdLoadModule`
