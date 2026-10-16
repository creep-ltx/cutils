# tui.library

The toolkit for Amiga programs that live **inside the terminal** (a CTerm tab or pane, or any
console window), as one shared library in `LIBS:`. Sumi, Maki, Markdown and CUtils' `man`, `top`
and `ctop` each carried their own copy of `tui.c` (about 28 KB each, and they had grown apart).
With the library, one copy sits in memory however many of them run, and a fix reaches all of them.

Nothing like it existed (checked 2026-10-16): Aminet has curses ports (Simon Raybould's 1993
curses, PDCurses over SDL, ncurses over ixemul), all linked into the program or needing
ixemul/SDL. None is a shared library speaking to the console, with CTerm's colours, mouse and
UTF-8.

## What it does

| | |
|---|---|
| **Drawing** | a grid of cells; `TuiFlush` sends only what changed |
| **Colours** | the ANSI 16, xterm's 256 (`TC_X(n)`), any 24-bit colour (`TC_RGB(r, g, b)`), the terminal's own; the nearest Workbench pens in a plain console, or a pen exactly (`TC_PEN(n)`, 0.3) |
| **Attributes** | bold, underline, reverse, italic; dim and strike in CTerm |
| **Text** | `TuiPut` for Latin-1, `TuiPutUTF8` for UTF-8 (a stray Latin-1 byte still shows; returns the width), `TuiPutC` one character |
| **Characters** | UTF-8 in CTerm. A plain console (Latin-1 only) gets the nearest it has: box lines as `- \| +`, a full block as a reversed space, shades as `: #`, `Łódź` as `Lódz`, quotes and dashes as `' " -`, arrows as `< > ^ v`, `€` as `E`. Wide (CJK, emoji) and combining characters are `?` everywhere. |
| **Keys** | the Amiga console's codes and xterm's, with Shift/Alt/Ctrl; F1-F12, Help |
| **Mouse** | press, release, drag, wheel, with modifiers (CTerm) |
| **Pastes** | bracketed, handed over in one piece |
| **Size** | asked at start, followed on resize; the cell size in pixels (CTerm) |
| **Own screen** | the Shell's history stays clean; suspend/resume for running a command |
| **Pictures** | `TuiRaw` + `TuiDirty` for sixel and friends (Markdown) |
| **Waiting** | one `Wait()` for keys, Ctrl-C..F and the program's own signals; never blocks quitting. A program with a `Wait()` of its own adds `TuiSignal()` and takes events with `TuiPoll()` (0.3) |
| **The terminal gone** | `TE_END` when a CTerm tab closes, so nothing spins |

## Using it

**New programs** call the library directly:

```c
#include <proto/tui.h>
struct Library *TuiBase;

TuiBase = OpenLibrary(TUINAME, 0);  /* 0.x: also check TuiBase->lib_Revision >= 2 for TC_RGB, TuiPutUTF8 */
t = TuiOpen(0);                     /* NULL: not a terminal */
TuiPut(t, 0, 0, "hello", 5, 7, TC_DEFAULT, TA_BOLD);
TuiPutUTF8(t, 0, 1, "Łódź", -1, TC_RGB(255, 200, 80), TC_DEFAULT, 0);   /* 0.2 */
TuiFlush(t);
TuiWait(t, &ev, -1, 0, NULL);
TuiClose(t);
CloseLibrary(TuiBase);
```

**The existing programs** keep their code: `include/tui.h` maps the old `tui_*` names onto the
library, and `build/libtuiglue.a` opens it the first time it is needed (and says
"needs tui.library" if it is missing). Built with `-DTUI_STATIC` and `libtuiglue_static.a` +
`libtui.a` instead, the toolkit goes inside the program, no library needed.

Moving a program over: delete its `tui.c`/`tui.h`, add `-I<this>/include`, link
`-L<this>/build -ltuiglue`. `tui_keyafter()`'s text is `tui_keymsg` now (Sumi and Markdown set
their own name there).

## Building

