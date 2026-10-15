# conbench - measure how fast a console window is

## SYNOPSIS
    conbench [NAME] LABEL [TO FILE] [REPS N] [SCALE N] [SYNC] [FORCE]

    Template: NAME/A,TO/K,REPS/K/N,SCALE/K/N,SYNC/S,FORCE/S

## DESCRIPTION
conbench times a console: CON:, CCON:, KingCON, ViNCEd, or any other
console handler. It writes fifteen kinds of output to the window, from
plain lines as `dir` prints them to the full-screen repaints of an
editor, and measures how long each one takes. It needs AmigaOS 2.0 or
later.

Run it **inside** the console you want to measure. It writes its test
output to its own window, so the console being measured is the one the
Shell runs in. Open a Shell in that console first, then start conbench
there.

Each test starts from a cleared window and names itself on the first
line, so the tests that are blank on purpose (scroll-nl, insdel-line)
do not look like a hang. At the end the window is cleared, the table
of results is shown, and the same table is **added to the end** of the
result file. So runs from several consoles land side by side in one
file.

Before the tests conbench asks the console how big the window is, and
writes the size into the results. When the window is narrower than 79
columns it adds a warning: the 78-character line tests then wrap, and
measure something else.

## OPTIONS
- **NAME** LABEL
  the name written into the results for this run, for example `CCON`
  or `CON`. Call it what you like. It must be given; the word NAME
  itself may be left out.
- **TO** FILE
  the file the results are added to. The default is
  `RAM:conbench.txt`. The file is made if it is not there.
- **REPS** N
  run all the tests N times and keep the best time for each (default
  3). This guards against another program taking time in the middle of
  a test.
- **SCALE** N
  make every test N times bigger (default 1). Use it when results come
  back marked with `?`.
- **SYNC** end every test with a check that makes the console finish
  what it holds before the clock stops (see HONEST NUMBERS).
- **FORCE** run even when the output is not a console (for testing
  conbench itself). The numbers then mean nothing.

## THE TESTS
    plain-lines   816 lines of 78 characters, one write per line:
                  ordinary command output
    block-4k      the same bytes in 4 KB writes
    block-32k     414 lines in one single write
    bytewise      one write per character
    scroll-nl     12000 empty lines: scrolling only (blank on
                  purpose)
    wrap-long     300-character lines the console must wrap
    sgr-colour    colour changes several times a line
    sgr-perchar   a colour change before every character
    cursor-pos    the cursor moved to a place, then short text
    vt-frame      a full-screen repaint, row by row, as an editor
                  or pager does it
    insdel-line   lines inserted and deleted (blank on purpose)
    insdel-char   characters inserted and deleted inside a line
    clear-page    a cleared page and a screenful, 40 times
    erase-eol     return, text, erase to the end of the line: the
                  progress-bar way
    sync-line     plain-lines with the SYNC check after every line

The table shows for each test the bytes written, the seconds, and the
bytes a second, then a TOTAL line.

## HONEST NUMBERS
A console accepts output and then draws it. What conbench times is how
long the console takes to **accept** the output. A console that says
"done" first and draws later looks faster here than it really is. Most
fast consoles work that way (the stock console, ViNCEd, CCON). Three
things help to see through it:

- **SYNC** ends each test with a request the console must answer after
  the output it holds. If a console's numbers change between a run
  without SYNC and one with it, that console draws later, and its
  numbers without SYNC are too good.
- **sync-line** makes that check after every line. The gap between
  plain-lines and sync-line is how much the console holds back.
- **The window size** in the results shows that the runs you compare
  had the same window.

The tests insdel-line and insdel-char are small. Without SYNC their
times can swing a lot; use SYNC for them.

To compare consoles fairly: the same window size, the same screen
depth, the same font, and nothing else running.

The clock counts in fiftieths of a second. A result under half a
second is marked `?`: too short to trust. Raise SCALE rather than
believing it.

## EXAMPLES
In a Shell opened with `NewShell CCON:`:

    conbench CCON TO RAM:conbench.txt
    conbench CCON TO RAM:conbench.txt SYNC

In a Shell opened with `NewShell CON:`:

    conbench CON TO RAM:conbench.txt SYNC
    conbench CON REPS 5 SCALE 2

## NOTES
conbench refuses to run when its output is redirected to a file: it
would time the file system, not the console. FORCE overrides this.

When its input is not a console, the window size cannot be asked, and
the results say the probe was skipped.

The result file is written only after all the tests, so no disk work
happens while the clock runs.

Results of version 0.2 and later cannot be compared with those of 0.1:
the tests were made twice as big.

## SEE ALSO
`NewShell`, CCON
