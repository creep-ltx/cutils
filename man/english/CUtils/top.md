# top - show the tasks that use the processor, refreshed every second

## SYNOPSIS
    top [-b] [-d SECONDS] [-n COUNT]

## DESCRIPTION
The classic Unix top, inside the terminal: a CTerm tab or any console
window. At the top of the window:

    top - 21:45:20 up 1:04, 8 shells     the time, the time since the
                                         start, the Shells open
    Tasks: 42 total, 1 running, 2 ready, 39 waiting
    %Cpu: 12.0 busy, 88.0 idle           over the last interval
    Chip: 1.9M total, 95K used, 1.8M free, 1.8M largest
    Fast: 8.0M total, 3.5M used, 4.4M free, 3.6M largest

On a PiStorm with Emu68 the %Cpu line also shows how many million 68k
instructions ran each second (MIPS). The largest block is the biggest
single piece of memory a program can get.

Below that, every task on the machine, the busiest first:

    CLI       the Shell's number (kill takes it); - for other tasks
    ADDRESS   the task in memory
    PRI       its priority
    STATE     run (top itself), ready (waiting for the processor) or
              wait (waiting for something to happen)
    CPU%      its share of the processor over the last interval
    TIME+     the processor time it has used since top started, in
              minutes:seconds.hundredths (- when it is sampled)
    STACK     the size of its stack
    FREE      how much of the stack is still free (? when unknown)
    COMMAND   the command the Shell runs now, or [the task's name]

Running and ready tasks are shown bold. When the output is a file or a
pipe, top prints one list as plain text and ends (`top | grep seq`).

## OPTIONS
- **-b** batch: plain text, no keys; one list (or **-n** of them), then
  the end.
- **-d** SECONDS the time between refreshes (1); tenths work too:
  `-d 0.5`.
- **-n** COUNT refresh that many times, then end.

## KEYS
    P              sort by CPU% (the default)
    M              sort by stack size (the Amiga keeps no memory
                   count per task)
    N              sort by name
    T              sort by state: running, ready, waiting
    c              the task's name instead of the command
    e              every task, or only the Shells
    Up Down        move the bar
    Shift+Up/Down  a page (PgUp PgDn, Home End on a PC keyboard)
    k              break (Ctrl-C) a task: type its CLI number or its
                   0x address, or press Return for the one under the bar
    space          refresh now
    d s            the time between refreshes
    h Help         the keys
    q Esc Ctrl-C   quit

In CTerm a click picks a row and the mouse wheel moves the bar. When the
window changes size, the list follows.

## EXAMPLES
    top
    top -d 5
    top -b -n 1
    top -b -n 3 -d 2 >RAM:load.txt
    top | grep -i wasabi

## RETURN CODES
0 fine, 10 bad arguments, 20 Ctrl-C in batch mode, no memory, no
timer.device, or the window could not be used as a terminal.

## NOTES
The Amiga keeps no account of the time each task uses, but Exec can be
asked to call a small routine each time a task gets the processor and
each time it loses it (the TF_LAUNCH and TF_SWITCH task hooks). top sets
these on every task and notes the time, to 1.4 microseconds, at every
switch. CPU% and TIME+ are therefore exact, also for short bursts that
a sampler would miss. Time when no task runs is idle.

What that means:

- A task started while top runs is hooked at the next refresh; until
  then its time counts as idle.
- Time spent in interrupts is counted for the task they interrupted.
- A task that ended within the interval is not listed; its time still
  counts as busy.
- A task that already had hooks of its own keeps them, and only one top
  at a time can use the hooks. Those tasks, and everything in a second
  top, are sampled instead: a small interrupt server notes the running
  task 50 times a second (60 on NTSC), so a short burst is missed or
  counted as a whole fiftieth of a second. Their TIME+ shows -.
- top itself takes a few percent on a 68020 to draw the list.

The hooks and the sampler are taken out again whenever top ends, also
after Ctrl-C.

## SEE ALSO
`ps`, `kill`, `free`, `uptime`
