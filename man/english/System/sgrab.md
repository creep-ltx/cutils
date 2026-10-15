# SGrab - save a picture of a screen or window (screen grabber)

## SYNOPSIS
    SGrab [FILE name] [WINDOW] [WINDOWCONTENTS] [MARK] [DELAY S]
          [PNG | JPG] [CLIPUNIT N] [X n] [Y n] [W n] [H n] ...
    SGrab CX_POPKEY key [GRABSCRKEY key] [GRABWINKEY key] ...

## DESCRIPTION
Grabs the front screen, the active window or a part of the screen and
saves it as an IFF ILBM picture (24-bit ILBM for deep screens), or as PNG
or JPEG, or puts it on the clipboard. It works from the Shell, from
Workbench (with tooltypes instead of arguments) and as a commodity with
hot keys and a window to pick screens and windows from.

From the Shell it waits DELAY seconds (5 by default) so the screen can be
arranged, then grabs. Grabbing a big screen can take some seconds, during
which drawing on it is held up. Without FILE, a file requester asks where
to save.

When CX_POPKEY is given, SGrab installs itself as a commodity and stays:
the hot key opens its window (a list of screens and windows, Grab, Mark &
Grab, View, the delay, file and file type), GRABSCRKEY grabs the current
screen and GRABWINKEY the current window at once.

## ARGUMENTS
What and how:
- **FILE** NAME
  the file to write (the word FILE may be left out). It may contain
  keywords in braces: {number} (the next free number), {width},
  {height}, {depth}, {size} (all three),
  {title}, {owner}, {type}, {time}, {date}; `{2:title}` takes only the
  first two words. Characters not allowed in names become `_`.
- **WINDOW** grab the active window, not the screen. **WIN** for short.
  Hidden parts of a window may come out blank or garbled.
- **WINDOWCONTENTS** grab the window without its borders. **WINC**.
- **MARK** mark the area with the mouse: drag with the left button;
  the right button cancels.
- **X** N
  the left edge of the area to grab (0 by default). **XSTART**.
- **Y** N
  its top edge (0 by default). **YSTART**.
- **W** N
  its width (the screen's by default). **WIDTH**.
- **H** N
  its height (the screen's by default). **HEIGHT**.
- **DELAY** S
  seconds to wait before grabbing.
- **NUMBER** number the files: FILE.1, FILE.2, ... **NUM** for short.
- **CLIPUNIT** N
  save to this clipboard unit instead of a file. **CU** for short.
- **ICON** FILE
  give each picture an icon copied from this template icon.
- **NOBEEP** do not flash the screen after grabbing.
- **NOBLIT** do not use the blitter (for a deadlock with MCX).

Formats:
- **PNG** save as PNG; **PNG_COMPRESSIONLEVEL** N (0-9, **PCL**).
- **JPG** save as JPEG (needs jpeg.library). **Q** N the quality
  (1-100, default 85; in this version's template the long name is
  misspelt JPG_QUALITIY, so use Q), **JPG_SMOOTHING** N (0-100,
  **S**), **JPG_DCT** ISLOW|IFAST|FLOAT (**DCT**), **JPG_PROGRESSIVE**
  (**P**).

Commodity:
- **CX_POPKEY** KEY
  the hot key that opens SGrab's window, as a commodity key string
  (`"ctrl shift numericpad *"`).
- **CX_POPUP** YES|NO
  open the window at once when started (with CX_POPKEY).
- **CX_PRIORITY** N
  the commodity's priority.
- **GRABSCRKEY** KEY
  a hot key that grabs the current screen at once.
- **GRABWINKEY** KEY
  a hot key that grabs the current window at once.
- **PUBSCREEN** NAME
  the screen to open the window on. **PUB** for short.
- **WINPOS** MOUSE|CENTER|TOP
  where the window opens.
- **VIEWER** COMMAND
  the program for the View button; MultiView by default.
- **CLIPVIEWER** COMMAND
  the same for a grab on the clipboard. **CV** for short.
- **WAITREFRESH** TICKS
  how long to let the screen redraw after SGrab's window closes
  (25 ticks by default). **WR**.
- **AREXXPORT** NAME
  the ARexx port's name (SGRAB by default). Its commands GRABSCREEN,
  GRABWINDOW and QUITSGRAB need easyrexx.library.

## EXAMPLES
    SGrab FILE RAM:screen.iff
    SGrab FILE RAM:shot_{number}.png PNG DELAY 2
    SGrab FILE RAM:win.png PNG WINDOWCONTENTS DELAY 3
    SGrab FILE RAM:part.png PNG X 0 Y 0 W 320 H 200
    Run >NIL: SGrab CX_POPKEY "ctrl alt g" GRABSCRKEY "ctrl alt s"

## NOTES
SGrab 1.22 (2002), Aminet `util/wb/sgrab`; the manual is SGrab.guide in
that archive. Needs a 68020 and AmigaOS 3.0 or later.

## SEE ALSO
`MultiView`
