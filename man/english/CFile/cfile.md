# cfile - two-pane file manager for every Amiga, Kickstart 1.3 and up

## SYNOPSIS
    CFile [LEFT] [RIGHT] [HIDE]

## DESCRIPTION
CFile is a file manager with two panes, side by side, inside a frame
that fills the screen. Everything is done from the keyboard. One
program runs on every Amiga, from Kickstart 1.3 on a plain 68000.

Files are known by their first bytes, not their names: programs, text,
ANSI art, lha, lzx and zip archives, ADF, DMS and ISO disk images, IFF
pictures and sounds, ProTracker modules. Each key does the natural
thing for the kind of file. Archives and disk images open like drawers,
with no outside program.

The frame is made at start for the font and screen CFile finds, so a
small font gives a wider, taller grid with the same layout. Each pane's
path is in the frame's border row; questions and messages borrow that
row and give it back. The border row also shows the free space of the
volume, or, while anything is marked, how many are marked and their
size. The size column shows a file's bytes, `<DIR>` for a drawer until
**=** measures it, or the date when sorted by date.

What each Kickstart gets:

                                         1.3     2.x     3.0 and up
    panes, copy, move, delete, viewer,   yes     yes     yes
    editor, find
    read lha, lzx, zip, ADF, ISO, DMS    yes     yes     yes
    write inside an ADF                  yes     yes     yes
    IFF pictures, 8SVX, ProTracker       yes     yes     yes
    write into lha, lzx, zip archives    (1)     yes     yes
    other picture and sound formats      -       -       datatypes
    panes refresh by themselves          -       yes     yes
    full AmigaDOS patterns in + and f    (2)     yes     yes
    command output while it runs         at end  yes     yes
    the hotkey (a commodity)             -       yes     yes

    (1) with a 68000 archiver in C:; LhA itself needs 2.0
    (2) only * ? and #?

## OPTIONS
- **LEFT** where the left pane starts. It wins over the settings file.
  An archive or disk image opens.
- **RIGHT** where the right pane starts.
- **HIDE** start hidden and wait for the hotkey (2.0 and up).
  `CX_POPUP=NO` is the same.

Without paths the panes start where the settings file says (SYS: and
RAM: at first).

On Kickstart 1.3, type `Stack 16000` in the Shell before starting
CFile. Without it CFile says so and stops, because 1.3's DOS crashes on
a small stack. Started from its icon, CFile needs nothing: the icon
carries a big enough stack.

## TOOL TYPES
Started from its icon, CFile reads the commodity tool types:

- **CX\_POPUP** `CX_POPUP=NO` starts CFile hidden.
- **CX\_POPKEY** the hotkey, for example `CX_POPKEY=ctrl alt c`.

## KEYS
These are the built-in keys. Most letters work in either case (**v**
and **V** both view). These are two keys each: **a A**, **c C**, **m
M**, **r R**, **k K**, **t T**. **D** deletes and **d** does nothing.
`<S-Up>` is Shift+Up, `<C-Up>` Ctrl+Up.

In the panes:

    <Tab>             switch the active pane
    <Up>  <Down>      move
    <S-Up> <S-Down>   a page up, a page down
    <C-Up> <C-Down>   the first, the last
    <Right>           enter a drawer, volume, archive or disk image
    <Left>            the parent; at a root, the list of volumes
    <Return>          open by type: enter, view, play, run (asks)
    v  V              view text, ANSI, hex, pictures; play sounds
    e  E              edit a text file
    i  I              information: size, date, note, bits, icon
    c                 copy to the other pane
    C                 copy, overwriting (never a newer version)
    m                 move to the other pane
    M                 move, overwriting (never a newer version)
    r                 rename (with marks, one question for each)
    R                 rename by pattern, with a preview
    n  N              new: name/ = drawer, name.adf = blank disk,
                      any other name = a new file in the editor
    <Del>  D          delete, drawers and all (asks)
    u  U              unpack archives into the other pane
    p  P              pack into a new archive in the other pane
    =                 measure a drawer
    <Space>           mark or unmark, and step down
    a                 mark all
    A                 unmark all
    *                 invert the marks
    +                 mark by pattern
    s  S              sort: name, size, date or reverse
    /                 filter the pane as you type
    g  G              go to a typed path
    h  H              history: the last 20 places of this pane
    b  B              set a bookmark here (then a digit)
    0 ... 9           jump to that bookmark
    f  F              find files by name, from here down
    t                 find text in the files from here down
    :                 run a Shell command in this drawer
    k                 compare the panes: what differs is marked
    K                 sync: make the other pane like this one
    x                 MD5 and CRC-32 of the marked files
    o                 an .adf into a drive (DA0:) and in; again: out
    T                 the next colour theme
    <F5>              read both panes again
    ?  <Help>         the keys
    <Esc>             quit (asks); stops a running job first

