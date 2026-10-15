# sumi - text editor that lives inside the terminal

## SYNOPSIS
    sumi [+LINE] [FILE ...]

## DESCRIPTION
Sumi is a text editor for the Amiga. You start it from a Shell in CTerm
(or in any console window) and it draws there, in that tab or pane.
It is simple but complete, in the way of nano and micro on Unix.

Each FILE opens in a tab of its own. With no FILE you get one empty,
new file. A name that does not exist yet opens as a new file, which is
made when you save it.

The keys are the ones most programs use today: Ctrl+S saves, Ctrl+C,
Ctrl+X and Ctrl+V copy, cut and paste, Ctrl+Z undoes, Ctrl+F finds.
Shift with a moving key selects text. Every key can be changed (see
KEYS FILE). **F1** or **Help** shows every key as it is bound now.

In CTerm the mouse works too: a click places the cursor, a drag
selects, a double click selects a word, the wheel scrolls, and a click
on a tab shows that file. Copy and paste use the Amiga clipboard, so
text moves to and from every Workbench program. Pasting with CTerm's
own paste key works as well; the text then goes in as it came, with no
extra indenting.

The bottom line shows messages and asks the questions (file names,
what to find, yes or no). There, **Esc** goes back without doing
anything, Up and Down bring back what you typed before, and **Tab**
completes a file name.

## OPTIONS
- **+LINE** start the next FILE at line LINE, e.g. `sumi +120 main.c`.

## FILES ARE KEPT AS THEY ARE
Sumi finds out how each file is written and saves it the same way:

