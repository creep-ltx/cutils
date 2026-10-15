# Sashimi - catch the serial debug output on the same Amiga

## SYNOPSIS
    Sashimi [ON] [BUFK N | BUFFERSIZE N] [CONSOLE] [WINDOW spec]
            [NOPROMPT] [QUIET] [ASKEXIT] [ASKSAVE] [TIMERON]
    Sashimi OFF | SAVE | SAVEAS file | EMPTY | WAKE
    Sashimi RECOVER file

## DESCRIPTION
Debugging output from programs (kprintf), Enforcer and MungWall normally
goes out of the serial port, to be read on a second computer. Sashimi
catches it on the Amiga itself instead: it keeps the text in a ring buffer
and shows it in its window or its output, so no second machine is needed
and modems or printers on the serial port are left alone.

A second run of Sashimi with OFF, SAVE, SAVEAS, EMPTY or WAKE talks to the
copy already running.

## ARGUMENTS
Starting:
- **ON** start (the default; not needed).
- **BUFK** N
  the buffer size in KB, a power of 2 (4 or more); 32 KB if not given.
- **BUFFERSIZE** N
  the buffer size in bytes, any number.
- **CONSOLE** open a console window of its own for the output. Keys
  there: Ctrl-E empty the buffer, Ctrl-F save it to T:sashimi.out,
  Ctrl-D reset the console (after garbage changed its colours or font),
  Ctrl-C quit.
- **WINDOW** SPEC
  with CONSOLE: the window to open, as a CON: specification.
- **NOPROMPT** do not print "Sashimi installed".
- **QUIET** only collect into the buffer, show nothing (until WAKE).
- **ASKEXIT** on Ctrl-C, ask whether to really quit. Needs an input,
  so not for a Sashimi started with Run >NIL:.
- **ASKSAVE** on quitting, offer to save the buffer.
- **TIMERON** look for new output ten times a second, instead of only
  when a line ends.

Talking to the running copy:
- **OFF** tell it to quit.
- **SAVE** save its buffer as T:sashimi.out.
- **SAVEAS** FILE
  save its buffer as FILE.
- **EMPTY** clear its buffer.
- **WAKE** end QUIET mode: show everything collected, then go on
  showing.

After a crash:
- **RECOVER** FILE
  search memory for the buffer of a Sashimi from before a crash and
  save it to FILE, then end. The system is frozen while it searches;
  finding it is not guaranteed.

## EXAMPLES
    Sashimi ASKSAVE
    Run >NIL: Sashimi CONSOLE BUFK 64 NOPROMPT
    Run Sashimi >RAM:debug.txt
    Sashimi SAVEAS RAM:debug.txt EMPTY
    Sashimi OFF
    Sashimi RECOVER RAM:lost.txt

## RETURN CODES
RECOVER: 0 the buffer was found and saved, 5 not found.

## NOTES
Sashimi 1.7 (2001) by Olaf Barthel, public domain, with source (Aminet
`dev/debug/Sashimi`; its documentation is sashimi.doc). It replaces
Carolyn Scheppner's Sushi. It needs Enforcer V37 or a compatible tool (not
older Enforcers) to catch Enforcer's output.

Only one copy collects at a time: a second copy started without one of the
talking keywords finds the first one in place and does nothing. A copy
that died but left its patch behind makes every later start do nothing,
silently. On this A1200 wasabid catches the same output with its own patch
of the same system function (`wasabi debug`).

## SEE ALSO
`Run`, `Break`
