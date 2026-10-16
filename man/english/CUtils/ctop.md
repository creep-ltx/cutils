# ctop - the processor, memory, disks, network and tasks, with graphs

## SYNOPSIS
    ctop [-d SECONDS]

## DESCRIPTION
btop for the Amiga, inside the terminal: a CTerm tab or pane, or any
console window. Four boxes, each shown or hidden with its number key:

    1 cpu    a history graph of how busy the processor is, a meter, the
             CPU (and Emu68's version on a PiStorm), 68k MIPS, the Pi's
             temperature, clock and throttling, memory in a line, the
             uptime; the clock and the refresh interval in its border
    2 mem    chip and fast memory: total, used, the largest block, a
             meter and a graph each; every volume with a meter
    3 net    bytes in and out a second, as a mirrored graph, and the
             totals, with the address in the border (the network stack's
             own counters: lwip-amiga and Roadshow have them)
    4 proc   every task: CLI, the command or [the task's name], priority,
             state, free stack, CPU% with a small graph of its last ten
             values, TIME+ (the time it used since ctop started)

ctop fits itself to the window. All four boxes show from about 70 x 24
(a Shell window on a PAL hires screen), and the cpu and proc boxes down
to 40 x 12. A bigger window or screen gets wider graphs and more rows.
In CTerm the graphs are braille dots in 256 colours. A plain console
window gets ASCII graphs and its own few colours.

    -d SECONDS   between refreshes (1; tenths too: -d 0.5)

## KEYS
Every key can be changed in ENVARC:ctop.keys, one KEY ACTION a line;
ctop writes it with the built-in keys when it is missing. h shows what
is bound.

    Up Down k j      select a task (PgUp PgDn, Shift+arrows, Home End g G)
    Return           details of the task (address, stack, signals...)
    t K              break it (Ctrl-C)
    s                send Ctrl-C, D, E or F
    N n              change its priority
    Left Right       sort by another column (cli name pri state cpu time stack)
    r                reverse the order
    f /              show only the tasks whose name has this text
    e                every task, or only the Shells
    c                the task's name instead of the command
    1 2 3 4          show or hide a box
    + -              refresh faster or slower
    Space            pause
    Ctrl-L           draw everything again
    h ? Help         the keys
    q Esc Ctrl-C     quit

The mouse selects a task; the wheel scrolls.

## RETURN CODES
0 fine, 10 bad arguments, 20 no terminal window, no memory or no
timer.device.

## NOTES
CPU% and TIME+ are exact: exec calls ctop's small hooks each time a task
gets and loses the processor (as `top` does; one of the two holds the
hooks at a time, the other samples 50 times a second). Time in
interrupts counts for the task they interrupted. Volumes are read every
10 seconds with requesters off; the Pi every 2 seconds.

## SEE ALSO
`top`, `ps`, `kill`, `free`, `df`, `uptime`