- **Characters**: UTF-8 when the file is valid UTF-8 with something
  beyond plain ASCII in it, otherwise Latin-1 (the Amiga's own).
- **Line ends**: CR LF when every line has them (files from a PC),
  otherwise LF (the Amiga's and Unix's).
- **The last new line**: kept, or left out, as the file had it.

Saving writes a new copy beside the file first, and only a complete
copy replaces the old file. The file keeps its protection bits and its
comment. If the file was changed on disk since Sumi read it, Sumi asks
before writing over it. A file protected against writing or deleting is
not saved: Sumi says so (use Protect with +wd first).

When you quit, Sumi remembers where the cursor was in each file, and
puts it there again the next time you open the same file.

## COLOURS FOR CODE
The language is chosen from the file's name, or its first line:

    C          .c .h .cpp .cc .hpp .cxx
    68k asm    .s .asm .i
    ARexx      .rexx .rx .ced .thor, or a first line starting
               with /* (when nothing else fits)
    Amiga E    .e .m
    Blitz      .bb .bb2 .ab2 .ab3 .abs .bba
    Installer  .install .installer, a name starting with Install,
               or a first line such as (set or (welcome
    AmigaGuide .guide, or a first line starting with @database
    AmigaDOS   .script, a name with "startup" in it
               (Startup-Sequence, User-Startup...), a file in S:
    Python     .py
    Markdown   .md .markdown
    shell      .sh .bash .zsh, or a first line #!.../sh
    Makefile   Makefile, GNUmakefile, .mk .mak
    JSON       .json
    HTML/XML   .html .htm .xml .svg
    Lua        .lua

The `lang` command (below) picks another one by hand.

Blitz Basic 2 and AmiBlitz files must be saved as text (ASCII) from the
Blitz editor; Blitz's own tokenized files are not text.

In CTerm the colours come from CTerm's theme. In a plain console window
Sumi uses the Workbench colours.

## FINDING AND REPLACING
**Ctrl+F** asks what to find and goes to the next match. Every match on
the screen is marked; **Esc** takes the marks away. If text is
selected on one line, that text is what Ctrl+F looks for. **F3** (or
Ctrl+G) finds the next one, **Shift+F3** the one before.

Case is "smart" at first: lower case finds both, and a capital letter
in what you seek makes the search exact. `set case` changes this.

**Ctrl+R** finds and replaces. For each match it asks: **y** replace
this one, **n** skip it, **a** replace all the rest, **q** stop. All the
replacing is undone with one undo.

With `set regex on`, what you seek is a *regular expression*, matched
one line at a time:

    .        any one character
    [a-z]    one of these        [^a-z]  anything but these
    ^  $     the start, the end of the line
    x*       x any number of times (also none)
    x+       x once or more      x?  x or nothing
    \d       a digit             \w  a letter, digit or _
    \s       a space or tab      \b  the edge of a word
    \.       a backslash makes a special character plain

There are no groups and no alternatives (`|`).

## COMMANDS
**Ctrl+U** asks for a command, typed on the bottom line:

- **save** [NAME]
  save the file; with NAME, save it under that name (the tab then has
  that name). **w** is the same.
- **open** [NAME]
  open NAME in a new tab; with no NAME, ask for it. **o** and **e**
  are the same.
- **insert** [NAME]
  put a file into the text at the cursor. Sumi asks for the name,
  with NAME already filled in.
- **goto** N
  go to line N. **line** is the same.
- **run** COMMAND
  run an AmigaDOS command right in the terminal. Its output stays on
  the screen until you press a key. If it fails, Sumi tells you its
  return code.
- **count** count the lines, words and characters of the file, or of
  the selection.
- **lang** NAME
  colour the file as another language: text, C, asm, rexx, e, bb,
  install, guide, script, py, md, sh, make, json, html, lua.
- **quit** quit, as Ctrl+W. **q** is the same.
- **set** NAME VALUE
  change a setting (see SETTINGS).

## SETTINGS
Set with the command `set NAME VALUE` (Ctrl+U first). The settings are
kept for the next time, except encoding and lineends, which belong to
the one file. on and off may also be written yes/no or 1/0.

    set tabsize N          tab width, 1 to 16 (8)
    set tabs spaces|tabs   what the Tab key puts in (tabs)
    set numbers on|off     line numbers (on)
    set autoindent on|off  a new line indented as the one
                           before (on)
    set wrap on|off        soft wrap: long lines folded on
                           screen (off)
    set wrapcol N          the width for Justify, 10 to 400 (72)
    set autoclose on|off   brackets and quotes closed as you
                           type them (off)
    set whitespace on|off  tabs and trailing spaces shown (off)
    set regex on|off       find with regular expressions (off)
    set case smart|exact|any   case when finding (smart)
    set backup on|off      keep the old version as NAME.bak
                           when saving (off)
    set encoding utf8|latin1   convert this file's characters
    set lineends lf|crlf   this file's line ends

F4, F8 and Shift+F4 switch line numbers, soft wrap and whitespace on
and off, and are kept the same way.

## KEYS
The defaults. Ctrl is the Ctrl key, Alt either Alt key, Shift either
Shift key.

    Ctrl+S             save
    F2                 save as (asks for a name)
    Ctrl+O             open a file in a new tab
    Ctrl+T             a new, empty tab
    Ctrl+Q             close the tab (asks if not saved)
    F10, F9            the next tab, the previous tab
    Ctrl+W             quit (asks for each file not saved)

    Ctrl+Z, Ctrl+Y     undo, redo
    Ctrl+C             copy (nothing selected: the line)
    Ctrl+X             cut (nothing selected: the line)
    Ctrl+V             paste
    Ctrl+K             cut the line (again: the lines gather)
    Ctrl+D             duplicate the line (or the selection)
    Ctrl+A             select everything
    Tab, Shift+Tab     indent, unindent (also selected lines)
    Return             a new line (indented as the one before)
    Backspace, Del     delete back, delete on
    Alt+Backspace      delete the word before
    Alt+Del            delete the word after

    Ctrl+F             find
    F3, Ctrl+G         find the next
    Shift+F3           find the previous
    Ctrl+R             find and replace
    Ctrl+L             go to a line (LINE or LINE:COLUMN)

    Up Down Left Right move
    Alt+Left/Right     a word back, a word on
    Ctrl+Left/Right    the start, the end of the line
    Alt+Up/Down        a page up, a page down
    Ctrl+Up/Down       the start, the end of the file
    Home, End          the start, the end of the line
    PgUp, PgDn         a page up, a page down
    Ctrl+Home/End      the start, the end of the file
    Ctrl+P, Ctrl+N     a page up, a page down
    Ctrl+B, Ctrl+E     the start, the end of the line
    Shift+arrows       select as you move; also with
                       Shift+Alt+arrows, Shift+Ctrl+arrows,
                       Shift+Home, Shift+End

    F5                 to the matching bracket
    F6                 comment the line(s) on or off
    Ctrl+Alt+Up/Down   move the line(s) up or down
    F7                 Justify: reflow the paragraph
    Shift+F2           insert a file at the cursor
    Shift+F5           record keys (again: stop)
    Shift+F6           play the recorded keys
    F8                 soft wrap on or off
    Shift+F4           show tabs and trailing spaces
    F4                 line numbers on or off
    Ctrl+U             a command (see COMMANDS)
    Esc                end the selection, or the question
    F1, Help           help: every key as bound now

Why these keys: in a terminal the Right Amiga keys belong to CTerm's
menus and the Left Amiga keys to its panes, so a program inside never
sees them. The A1200 has no Home, End or Page keys, so Ctrl and Alt
with the arrows do that work. CTerm sends those combinations to Sumi; a
plain console window sends them as plain arrows, which is why Ctrl+P,
Ctrl+N, Ctrl+B and Ctrl+E are there as well.

Ctrl+C, Ctrl+D, Ctrl+E and Ctrl+F reach a program as the Amiga's break
signals. Sumi takes them as keys: Ctrl+C copies, it does not stop Sumi.

A key that does nothing says its name on the bottom line, so you know
what to write in the keys file.

## KEYS FILE
The keys are in `ENVARC:Sumi/Sumi.keys`. Sumi reads `ENV:Sumi/Sumi.keys`
first (the copy made at boot), then the one in ENVARC:. If there is
none, Sumi writes one with all the defaults on its first start, in both
places, for you to change. A changed file is used the next time Sumi
starts; change the ENVARC: copy and copy it to ENV: too, or reboot.

One binding a line: the key, a space, the action. A line starting with
`;` or `#` is a comment. The action `none` takes a key away. When a key
is named twice, the later line wins. Keys you do not name keep their
defaults, so a key added in a new version works without editing the
file.

    ; my keys
    <C-s> save
    <F3> findnext
    <A-Left> wordleft
    <C-q> none

Keys are written `<C-x>` (Ctrl+X), `<A-x>` (Alt; `<M-x>` also works),
`<S-x>` (Shift); they combine, e.g. `<S-A-Left>`. Named keys: `<Up>`
`<Down>` `<Left>` `<Right>` `<Home>` `<End>` `<PgUp>` `<PgDn>` `<Ins>`
`<Del>` `<BS>` `<Tab>` `<Enter>` (or `<Return>`) `<Esc>` `<Help>`
`<Space>` and `<F1>` to `<F12>`. A single character stands for itself.

The actions:

    up down left right wordleft wordright linestart lineend
    pageup pagedown filestart fileend
    selup seldown selleft selright selwordleft selwordright
    sellinestart sellineend selpageup selpagedown
    selfilestart selfileend selectall
    enter backspace delete delwordleft delwordright
    tab untab cut copy paste cutline duplicate undo redo
    save saveas open newtab close nexttab prevtab quit
    find findnext findprev replace goto help command
    numbers escape match comment justify softwrap
    moveup movedown insert macrorecord macroplay whitespace
    none

A line Sumi does not understand is named on the bottom line when it
starts.

## FILES
- **ENVARC:Sumi/Sumi.keys** the keys (see KEYS FILE).
- **ENVARC:Sumi/Sumi.prefs** the settings, one `name=value` a line.
  Written by Sumi when a setting changes, also to ENV:Sumi.
- **ENVARC:Sumi/Sumi.places** where the cursor was in each file (the
  last 100 files).
- **NAME.bak** the version before the last save, with `set backup on`.

## EXAMPLES
    sumi S:User-Startup
    sumi +40 main.c main.h
    sumi

## RETURN CODES
0 fine, 10 not started from a Shell in a terminal or console window,
20 out of memory, or the console could not be used.

## NOTES
Sumi must be started from a Shell; started from Workbench it does
nothing. It is made for CTerm (colours from its theme, the mouse,
UTF-8, the modern key combinations). In a plain console window it
works with the keyboard.

Up to 24 files can be open in tabs at once.

Justify (F7) reflows the paragraph at the cursor to the width set with
`set wrapcol` (72), as nano's Justify does.

Needs AmigaOS 3.x and a 68020 or better; made for 3.2 with fast
RAM.

## SEE ALSO
Maki, CTerm, `man`
