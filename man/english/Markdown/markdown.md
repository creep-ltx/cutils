# markdown - view and edit Markdown files inside the terminal

## SYNOPSIS
    markdown [-e] [+LINE] [FILE ...]

## DESCRIPTION
Markdown shows a Markdown file the way it reads, and lets you edit it.
It runs inside the terminal: start it from a Shell in CTerm (or in any
console window) and it draws there, in that tab or pane. It is for
AmigaOS 3.2 on a fast Amiga (PiStorm, accelerator), 68020 or better.

The view shows headings, **bold**, *italic*, `code`, strikethrough,
lists (nested, numbered, task boxes), quotes and GitHub's
`> [!NOTE]` alerts, code blocks in a box with colours for the language
(C, asm, ARexx, E, Python, shell and more), tables with box lines, rules,
links, and the bits of HTML that READMEs use (`<img>`, `<br>`, `<b>`,
`<a href>`, comments).

Pictures (PNG, JPEG, GIF, IFF, BMP: whatever your datatypes read) show
in the text when it runs in CTerm. Elsewhere a line names the picture.

A file whose name ends in `.md` or `.markdown` opens in the view. Any
other file opens as text to edit, as in Sumi. A name that does not exist
yet opens as a new, empty file. Each file gets its own tab. Markdown
remembers where you were in each file and goes back there next time.

The editor is Sumi's, whole: tabs, undo, the clipboard, search and
replace, the mouse, macros. **e** (or Ctrl+E) turns the view into text
to edit, and back again.

## OPTIONS
- **-e** open the Markdown files to edit instead of in the view.
- **+LINE** start the next FILE at line LINE (`+120 notes.md`).

## THE VIEW
- **Links**: Tab chooses the next link, Return follows it. Another
  Markdown file opens in a new tab, `#heading` jumps to that heading,
  and a web address goes to your browser through OpenURL (without
  openurl.library it is copied to the clipboard). Backspace goes back.
- **Task boxes**: Tab to one, **x** ticks or unticks it; save with
  Ctrl+S.
- **The outline** (**o**): every heading in a list; Return goes there.
- **Find**: **/** asks for a word, **n** finds the next, **N** the one
  before.
- **Split** (**p** or Ctrl+P): the text and the view together, the view
  following the cursor as you type. On 80 columns the text is above
  the view; on 120 columns or more they are side by side
  (`set split auto|side|under` chooses).

## WRITING MARKDOWN
While editing a Markdown file:

- **Return** in a list or a quote starts the next item. On an empty
  item it ends the list.
- **Ctrl+B** makes the selection **bold**. With nothing selected it
  puts the bold marks in at the cursor.
- **Ctrl+N** is the format menu: italic, code, strike, link, picture,
  heading, quote, list, numbers, task box, code block, rule, table.
- **table** (a command, see below) makes the table at the cursor neat.
- **toc** (a command) puts in a list of all the headings, as links.

## KEYS
Every key can be changed. The keys are kept in
`ENVARC:Markdown/Markdown.keys`, one a line, a key and then what it
does (`<C-s> save`, `<A-Left> wordleft`, `x task`). The file has two
parts: the keys while editing come after `[edit]`, the keys in the view
after `[view]`. The word `none` takes a key away; `;` starts a comment.
The file is written with the defaults the first time Markdown starts.
**h** (in the view) or Help shows every key as it is bound now.

The default keys in the view:

    Space, f, PgDn, Alt+Down   a page down
    b, PgUp, Alt+Up            a page up
    Down, j / Up, k            a line down / up
    d / u                      half a page down / up
    g, <, Home, Ctrl+Up        the start
    G, >, End, Ctrl+Down       the end
    ] / [                      the next heading / the one before
    Ctrl+Right / Ctrl+Left     the next heading / the one before
    o, t, Shift+F1             the outline
    Tab / Shift+Tab            the next link or task box / the one
                               before
    Return, Right              follow the link (or tick the box)
    Backspace, Left            back
    x                          tick or untick a task box
    c                          copy the link's address
    /, ?, Ctrl+F               find
    n, F3 / N, Shift+F3        find the next / the one before
    e, Ctrl+E                  edit
    p, Ctrl+P                  edit with the view beside
    i                          pictures on or off
    Ctrl+S                     save
    Ctrl+O / Ctrl+T            open a file / a new empty tab
    F10 / F9                   the next tab / the one before
    q, Ctrl+Q                  close the tab
    Ctrl+W                     quit
    Ctrl+U                     a command (see COMMANDS)
    h, Help, F1                the keys

While editing, the keys are Sumi's: Ctrl+S save, Ctrl+Q close the tab,
Ctrl+W quit, Ctrl+F find, Ctrl+R replace, Ctrl+Z undo, Ctrl+Y redo,
Ctrl+C, Ctrl+X, Ctrl+V copy, cut and paste, Shift with the arrows
selects. The A1200 has no Home, End or page keys, so Ctrl and Alt with
the arrows do their work. Added for Markdown: Ctrl+E the view, Ctrl+P
split, Ctrl+B bold, Ctrl+N the format menu, Shift+F1 the outline.

Right Amiga keys belong to CTerm's menus and Left Amiga keys to its
panes, so Markdown never sees them.

## COMMANDS
Ctrl+U asks for a command:

- **save** [NAME] save (under NAME).
- **open** [NAME] open a file in a new tab.
- **insert** [NAME] put a file in at the cursor.
- **goto** N go to line N.
- **run** CMD run an AmigaDOS command here in the terminal.
- **count** lines, words and characters in the file or the selection.
- **table** make the table at the cursor neat.
- **toc** put in a list of the headings, as links.
- **lang** NAME the colours of another language (C, asm, rexx, e, md,
  sh, ...).
- **set** NAME VALUE change a setting (below).
- **quit** quit.

## SETTINGS
Settings are made with the **set** command and kept in
`ENVARC:Markdown/Markdown.prefs`.

- **set width** N the view's text is at most N columns wide (20 to
  400; 0 is the whole width). 100 at first.
- **set pictures** on|off show pictures or not.
- **set split** auto|side|under how split places the text and the
  view.

All of Sumi's settings work too: tabsize, tabs spaces|tabs, numbers,
autoindent, wrap, autoclose, whitespace, regex, backup (each on|off),
wrapcol N, case smart|exact|any, encoding utf8|latin1, lineends lf|crlf.

## FILES
- **ENVARC:Markdown/Markdown.keys** the keys (ENV: is read first).
- **ENVARC:Markdown/Markdown.prefs** the settings (ENV: is read first).
- **ENVARC:Markdown/Markdown.places** where each file was left.

## EXAMPLES
    markdown README.md
    markdown -e notes.md
    markdown +40 docs/design.md
    markdown README.md docs/install.md

## RETURN CODES
0 fine, 10 not started in a terminal, 20 out of memory, or the console
could not be used.

## NOTES
Markdown needs a terminal. Started from Workbench it does nothing.

Wide characters (Chinese, emoji) show as `?`. A font without a character
shows it as a blank.

A picture shows only when the whole of it is in sight; until then a line
names it.

Footnotes (`[^1]`) show as they are written.

In a plain Shell window (not CTerm) there are no pictures and no
strikethrough.

## SEE ALSO
`Sumi`, `Maki`, `CTerm`, `man`
