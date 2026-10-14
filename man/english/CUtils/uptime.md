# uptime - show how long since the Amiga was started

## SYNOPSIS
    uptime [-p | -s]

## DESCRIPTION
Shows the time now, how long the Amiga has run since it was last
started (a reset or switching it on), and how many Shells are open:

    21:45:20 up 2 days, 1:02, 3 shells

The time is counted by the timer's E-clock, which starts from zero when
the system starts.

## OPTIONS
- **-p** pretty: only the time up, in words: `up 2 days, 1 hour, 2
  minutes`.
- **-s** since: the date and time of the last start, e.g.
  `2026-10-14 20:57:12`.

## EXAMPLES
    uptime
    uptime -p
    uptime -s

## RETURN CODES
0 fine, 10 bad arguments, 20 the timer could not be opened.

## NOTES
The Amiga keeps no load average, so unlike Unix uptime there is none.

## SEE ALSO
`date`, `ps`, `free`
