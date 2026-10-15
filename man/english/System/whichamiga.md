# whichamiga - report what Amiga this is: model, chips, ROM, boards

## SYNOPSIS
    whichamiga

## DESCRIPTION
Examines the machine and prints a report: the Amiga model, CPU and FPU,
the custom chips (Agnus/Alice, Denise/Lisa, Paula), chip and fast memory,
the Kickstart ROM and Workbench versions, SetPatch, the expansion boards
found (accelerators, graphics and sound cards, with names where it knows
them), PowerPC boards, and whether it runs under UAE or MorphOS. It takes
no arguments.

To find all that it reads the custom chip registers and the hardware
directly.

## EXAMPLES
    whichamiga

## NOTES
WhichAmiga 1.3.25 (2007) by Harry "Piru" Sintonen. No documentation was
found; this page is written from the messages in the program.

**Do not run it on this A1200.** Under PiStorm/Emu68 its direct hardware
probing once froze the machine completely (no network, no input), and only
a reset by hand brought it back. For Emu68 machines use `Emu68Info`
instead.

## SEE ALSO
`Emu68Info`, `lspci`, `Version`
