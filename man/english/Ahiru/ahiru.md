# ahiru - file manager in three columns, driven by vi keys

## SYNOPSIS
    Ahiru [PATH] [SCREEN | WINDOW | TERMINAL]

## DESCRIPTION
Ahiru is a file manager for the classic Amiga (AmigaOS 2.04 and up), in
the style of Ranger and Yazi on Linux. It works in text, from the
keyboard, with vi keys. The name is Japanese for a duck. It was called
UP-File until version 0.2b2.

The screen has three columns. On the left is the **parent** drawer, in
the middle the **current** drawer, and on the right a **preview** of
what the cursor is on:

- a drawer shows what is in it;
- text shows its first screen, and ANSI art shows in colour;
- a program shows its hunk count and its version string;
- an IFF picture shows its size, colours and mode, an 8SVX sound its
  rate and length;
- a ProTracker module shows its title, patterns and sample names;
- an LhA archive lists its members, an LZX or Zip archive shows how
  many members it has and their size;
- an ADF disk image shows its file system and volume name;
- anything else shows as hex.

The preview is made only when no key is pressed, so holding a key down
over a slow disk does not read a file for every row.

Files are coloured by kind: drawers blue, programs green, archives red,
pictures yellow, sounds magenta, source code cyan, icons dim.

Icons travel with their files. Copying, moving, renaming or deleting
`Foo` takes `Foo.info` along, whether icons are shown or not. Ahiru
remembers the cursor in each drawer, so going back into a drawer puts
you where you were.

Ahiru is a commodity (from 2.0). Its hotkey (**ctrl alt a** unless the
settings say other) brings it to the front, in the drawer that is
active on Workbench. Starting Ahiru a second time brings up the first
one. Exchange can show, hide and quit it.

## OPTIONS
- **PATH** the drawer to start in. Without it Ahiru starts where it was
  when it last quit (SAVEDIR yes), or else in START. A PATH that is
  not a drawer starts in SYS:.
- **SCREEN** a screen of its own, for this start only.
- **WINDOW** a window on Workbench, for this start only.
- **TERMINAL** inside the Shell window or CTerm pane it was started
  from, for this start only. TERM is the same.

Without one of these three words the MODE setting decides (see
SETTINGS).

## TOOL TYPES
Started from its icon, Ahiru reads these tool types. They win over the
settings file.

- **CX\_POPUP** `CX_POPUP=NO` starts Ahiru hidden. It waits for the
  hotkey.
- **CX\_POPKEY** the hotkey, for example `CX_POPKEY=ctrl alt a`.

## THREE WAYS TO SHOW
- **Screen** a screen of its own. It copies the Workbench's display
  mode and size, with 4, 8 or 16 colours. If that mode will not take
  the colours, a plain hires screen is used. If no screen can be
  opened, a window on Workbench.
- **Window** an ordinary window on Workbench, in Workbench's colours.
  It can be moved, sized and closed as any window. Its place and size
  are kept (WINDOW in the settings).
- **Terminal** inside the terminal Ahiru was started from, the way
  ranger and yazi run on Linux. Ahiru opens no window of its own. When
  it quits, the terminal is as it was.

In **CTerm** the terminal way has CTerm's colours, the mouse (click,
double-click, the wheel), pictures in the preview as real pictures,
and it follows the pane's size. In any other console (CON:, KingCON)
it has Workbench's colours, no mouse and no pictures, and the screen is
cleared when it quits. A Shell command (**!**), a Shell (**S**) and the
editor run in the terminal while Ahiru waits; then Ahiru comes back.
There is no hotkey in the terminal way.

MODE auto, the usual setting, means: inside the terminal when Ahiru is
started from a Shell in CTerm, otherwise a screen of its own. Started
from Workbench there is no terminal, so TERMINAL then means a screen.

## KEYS
A prefix key (**g y d p c u z o =**) opens a small panel at the bottom
that lists what can follow. **m** and **'** show the bookmarks and wait
for a key of their own. **?** or Help shows every key as it is bound
now. Ctrl-C quits from anywhere.