In the viewer:

    <Esc>  q  Q       close
    e  E              edit (text only)
    <Up>  <Down>      a line up, a line down
    <Space> <S-Down>  a page down
    <S-Up>            a page up
    <C-Up> <C-Down>   the start, the end
    /                 search
    n  N              the next match
    p  P              the match before
    <Right> <Left>    the next, the previous marked file (a tour)

In every typed line:

    <Return>          accept
    <Esc>             cancel
    <Left>  <Right>   move the cursor
    <S-Left>          to the start
    <S-Right>         to the end
    <BS>              delete before the cursor
    <Del>             delete under the cursor
    <S-BS>            delete back to the start
    <C-BS>            delete back to the nearest / or :

In the editor:

    <Esc>             close (asks about saving when changed)
    <Return>          split the line
    <BS> <S-BS> <C-BS>  delete before the cursor (joins lines)
    <Del>             delete under the cursor (joins lines)
    <Tab>             spaces to the next tab stop
    <Up>  <Down>      a line up, a line down
    <S-Up> <S-Down>   a page up, a page down
    <C-Up> <C-Down>   the first line, the last line
    <Left>  <Right>   the cursor left, right
    <S-Left>          the line's start
    <S-Right>         the line's end

In a list (the hits of f and t, the history, the / filter):

    <Return>          take this one
    <Esc>             close
    <Up>  <Down>      up, down
    <S-Up> <S-Down>   a page up, a page down
    <C-Up> <C-Down>   the first, the last
    <Space>           mark or unmark (the / filter)
    <BS>              take back a letter (the / filter)

In the picture viewer:

    <Esc>             close (any key not listed here does too)
    <Space> <Return>  the next picture (a tour)
    <BS>              the picture before
    arrows            walk around a big picture; at the edge,
                      the next or the picture before
    Shift + arrows    walk a screenful
    Ctrl + arrows     only walk
    +  =              zoom in (datatypes, 3.0)
    -                 zoom out

In the information window:

    <Esc>             close
    h s p a r w e d   switch that protection bit (either case)
    c  C              edit the note (comment)
    t                 the icon's next tool type
    T                 all tool types in the editor

The answers to a question (y and n, the sort letters, a bookmark digit)
are fixed: they are the letters the question shows.

## THE KEYS FILE
Every key can be changed in `CFile.keys`: `PROGDIR:CFile.keys` (beside
the program) from Kickstart 2.0, `S:CFile.keys` on 1.3. When there is
no such file, CFile writes one with all the built-in keys. Edit it and
start CFile again.

One key a line: the keys, the action, and maybe an argument.

    [main]
    <F5> rescan
    d delete
    T none

Keys are typed characters, or names in angle brackets: `<Up>` `<Down>`
`<Left>` `<Right>`, `<S-Up>` (Shift), `<C-Up>` (Ctrl) and the same for
the other arrows, `<C-d>` (Ctrl+d), `<Return>` `<Esc>` `<Tab>`
`<Space>` `<Del>` `<BS>` `<S-BS>` `<C-BS>` `<Help>` `<F1>` to `<F10>`.
The action `none` takes a key away. A line starting with `;` is a
comment. `jump` takes the bookmark's digit.

A section says where the keys work: `[main]` the panes, `[view]` the
text viewer, `[prompt]` every typed line, `[edit]` the editor, `[list]`
the lists of hits, the history and the / filter, `[picture]` the
picture viewer, `[info]` the information window. A section missing
from an older file keeps its built-in keys. In `[prompt]`, `[edit]` and
`[list]` a typed character bound to an action no longer types itself.

Two keys in a row make a prefix, in `[main]` only. The first key then
shows on the border row what can follow. A key that is bound alone and
also starts a prefix loses its own action; CFile says so at start, as
it does for a line it does not understand. The other lines still work.
The `[main]` key for `quit` also stops a running copy, search or
command.

