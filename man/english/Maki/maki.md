# maki - pager: show text a page at a time, inside the terminal

## SYNOPSIS
    maki [-N] [-S] [+LINE | +/TEXT] [FILE ...]
    command | maki [-N] [-S] [+LINE | +/TEXT] [- FILE ...]

## DESCRIPTION
Maki shows text one screen at a time, forwards and back. It is less
for the Amiga, and it can take the place of More. You start it from a
Shell in CTerm (or in any console window) and it draws there, in that
tab or pane.

Give it one or more files, or send another command's output into it:
`List | Maki`, `Type foo | Maki`. A pipe shows at once and fills as the
text comes; the keys still come from the console. With several files,
**]** and **[** (or F10 and F9) go between them.

Big files are never read whole. Lines are found only as far as they
are needed, so a 10 MB file opens as fast as a small one. While you
read, Maki goes on finding lines in the background.

When the output is not a terminal (`maki foo >file`), Maki writes the
files out as they are, as `cat` does.

**h** shows every key as it is bound now; **q** quits. Every key can be
changed (see KEYS FILE).

## OPTIONS
- **-N** line numbers on the left.
- **-S** long lines cut at the screen's edge (Left and Right then look
  aside) instead of folded onto the next rows.
- **+LINE** start at line LINE.
- **+/TEXT** start at the first match for TEXT (a search, see
  SEARCHING).
- **-** read the pipe as well as the files given, as the first file.
- **?** show the usage line. **-h** and **--help** do the same.

Letters may be put together (`-NS`), and lower case works too (`-n`,
`-s`). `ENV:MAKI` may hold options that are used every time, before the
ones on the command line, as less's LESS: `SetEnv MAKI -N`.

## MOVING
Most keys take a number typed first: `5` then **Space** goes five pages
on, `120` then **g** goes to line 120, `50` then **%** goes to the
middle of the file. A number with **j** or **k** moves that many lines.

The mouse wheel scrolls three lines (in CTerm).