The built-in keys in the file list. `<S-Up>` is Shift+Up, `<C-d>` is
Ctrl+d.

    j  <Down>            move down
    k  <Up>              move up
    J  <C-d>             half a page down
    K  <C-u>             half a page up
    <S-Down>  <C-f>      a page down
    <S-Up>  <C-b>        a page up
    gg  <S-Left>         to the top
    G  <S-Right>         to the bottom
    h  <Left>  <BS>      the parent drawer (at a root: the volumes)
    l  <Right>  <Return> enter, or open the file by its type
    i                    view the file (text, ANSI, hex)
    E                    edit the file
    x                    unpack the marked archives here (asks)
    <Space>              mark and move on
    v                    invert the marks
    uv                   unmark all
    yy                   yank (copy) the marked, or this entry
    dd                   cut the marked, or this entry
    pp                   paste here
    po                   paste, overwriting everything
    uy  ud               forget the yank list
    dD  <Del>            delete (asks)
    dt                   move to the volume's Trashcan
    cw                   rename, with an empty field
    a                    rename, cursor at the extension
    A                    rename, cursor at the end
    I                    rename, cursor at the start
    cb                   rename the marked (or all) in the editor
    cr                   rename by pattern, with a preview
    M                    make a drawer
    /                    search
    n                    the next match
    N                    the match before
    f                    filter the list as you type
    <Esc>                clear the filter
    s                    find names in every drawer below
    <C-s>                find text in the files below
    :                    command line (see below)
    !                    run a Shell command here
    S                    open a Shell here
    R  <F5>              read the drawer again
    ?  <Help>            the keys
    q                    quit
    zh                   show or hide .info icons
    zp                   preview column on or off
    zc                   4, 8 or 16 colours, in turn
    on                   sort by name
    os                   sort by size
    ot  om               sort by time
    oe                   sort by extension
    oN oS oT oM oE       the same, reversed
    or                   reverse the order
    gs gc gl gd          go to SYS: C: LIBS: DEVS:
    ge gt gr             go to ENV: T: RAM:
    gv                   the list of volumes
    g/                   the root of this volume
    gh                   home (START in the settings)
    cd                   go to a typed path
    m + key              bookmark this drawer on that key
    '  or  ` + key       jump to a bookmark
    t                    a new tab, here
    <C-w>                close this tab
    ]  [                 the next tab, the tab before
    1 ... 9              go to tab 1 to 9
    V                    visual mode: marks follow the cursor
    P                    play the module or sound (again: stop)
    cc  cf  cF           copy the path, name, drawer to the clipboard
    O                    open with ... (a menu)
    Z                    jump to a drawer you use often
    L                    after the names: size, date, bits, nothing
    w                    the copy running in the background
    pk                   pack the marked (or this) into an archive here
    =c                   compare with the next tab's drawer (marks)
    =s                   make the next tab's drawer like this one
    =m                   MD5 and CRC-32 of the marked files
    =d                   an .adf into a drive (DA0:) and in; again: out

In the viewer (**i**):

    q  Q  <Esc>  h  <Left>     close the viewer
    j  <Down>  <Return>        a line down
    k  <Up>                    a line up
    <Space>  f  <C-f>  <S-Down>  a page down
    b  <C-b>  <S-Up>           a page up
    d  <C-d>                   half a page down
    u  <C-u>                   half a page up
    g  <S-Left>                the start
    G  <S-Right>               the end
    x                          text or hex

The command line (**:**) takes `mkdir NAME`, `touch NAME`, `cd PATH`,
`shell COMMAND`, `rename NEW` (the entry under the cursor; without NEW
it asks) and `q` or `quit`.

## THE KEYS FILE
Every key can be changed in `PROGDIR:Ahiru.keys`, the file beside the
program. When there is no such file, Ahiru writes one with all its
built-in keys, as a full list to edit. Edit it and start Ahiru again.

One key a line: the keys, the action, and maybe an argument.

    gs go SYS:
    <Up> up
    q none

Keys are typed characters, or names in angle brackets: `<Up>` `<Down>`
`<Left>` `<Right>`, `<S-Up>` (Shift), `<C-Up>` (Ctrl) and the same for
the other arrows, `<C-d>` (Ctrl+d), `<Return>` `<Esc>` `<Tab>`
`<Space>` `<Del>` `<BS>` `<Help>` `<F1>` to `<F10>`. Two keys in a row
make a prefix. The action `none` takes a key away. A line starting with
`;` is a comment. `[main]` starts the keys of the file list, `[view]`
those of the viewer.

`sort` takes name, size, time or ext, with `reverse` after it, or
`flip`. `go` takes a path, or `volumes`, `root` or `home`. `tab` takes
a number, `bookmark` and `jump` wait for a key.

Your own commands: `run` shows the output in a console and waits,
`launch` starts the program and goes on. `{file}` `{name}` `{dir}`
`{marked}` and `{other}` (the next tab's drawer) are filled in, in
quotes:

    <F2> run Echo {file}
    <F3> launch SYS:Utilities/MultiView {file}

A line Ahiru does not understand is shown in red at the start; the
other lines still work. So is a key that is bound alone and also starts
a two-key key: the two-key key wins.

## SETTINGS
The settings are in `PROGDIR:Ahiru.config`. Ahiru writes the whole file
again each time it quits, from what it has in memory: comments and
lines it does not know are not kept. A line is `KEY value` (or
`KEY=value`); `;` or `#` at the start makes a comment.

    COLORS 8              ; 4, 8 or 16 (COLOURS also works)
    FONT topaz.font/8     ; a fixed-width font
    HIDEINFO yes          ; hide .info icons
    PREVIEW yes           ; the third column
    SORT name             ; name, size, date or ext
    REVERSE no
    START SYS:            ; where gh goes
    SAVEDIR yes           ; start where you quit (LASTDIR)
    HOTKEY ctrl alt a
    CXPOPUP yes
    VERIFY no
    LASTDIR Work:         ; written by Ahiru with SAVEDIR yes
    EDITOR                ; empty: ENV:EDITOR, then C:Ed
    EDITORCONSOLE no
    MODE auto             ; auto, screen, window or terminal
    WINDOW 40 30 680 400  ; the window's place and size
    LINEMODE size         ; size, date, bits or none
    OPENWITH mod Play     ; one line for each

- **COLORS** 4, 8 or 16 colours on Ahiru's own screen (default 8).
  **16** is the classic sixteen ANSI colours, **8** the ANSI eight with
  the blue made lighter, **4** black, grey, sky blue and amber.
- **FONT** any fixed-width font, as `name.font/size` (default
  topaz.font/8).
- **HIDEINFO** yes hides the .info icons in the list (default yes).
- **PREVIEW** yes shows the third column (default yes).
- **SORT** the order: name, size, date or ext (default name).
- **REVERSE** yes sorts backwards (default no).
- **START** the drawer **gh** goes to, and the start when there is no
  LASTDIR (default SYS:).
- **SAVEDIR** yes: Ahiru starts where it was when it quit. It writes
  that place as LASTDIR (default yes).
- **HOTKEY** the commodity's hotkey (2.0 and up). It brings Ahiru up
  in the drawer Workbench has active (default ctrl alt a).
- **CXPOPUP** no starts Ahiru hidden, waiting for the hotkey; **q**
  then only hides it (default yes).
- **VERIFY** yes reads every copy back and compares it before it is
  kept. Slower (default no).
- **EDITOR** the editor for **E**. Empty means ENV:EDITOR, then C:Ed.
- **EDITORCONSOLE** yes: the editor needs a console to run in. Sumi is
  known as one without this.
- **MODE** auto, screen, window or terminal (see THREE WAYS TO SHOW).
  A word on the command line is not saved here.
- **WINDOW** left, top, width and height of the window in window mode.
  Ahiru keeps it up to date.
- **LINEMODE** what follows the names: size, date, bits (protection) or
  none (default size).
- **OPENWITH** an extension (without the dot) and a program, offered by
  **O** for files with that ending. `*` means any file.

## FILES
- **PROGDIR:Ahiru.config** the settings (see SETTINGS).
- **PROGDIR:Ahiru.keys** the keys (see THE KEYS FILE).
- **PROGDIR:Ahiru.marks** the bookmarks, one a line: the key, a space,
  the drawer (`a DH0:Work`).
- **PROGDIR:Ahiru.places** the drawers you use, for **Z**.
- **PROGDIR:Ahiru.theme** the colours, written when missing. A section
  for each number of colours (`[4]`, `[8]`, `[16]`), with lines
  `colour N RRGGBB` (hex) and `ROLE N`, the colour that draws text,
  highlight, dir, program, archive, image, sound, dim, accent, mark,
  error, icon, source or assign.

If Ahiru.config or Ahiru.marks is missing, an old UP-File.config or
UP-File.marks beside the program is read instead.

## ARCHIVES AND DISK IMAGES
Enter on an lha, lzx or zip archive, or an ADF or ISO image, opens it
like a drawer. A DMS file is unpacked to an ADF first (Ahiru asks). You
can walk around in it, view (**i**) and edit (**E**) its files, yank
out of it, paste into it, rename and delete inside it. An edited file
goes back in when it is saved. A typed path or a bookmark can point
inside an archive.

Changes to lha, lzx and zip archives are made with C:lha, C:lzx and
C:zip, on a copy. Ahiru reads the copy back and checks it before it
replaces the archive. ADF images are written by Ahiru itself. ISO
images can only be read.

**x** unpacks archives with Ahiru's own readers; no outside program is
needed. **pk** packs into a new archive, with C:lha, C:lzx or C:zip.
**=d** puts an ADF into a drive (DA0:) so every program can use it;
this needs OS 3.2.

## MORE
- **Background copies** a paste between drawers on disks runs on its
  own. The status row counts it up, and you can go on. **w** shows it,
  and **x** there stops it. The other file operations wait for it.
  Pastes into or out of archives are done at once.
- **Find** **s** finds names in this drawer and all below: a part of
  the name, or an AmigaDOS pattern such as `#?.mod`. Ctrl-S finds files
  that hold a text. The hits show as a list; **h** or **R** goes back.
- **The trashcan** **dt** moves to the volume's Trashcan drawer, as
  Workbench has it, and makes one when the disk has none. **dD**
  deletes for good.
- **Compare and sync** **=c** marks what differs from the next tab's
  drawer; **=s** makes the next tab's drawer like this one.
- **Checksums** **=m** shows MD5 and CRC-32 of the marked files, and
  Return saves them as a .md5 file. On a .md5 or .sfv file it checks
  the files it lists.
- **Mouse** a click selects, a second quick click opens, a click in the
  parent column goes up, and the wheel scrolls.

## AREXX
Ahiru's ARexx port is called **AHIRU**.

- **GO** PATH go to a drawer.
- **GET** PATH|NAME|FILE|MARKED
  the current drawer, the name under the cursor, its full path, or the
  marked names.
- **ACTION** NAME [ARGUMENT]
  any action of Ahiru.keys, for example `ACTION sort size`.
- **REFRESH** read the drawer again.
- **QUIT** quit Ahiru.

While Ahiru is hidden or waits in a question, GET is answered, QUIT
quits when hidden, and the rest return 5 (busy). An unknown command or
action returns 10.

## EXAMPLES
    Ahiru
    Ahiru Work:Projects
    Ahiru RAM: WINDOW
    Ahiru TERMINAL
    rx "address AHIRU 'GO Work:'"

## RETURN CODES
0 fine, 20 Ahiru could not start (not enough memory, or no screen or
window could be opened).

## NOTES
This page is for Ahiru 0.3b2.

The work on files, archives and disk images is done by the same core
as CFile's, with the same safety rules. A file is written beside the
old one and swapped in only when it is whole. A move deletes only what
arrived. Links stay links. A newer version of a file is never replaced
without asking.

For testing: `KEYS=...` plays keys at the start, and `LOG` writes each
start-up step to RAM:Ahiru.log and the screen as text to RAM:Ahiru.grid.

## SEE ALSO
CFile, Sumi, CTerm, MultiView, Exchange
