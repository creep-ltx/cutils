# StackAttack - give every new program at least a minimum stack

## SYNOPSIS
    StackAttack [MINSTACK BYTES] [ADDSTACK BYTES] [AUTOSTACK] [NOTCLI]
                [SHOW]

## DESCRIPTION
Each Amiga program gets a fixed amount of stack, and many get too little -
especially when system patches and commodities make the system calls they
use need more. Too little stack crashes the machine in odd ways.
StackAttack patches the system functions that start new processes
(CreateProc, CreateNewProc, RunCommand, SystemTagList) so that each new
process gets at least MINSTACK bytes.

The patch stays in memory with a named port, so running StackAttack again
only changes its values; it does not add another patch.

With no arguments it sets a minimum of 8192 bytes.

## ARGUMENTS
- **MINSTACK** BYTES
  the smallest stack a new process gets. **MIN** for short.
- **ADDSTACK** BYTES
  add this much to every stack as well. Kept for the older StackAttack;
  its author advises against it unless memory is very tight. **ADD**.
- **AUTOSTACK** choose the minimum by the amount of memory: 8 KB, or
  16, 32, 64 or 128 KB with more than 16, 32, 64 or 128 MB. **AUTO**.
- **NOTCLI** do not also set the Shell's own stack. By default
  StackAttack sets it too, as the `Stack` command would.
- **SHOW** print the current values.

## EXAMPLES
    StackAttack
    StackAttack AUTO
    StackAttack MIN 16384
    StackAttack SHOW

The installation suggests `StackAttack` right after SetPatch in
S:Startup-Sequence and `StackAttack AUTO` just before LoadWB; this A1200
does exactly that.

## NOTES
StackAttack 2.9 (2004) by Robin Cloutman, a rewrite of Georg Steger's
StackAttack 1; Aminet `util/boot/StackAttack2`.

## SEE ALSO
`Stack`