## SEARCHING
**/** searches forward, **?** backward. **n** finds the next match in the
same direction, **N** the one in the other direction. Every match on the
screen is marked; **Esc** takes the marks away. An empty search (just
Return) looks for the last text again. A search starts from where you
are and does not go round past the end or the start. **Ctrl+C** stops
a long search.

A capital letter in the text makes the search exact; with only lower
case, case does not matter. The text is a *regular expression*:

    .        any one character
    [abc]    one of these        [^a-z]  anything but these
    ^  $     the start, the end of the line
    x*       x any number of times (also none)
    x+       x once or more      x?  x or nothing
    \d       a digit             \w  a letter, digit or _
    \s       a space or tab      \b  the edge of a word
    \.       a backslash makes a special character plain: a dot

On the bottom line, Up and Down bring back what you searched for
before, and **Esc** (or Backspace on an empty line) goes back.

## WHAT IT SHOWS
- **Colours**: ANSI colour codes show as colours, also 256-colour and
  24-bit ones (as the nearest of the terminal's sixteen). Both ESC [ and
  the Amiga's own CSI character (0x9B) work.
- **Bold and underline** the way Unix man pages write them (a
  character, Backspace, the character again) show as bold and
  underline.
- **Characters**: UTF-8 when the start of the file is valid UTF-8,
  the PC's CP437 for files ending in .ans .nfo .diz or .asc, otherwise
  Latin-1. **F7** changes it: Latin-1, UTF-8, CP437 in turn.
- Tabs go to every eighth column. Other control characters show as
  `^X` in reverse.

## KEYS
The defaults. Ctrl is the Ctrl key, Alt either Alt key, Shift either
Shift key. N is a number typed first.

    Space f z PgDn Alt+Down Ctrl+V   a page down
    b w PgUp Alt+Up Ctrl+B           a page up
    Backspace                        a page up
    Return j e Down Ctrl+N           a line down
    k y Up Ctrl+P                    a line up
    d Ctrl+D                         half a page down
    u Ctrl+U                         half a page up
    g < Home Ctrl+Up                 the start (N g: line N)
    G > End Ctrl+Down                the end (N G: line N)
    Left, Right                      look aside (long lines cut)
    % p                              N percent of the way
    : Ctrl+L                         go to a line (asks)

    / Ctrl+F                         search forward
    ?                                search backward
    n F3                             the next match
    N Shift+F3                       the match before

    ] F10                            the next file
    [ F9                             the file before
    = Ctrl+G                         about the file
    v                                edit the file at this line

    F                                follow the end as it grows
    F4                               line numbers on or off
    F8                               long lines folded or cut
    F7                               the characters (encoding)
    m + a letter                     mark this place
    ' + the letter                   back to the mark
    r Ctrl+R                         draw everything again
    Esc Ctrl+C                       stop (following, a number,
                                     the search marks)
    h H Help F1                      help: these keys
    q Q                              quit (from the help: back)

**F** (follow) keeps showing the end of a file or pipe that is still
growing, as `tail -f` does. Any key stops it.

**v** opens the file in an editor at the line at the top of the screen:
the one named in `ENV:EDITOR`, or Sumi if it is not set. When the editor
ends, Maki reads the file again. A pipe cannot be edited.

**=** says the file's name, its lines and bytes, and its characters
(and "still coming" while a pipe is still being read).

Marks are the letters a to z, one set for each file.

Why these keys: in a terminal the Right Amiga keys belong to CTerm's
menus and the Left Amiga keys to its panes, so a program inside never
sees them. The A1200 has no Home, End or Page keys, so Ctrl and Alt
with the arrows do that work. Ctrl+C, Ctrl+D, Ctrl+E and Ctrl+F reach a
program as the Amiga's break signals; Maki takes them as keys.

A key that does nothing says its name on the bottom line, so you know
what to write in the keys file.

## KEYS FILE
The keys are in `ENVARC:Maki/Maki.keys`. Maki reads `ENV:Maki/Maki.keys`
first (the copy made at boot), then the one in ENVARC:. If there is
none, Maki writes one with all the defaults on its first start, in both
places, for you to change. A changed file is used the next time Maki
starts; change the ENVARC: copy and copy it to ENV: too, or reboot.

One binding a line: the key, a space, the action. A line starting with
`;` or `#` is a comment. The action `none` takes a key away. When a key
is named twice, the later line wins. Keys you do not name keep their
defaults.

    ; my keys
    <Space> pagedown
    <C-f> search
    x quit
    v none

Keys are written `<C-x>` (Ctrl+X), `<A-x>` (Alt; `<M-x>` also works),
`<S-x>` (Shift); they combine, e.g. `<S-F3>`. Named keys: `<Up>`
`<Down>` `<Left>` `<Right>` `<Home>` `<End>` `<PgUp>` `<PgDn>` `<Ins>`
`<Del>` `<BS>` `<Tab>` `<Enter>` (or `<Return>`) `<Esc>` `<Help>`
`<Space>` and `<F1>` to `<F12>`. A single character stands for itself,
so `G` and `g` are different keys.

The actions:

    down up pagedown pageup halfdown halfup start end
    left right search rsearch next prev quit help
    numbers wrap follow edit nextfile prevfile info
    encoding mark gomark percent redraw stop goto
    none

A digit that is bound to an action no longer counts as part of a
number. A line Maki does not understand is named on the bottom line
when it starts.

## ENVIRONMENT
- **MAKI** options used every time, before the command line's.
- **EDITOR** the editor for **v** (Sumi if not set).

## FILES
- **ENVARC:Maki/Maki.keys** the keys (see KEYS FILE).

## EXAMPLES
    maki S:Startup-Sequence
    maki -N main.c main.h
    maki +200 Work:Logs/big.log
    maki +/Error RAM:build.log
    List SYS:C | maki
    Type readme.txt | maki
    maki -S art.ans
    SetEnv PAGER Maki

## RETURN CODES
0 fine, 10 a file could not be opened (it is said; the others are
still shown), 20 out of memory, no console for the keys, or the console
could not be used.

## NOTES
Maki must be started from a Shell; started from Workbench it does
nothing. With no file, and nothing piped in, it shows the usage line.

Up to 32 files at once.

`man` uses Maki to show its pages when `ENV:PAGER` is set to Maki.

When the program writing into a pipe never stops (`Type SER:` and the
like), quitting leaves one small read buffer behind: the pipe may still
write into it, so it cannot be freed safely.

ANSI art that moves the cursor up or sideways (other than forward) is
shown as a stream, not drawn in place.

Needs AmigaOS 3.x and a 68020 or better.

## SEE ALSO
Sumi, CTerm, `man`, More
