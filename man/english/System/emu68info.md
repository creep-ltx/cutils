# Emu68Info - show (and change) settings of Emu68 and the Raspberry Pi

## SYNOPSIS
    Emu68Info [HELP] [AMIGA] [BOARD] [CPU] [JIT] [TEMPERATURE] ...
    Emu68Info SETCLOCKRATE MHZ | SETTURBO 0|1 | SETLED 0|1 | ...

## DESCRIPTION
On an Amiga with a PiStorm running Emu68 (the 68k emulator on a Raspberry
Pi), Emu68Info reads what Emu68 and the Pi tell about themselves: the Pi
board and its clocks, temperature, voltage and power state, Emu68's
version, JIT settings and counters, and the Amiga side: chip set, memory
list, expansion boards and resident modules. Some values can also be
changed.

Several keywords may be given at once. `Emu68Info HELP` lists them all.
Keywords marked ($RC) below also put their value into the return code, for
use in scripts.

## ARGUMENTS
Reading:
- **HELP** list the keywords.
- **AMIGA** Amiga information.
- **BOARD** Raspberry Pi board information.
- **BOARDID** the Pi board's identifier ($RC).
- **BOARDNAME** the Pi board's name.
- **BOOTARGS** the Pi's boot arguments (device tree).
- **CMDLINE** the contents of the Pi's cmdline.txt.
- **CHIPID** the Agnus/Alice chip's ID ($RC).
- **DENISEID** the Denise/Lisa chip's ID ($RC).
- **CLOCKRATE** the ARM clock in MHz ($RC); **CLOCKRATES** all clocks.
- **COUNTERS** Emu68's counters: instructions run, 68k and ARM.
- **CPU** the CPU information.
- **DEBUG** Emu68's debug settings.
- **DETECT** whether Emu68 is running ($RC).
- **DMA** the Pi's DMA information.
- **EDID** the connected monitor's EDID data.
- **EXPANSION** the Amiga's list of expansion boards.
- **IDSTRING** Emu68's identification string.
- **JIT** the JIT compiler's settings and cache use.
- **LED** the Pi's LED information.
- **MEMLIST** the Amiga's memory list.
- **MODULES** the Amiga's resident modules.
- **POWERSTATE** the Pi's power information.
- **TEMPERATURE** the Pi's temperature.
- **UPTIME** the Pi's time since start ($RC).
- **VOLTAGE** the Pi's voltages.

Changing:
- **SETATTN** HEX
  set Exec's AttnFlags (the CPU/FPU flags), in hexadecimal.
- **SETCLOCKRATE** MHZ
  set the ARM clock rate.
- **SETTURBO** 0|1
  the Pi's turbo mode off or on.
- **SETLED** 0|1
  the Pi's LED off or on.
- **SETJITICNT** N
  JIT: instruction depth, 1 to 256.
- **SETJITIRNG** N
  JIT: inlining range, 0 to 65535.
- **SETJITLCNT** N
  JIT: inline loop count, 1 to 16.
- **SETJITSF** 0|1
  JIT: soft flush off or on.
- **SETJITSFL** N
  JIT: soft flush limit, 1 to 4000.
- **HARDRESET** reset the Pi: Emu68 is loaded again from scratch.

## EXAMPLES
    Emu68Info HELP
    Emu68Info BOARD TEMPERATURE VOLTAGE
    Emu68Info JIT
    Emu68Info DETECT

## NOTES
Emu68Info 0.1 (2025) by Philippe Carpentier (flype44), reachable from the
Emu68-tools collection (github.com/michalsc/Emu68-tools). It is only for
machines running Emu68.

The SET keywords and HARDRESET change a running machine: a wrong clock,
JIT or AttnFlags value can crash it, and HARDRESET restarts everything at
once. On this A1200 they have not been tried.

## SEE ALSO
`lspci`, `whichamiga`
