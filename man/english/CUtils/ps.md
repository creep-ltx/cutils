# ps - list the programs and tasks that run

## SYNOPSIS
    ps [-elw] [name | pattern ...]

## DESCRIPTION
Shows every Shell (CLI) process and the command it runs now, as Status
does, but easier to read. With **-e** it shows every task and process on
the machine: devices, handlers, Workbench, everything. Names or patterns
keep only the tasks whose name or command matches.

The columns:

    CLI       the Shell's number (kill takes it); - for other tasks
    ADDRESS   the task in memory (kill takes 0x... too)
    PRI       its priority
    STATE     run (the one running: ps itself), ready (waiting for
              the CPU) or wait (waiting for something to happen)
    COMMAND   the command the Shell runs now, or [the task's name]
              when it runs nothing

## OPTIONS
- **-e** every task and process, not only the Shells (also -a).
- **-l** long: also the stack's size, how much of it is still free
  (a program that runs out of stack crashes the machine), and the
  signals the task waits for.
- **-w** wide: names and commands are not cut to the window.

## EXAMPLES
    ps
    ps -e
    ps -el
    ps wait
    ps #?shell#?

## RETURN CODES
0 fine, 5 a name or pattern matched nothing, 10 bad arguments.

## NOTES
The Amiga keeps no account of how much processor time each task uses,
so there is no CPU column. A program that ran out of stack shows little
FREE in `ps -l` before it crashes. The list is copied with the system
held still for a moment, so it is always consistent.

## SEE ALSO
`kill`, `free`, `uptime`, Status, Break