`make` (bebbo's gcc in `~/opt/amiga`): the library three times, the glue and static libraries,
`build/tuitest` / `tuitest_static` and `build/tuibench`. `make check060` proves the 060 build
has no 64-bit multiply or divide.

**Which library to install** as `LIBS:tui.library`:

| File | For | Why its own |
|---|---|---|
| `build/000/tui.library` | 68000, 68010 (every Amiga) | no 32-bit multiply/divide in hardware |
| `build/020/tui.library` | 68020, 68030, 68040, Emu68 | the fastest there (a 68030 runs 020 code as it is) |
| `build/060/tui.library` | 68060 | the 020 build divides by constants with a 64-bit multiply, which the 060 only emulates (a trap each time); this one does not |

Measured with `tuibench 20` in CTerm, FS-UAE (2026-10-16), ms, two runs averaged:

| Machine | Build | Drawing | Finding changes | Full screen (mostly CTerm) |
|---|---|---|---|---|
| 68EC020 | 000 | 7970 | 6260 | 17630 |
| 68EC020 | **020** | **6470** | **6000** | 15820 |
| 68EC020 | 060 | 6660 | 6020 | 15230 |
| 68030 | 000 | 8850 | 6270 | 17800 |
| 68030 | **020** | **6900** | **6080** | 15640 |
| 68030 | 060 | 7090 | 6080 | 15470 |

The 68000 build costs a 68020/030 about a quarter of the drawing speed, so it is not the one
for them. A real 68060 was not measured (FS-UAE has none set up). After adding a call to `tui.sfd`: `make headers`.

- **The table order is for ever.** `tui.sfd` and `functable` in `src/lib.c` list the calls in
  the same order; new ones go at the end of both, and `TUI_REVISION` goes up (0.2, 0.3...;
  from 1.0 on, `TUI_VERSION`). Programs check `lib_Revision` while the version is 0.
- The library has no C library inside (`-nostartfiles -nostdlib`, its own `memcpy` and friends).
  The build fails if libnix code gets into it.
- AmigaOS 2.04+ (dos.library 37 calls); any processor, from the three builds above. Tested on the
  A600 test machine (68000, Workbench 2.05) in a plain console: drawing, keys, quitting.
- Stack: the deepest call chain is under 0.5 KB (`-fstack-usage`), so a 4 KB caller is fine.

## Files

| File | What |
|---|---|
| `tui.sfd` | the calls and their registers: the source of the headers |
| `include/libraries/tui.h` | types, constants, `struct Tui` (read-only size and kind) |
| `include/proto/`, `inline/`, `clib/` | generated by `make headers` |
| `include/tui.h` | the old `tui_*` names |
| `src/core.c` | the toolkit (merged from CUtils' and Markdown's copies, 2026-10-16) |
| `src/lib.c`, `src/start.s` | the library: Resident tag, open/close/expunge, jump table |
| `src/glue.c` | the old names' open/close, the library opened on demand |
| `tests/tuitest.c` | draws everything, logs events: `tuitest` or `tuitest SECS` |

## State

**On the A1200 since 2026-10-16 (the user's go):** `LIBS:tui.library` 0.3b1 (the 020 build, Emu68),
with C:Maki 0.1b4, C:Markdown 0.1b3, C:Sumi 0.2b4 and Work:Utils/Ahiru/Ahiru 0.3b5; the old
ones kept beside them as `-<version>.bak`. Tried there in CTerm (1280x960, 24-bit): Maki paging,
Markdown a man page, Sumi typing, Ahiru moving and a **sixel JPEG preview through `TuiRaw`**,
cleared again when the cursor moved off. That is the first sixel seen through the library.

**0.3b1, 2026-10-16:** `TuiSignal`, `TuiPoll`, `TuiKeyWaiting` (a program's own `Wait()`), `TC_PEN`.
Ahiru 0.3b5 moved onto it (`CFile/ahiru/src/term.c`).

**Programs on it (2026-10-16):** Maki 0.1b4, Markdown 0.1b3, Sumi 0.2b4 (each about 8 KB
smaller; now in `~/Projects/TextTools`), Ahiru 0.3b5, tested in FS-UAE in CTerm: Maki on a file and a pipe, Markdown viewing and
editing, Sumi saving a file. The old `tui_*` names are safe before `tui_open` and after
`tui_close`, as the built-in copy was: Markdown lays its file out first, and through the
library that call crashed (an empty `TuiBase`, guru `8000000B`) until `include/tui.h`
checked for it.

**Version numbers:** 0.x while the calls may still grow; `lib_Version` is 0 and
`lib_Revision` counts (0.1, 0.2...), so programs open version 0 and check the revision.

**0.2b2, 2026-10-16:** three builds (000, 020, 060), same code, measured above.

**0.2b1, 2026-10-16:** 24-bit colour, `TuiPutUTF8`, and the nearest character in a Latin-1
console, tested in FS-UAE: CTerm on its own 256-colour screen (a smooth 24-bit gradient), a plain
console (every stand-in above), and a 0.1 program run on the 0.2 library. The
library is 12.9 KB. **Known gap, in CTerm:** with an Amiga bitmap font CTerm draws characters
beyond Latin-1 itself, and its own stand-in table (CTerm `src/draw.c` `fallback()`) has no
accented letters or `€` yet, so `Ł Š ź €` show as `?` there (with TrueType they show). The
library sends the right UTF-8; noted for the CTerm session in CTerm's TODO.md.

**0.1b1, 2026-10-16**, tested in FS-UAE (A1200, OS 3.2.1, `amiga-test a1200-32-mui5`): in CTerm
and in a plain Shell window at the same time (each its own size and kind), keys (Ctrl, Shift,
F-keys, å), mouse clicks, quitting, the static build, a missing library (message, rc 10), and
the library thrown out by `Avail FLUSH` and loaded again. Not yet on the A1200.

**Next:** CUtils' man/top/ctop stay independent (the user, 2026-10-16: CUtils has no `LIBS:`
dependencies). They may compile `src/core.c` in statically instead of their own copy, so
**`core.c` must always build outside the library** too (`TUI_STATIC`, `build/libtui.a`). CFile has no terminal mode. Markdown's pictures through
`TuiRaw` not tried yet (Ahiru's are, on the A1200). Then grow it: keys files, themes, `ENVARC:FileColours`, lists, menus, a file picker.