Your own commands: `run` shows the output and waits, `launch` starts
the program and goes on. `{file}` `{name}` `{dir}` `{marked}` and
`{other}` (the other pane's drawer) are filled in, in quotes:

    <F2> run Echo {file}
    <F3> launch SYS:Utilities/MultiView {file}
    <F4> run lha a RAM:backup.lha {marked}

## CFILE.CONFIG
The settings are in a plain text file, `cfile.config`:
`PROGDIR:cfile.config` (beside the program) from Kickstart 2.0, and
`S:cfile.config` on 1.3. A line is `KEY value`. A line starting with
`;` is a comment.

    LEFT          SYS:
    RIGHT         RAM:
    SAVEDIRS      ON
    ICONS         ON
    SORT          name
    FONT          topaz/8
    THEME         CFile
    SAVEBOOKMARKS OFF
    HOTKEY        ctrl alt c
    CXPOPUP       YES
    VERIFY        OFF

- **LEFT** where the left pane starts (default SYS:). `(volumes)`
  starts it in the list of volumes. A path on the command line wins.
  A path that is an archive or disk image opens it.
- **RIGHT** the same for the right pane (default RAM:).
- **SAVEDIRS** ON: when CFile quits, the panes' places become the next
  start's LEFT and RIGHT (default ON). Inside an archive, the drawer
  that holds it is saved.
- **ICONS** ON: copy, move, delete and rename take a file's .info icon
  along (default ON). OFF leaves icons alone.
- **SORT** the order at start: name, size or date, with `rev` after it
  for reverse, as in `SORT size rev` (default name).
- **FONT** a fixed-width font, as `name/size`; `topaz` is the ROM font
  (default topaz/8). A font that will not open, is proportional, or
  leaves less than 80 by 18 characters is refused with a message, and
  Topaz 8 is used.
- **THEME** the colours: `CFile`, `Workbench 1.3`, `Workbench 2.x-3.x`
  or `ANSI / BBS` (default CFile). **T** goes through them, and the one
  you end on is saved. Themes need CFile's own screen.
- **SAVEBOOKMARKS** ON keeps the ten bookmarks across runs, as lines
  `BOOKMARK0` to `BOOKMARK9` (default OFF).
- **HOTKEY** the commodity's hotkey, 2.0 and up (default ctrl alt c).
  It brings CFile up in the drawer that is active on Workbench (3.x;
  elsewhere where it was). Exchange can show, hide and quit CFile, and
  starting CFile a second time brings up the first one.
- **CXPOPUP** NO starts CFile hidden, waiting for the hotkey (default
  YES). Quitting then only hides it; **q** at the question quits for
  good. It counts from the next start.
- **VERIFY** ON reads every copy back and compares it with the
  original before it is kept or replaces anything (default OFF).
  Slower; a copy that reads back wrong is thrown away with a message.

**The file looks after itself.** If there is none, CFile writes one
with every setting and a comment above each. When CFile knows a
setting your file lacks, it adds that line with its default. When it
quits, CFile changes only its own lines: LEFT and RIGHT (with SAVEDIRS
ON), the BOOKMARK lines (with SAVEBOOKMARKS ON) and THEME (when **T**
changed it). Your other lines, values and comments stay as you wrote
them. A file that is there but cannot be read is left alone. The file
is saved the safe way: written beside the old one, then swapped in.

Editing cfile.config in CFile's own editor applies it when you save,
a new HOTKEY too. A new FONT and CXPOPUP come with the next start.
An old `ARCWRITE` line from the E version of CFile is kept but not
used.

## FILE OPERATIONS
With marks, **c**, **m**, **Del**, **u** and **p** work on all the
marked entries at once, **r** asks for each one, and **v** shows them
one after another (a tour).

Copy and move ask about each name that is already taken:
`(s)kip (o)verwrite (r)ename?`. When a file meets a file, the question says
which versions they are, read as the Version command reads them, and
which one is newer. **C** and **M** overwrite without asking, except
over a newer version: that always asks. All questions come before
anything is copied, so cancelling there changes nothing. If the copy
will not fit on the target volume, CFile says how much it needs and
asks. Drawers are copied with all they hold. Copies keep protection
bits, dates and notes.

Delete asks first. A delete-protected entry asks
`unprotect? (y)es (n)o (a)ll`. An entry that will not go is skipped,
and the summary says what is left.

**Esc** stops a running copy, move, delete, search, archive job or
Shell command. What is done stays done, nothing half-written is left,
and a stopped move never loses its source.

## ARCHIVES AND DISK IMAGES
**Right** or **Return** on an lha, lzx or zip archive goes inside it,
and the pane works like a drawer: view, edit, copy and move in and out,
rename, new files and drawers, delete. **i**, **u**, **p**, **:**,
**f** and **t** are not offered inside an archive. An archive inside an
archive does not open: copy it out first.

Every change to an archive is a checked commit. CFile works on a copy
of the archive with C:lha, C:lzx or C:zip, reads the copy back with its
own reader, checks that every file is there with the right size, and
only then puts it in place of your archive. If anything goes wrong,
your archive stays as it was. From 2.0, **Esc** stops a running tool.

**u** unpacks archives (and the files of an ADF or ISO image) with
CFile's own readers. **p** packs into a new archive; the name you type
picks the kind: `.lha` or `.lzh`, `.lzx`, `.zip`. Packing needs C:lha,
C:lzx or C:zip.

**ADF images** open like drawers on every Kickstart: floppy images
(880K and 1.76MB) and hard disk images with a Rigid Disk Block (the
first DOS partition). CFile can write into OFS and FFS images
(`DOS\0` to `DOS\3`); other kinds, and a disk that needs validating,
are read only. A write that is cut off by a crash leaves a valid disk.
`n name.adf` makes a blank, bootable 880K disk image.

**DMS files**: **Return** asks, then unpacks the file to an .adf of the
same name beside it, and opens that. A DMS with a password is refused.

**ISO images** can be looked at and copied from, not written.

**o** puts an ADF into a real drive (DA0: or the next free one), so
every program can use it. This needs OS 3.2 (trackfile.device and
C:DAControl). **o** again, on the image or inside the drive, takes it
out.

## PICTURES, SOUNDS AND MUSIC
Built in, on every Kickstart: IFF pictures (ILBM and PBM, also EHB,
HAM6, HAM8 and AGA pictures; a picture the screen cannot show is
reduced), 8SVX sounds, and ProTracker modules (`M.K.`, `M!K!`, `M&K!`,
`FLT4`, `4CHN`) through CFile's own player. Any key stops a sound or a
module.

From Kickstart 3.0 every other format your system has a datatype for
works too: GIF, JPEG, PNG, WAV, AIFF and more.

## THE CONSOLE
**:** runs a Shell command with the active pane's drawer as its current
drawer. Running a program with **Return** works the same way. The
output shows inside CFile's frame. From 2.0 it shows while the command
runs and **Esc** sends it a break; on 1.3 it shows when the command
ends. Afterwards the border row shows the return code; the arrows open
the whole output in the viewer, and any other key goes back.

## DISPLAY
CFile opens its own screen in the Workbench's mode and size, with 8
colours (4 when less than 256K of chip memory is free). From 2.0 it is
a public screen named `CFILE`. If the screen cannot be opened, CFile
uses a window on Workbench, in Workbench's colours.

From 2.0, if `ENV:` or `T:` is missing, CFile makes them (`RAM:Env`,
`RAM:T`) and removes only what it made when it quits. On 1.3 its
scratch files go to RAM:.

## EXAMPLES
    CFile
    CFile Work: DH1:
    CFile Work:Games/game.lha RAM:
    CFile HIDE
    Stack 16000
    CFile

## RETURN CODES
0 fine, 20 CFile could not start (too small a stack on 1.3, not enough
memory, or no screen or window could be opened).

## NOTES
This page is for CFile 1.0 (the build says 1.0d3).

CFile never empties an existing file first. The new one is written
beside it and swapped in, so if anything fails the old file is still
there. Soft links are never followed when copying, deleting or
searching; a copied tree leaves them out and says how many.

The panes refresh by themselves from 2.0 when something changes on
disk. Some emulated drives never report changes; **F5** reads both
panes again.

One stray **Esc** does not end the session: quitting asks first. When
the hotkey works, the question also offers `(h)ide`.

## SEE ALSO
Ahiru, CTerm, MultiView, Exchange
