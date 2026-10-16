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

## How it draws

- lib/tui.c: diff redraw, 256 colours (TC_X, new for ctop; a plain console gets the nearest of its pens).
- Graphs in braille (CTerm draws U+2800-28FF itself in any font), colour by height green-yellow-red. A plain
  console (no UTF-8) gets ASCII graphs (`.:|#`) and `#`/`-` meters.
- Rounded boxes through tui_box(); titles in the top border, hot letters in red.
- 80 x 24 is the smallest (a 640 x 256 topaz 8 screen is 80 x 31); bigger screens get wider graphs and more rows.

## Keys (all configurable: ENVARC:ctop.keys, written with the built-ins when missing)

Up/Down/PgUp/PgDn/Home/End select, Enter details, k break (Ctrl-C), s choose the break signal (C/D/E/F), N
priority, Left/Right sort column, r reverse, f filter, e every task or only the Shells, c name or command,
1-4 boxes, + - interval, space pause, h help, q Esc quit. Mouse: click a row, the wheel scrolls.

## State

0.1b1 being built 2026-10-16.
