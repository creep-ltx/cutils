# kill - stop a program by sending it a break (Ctrl-C)

## SYNOPSIS
    kill [-C | -D | -E | -F | -s SIG] TARGET ...
    kill -l

## DESCRIPTION
Sends a break to a program, the same as pressing Ctrl-C in its window. A
program that checks for breaks (nearly all commands do) stops; one that
does not goes on. Unlike Break, kill also finds programs by name.

A TARGET may be:

    3           a Shell's number, as ps and Status show it: the
                break reaches the command running in that Shell
    0x0043f6a0  a task's address, from ps
    wait        a name: every task whose name, or whose Shell's
                command, is that name (case blind); patterns with
                #? or * work too, as pkill and killall do it

## OPTIONS
- **-C** Ctrl-C, the default. -INT, -TERM, -HUP, -2 and -15 are the
  same, for Unix habits.
- **-D** Ctrl-D (stops a script started with Execute).
- **-E** Ctrl-E.
- **-F** Ctrl-F.
- **-s** SIG the same by name: C, D, E, F, INT, TERM.
- **-l** list the signals that can be sent.

## EXAMPLES
    kill 3
    kill wait
    kill -D 4
    kill 0x0043f6a0
    ps wait
    kill #?player#?

## RETURN CODES
0 fine, 5 a target matched no program (or it was already gone), 10 bad
arguments.

## NOTES
The Amiga has no safe way to force a program out: its memory, files and
windows would stay taken and could crash the machine later. So kill only
asks, and -9 / KILL are refused with a word. kill never sends a break to
itself. In csh, `kill %1` and `kill` with a job's number go to csh's own
jobs; other targets are passed to this command.

## SEE ALSO
`ps`, Break, Status
