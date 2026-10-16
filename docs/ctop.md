# ctop - btop for the Amiga

The user runs btop on the PC; ctop is that, showing the Amiga, inside the terminal (a CTerm tab or pane, or
any console window). Name and home chosen by the user 2026-10-16 (ctop, in CUtils). The original idea and
btop's look, from screenshots of the user's btop 1.4.7: ~/Projects/Ideas/amiga-btop.md and its pictures/.

## What it shows

Four boxes, as btop, each with a key (1-4) to show or hide it:

| Box | What | From |
|---|---|---|
| ¹cpu | a history graph of how busy the processor is (exact), a meter, the CPU type, 68k MIPS on Emu68, the Pi's temperature, ARM clock and throttling, the uptime | lib/cu_cpu.c (exec's task hooks), AttnFlags, lib/cu_pi.c (mailbox.resource) |
| ²mem | chip and fast memory: used, free, largest block, each with a meter and a graph; disks beside it: every volume with a meter | AvailMem, Info() (every 10 s, requesters off) |
| ³net | packets in and out per second, a mirrored graph, totals | SANA-II S2_GETGLOBALSTATS on the device in DEVS:Networks |
| ⁴proc | every task: CLI, name or command, priority, state, stack and its free part, CPU% with a small graph, TIME+ | cu_tasks + cu_cpu |

The clock sits in the top border, the refresh interval at the right (`- 1000ms +`).
The box at the right of the cpu box loses its frame below 6 rows. Below 70 columns the mem/net column goes
(memory then shows as one line in the cpu box). The net box needs 14 rows below the cpu box. Below 40 x 12
ctop only says the window is too small.

## How it draws

- lib/tui.c: diff redraw, 256 colours (TC_X, new for ctop; a plain console gets the nearest of its pens).
- Graphs in braille (CTerm draws U+2800-28FF itself in any font), colour by height green-yellow-red. A plain
  console (no UTF-8) gets ASCII graphs (`.:|#`) and `#`/`-` meters.
- Rounded boxes through tui_box(); titles in the top border, hot letters in red.
- It fits itself to the window: a Shell window on PAL hires (about 76 x 27) shows all four boxes; 40 x 12 is
  the smallest. The user asked for this (2026-10-16): btop's 80 x 24 minimum shut out a stock Shell window.

## Keys (all configurable: ENVARC:ctop.keys, written with the built-ins when missing)

Up/Down/PgUp/PgDn/Home/End select, Enter details, k break (Ctrl-C), s choose the break signal (C/D/E/F), N
priority, Left/Right sort column, r reverse, f filter, e every task or only the Shells, c name or command,
1-4 boxes, + - interval, space pause, h help, q Esc quit. Mouse: click a row, the wheel scrolls.

## Traps met building it

- **A 4 KB stack is not enough** (a CLI started by `Run` has 4 KB): the drawing plus 64-bit sums overflowed it,
  and the result was a random Line-F crash (#8000000B) seconds later. ctop swaps to a 16 KB stack of its own,
  as wget does.
- **qsort and a key pointer into the rows**: a pointer kept in a row points into another row once qsort has
  moved them, so comparisons changed their minds mid-sort and libnix's quicksort could run off the array.
  The key is worked out from the row on every comparison.
- **Inline asm calls to bsdsocket**: a0/a1/d0/d1 are the callee's to change; list them as outputs ("+r"), not
  plain inputs.
- **Emu68 is found through devicetree.resource + /emu68** (as Emu68CP and Wasabi do); the old expansion-board
  check in top never matched on the A1200.
- FS-UAE's emulated bsdsocket has no byte counters: the net box says so. On the A1200 (lwip-amiga) it works.

## State

0.1b1, 2026-10-16: tested in FS-UAE 3.1 and 3.2.1 (CTerm on an own 8-bit screen, Shell windows from 48 x 18
to 78 x 29) and on the A1200 (CTerm full screen RTG and in a Workbench window).
