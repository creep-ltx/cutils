/* libraries/tui.h - tui.library: the toolkit for programs that live
 * inside the terminal (a CTerm tab or pane, or any Amiga console window).
 * No window or screen of their own: a grid of cells drawn with escape
 * sequences on Output(), keys, the mouse and pastes from Input() in raw
 * mode.
 *
 * TuiOpen() gives each program its own struct Tui; every call takes it.
 * Draw into the grid (TuiPut for Latin-1 text, TuiPutUTF8 for UTF-8,
 * TuiPutC one character, TuiFill), then TuiFlush() sends only what
 * changed. TuiWait() gives one event at a time.
 *
 * Characters: UTF-8 in CTerm. A terminal with only Latin-1 (a plain
 * console) gets the nearest it has: box lines as - | +, a full block as
 * a reversed space, shades as : and #, accented letters without the
 * accent, quotes and dashes as ' " -, arrows as < > ^ v. Wide (CJK,
 * emoji) and combining characters are ? everywhere: they would move the
 * rest of the line.
 *
 * The rule of a library: a function's place in the table never moves.
 * New calls are added at the end of tui.sfd and the version goes up. */
#ifndef LIBRARIES_TUI_H
#define LIBRARIES_TUI_H

#include <exec/types.h>
#include <dos/dos.h>

#define TUINAME    "tui.library"
#define TUIVERSION 2                /* the version this header describes */

/* colours: the ANSI sixteen (CTerm's theme colours), TC_DEFAULT the
 * terminal's own (its background picture shows through), TC_X(n) one of
 * xterm's 256, TC_RGB(r, g, b) any 24-bit colour (version 2). CTerm shows
 * them as they are; a plain console gets the nearest of its own. */
#define TC_DEFAULT 16
#define TC_X(n)    (0x100 | (n))
#define TC_RGB(r, g, b) (0x1000000 | (((r) & 255) << 16) | (((g) & 255) << 8) | ((b) & 255))
/* attributes (italic, strike and dim only where CTerm draws them) */
#define TA_BOLD    1
#define TA_UNDER   2
#define TA_REV     4
#define TA_ITAL    8
#define TA_STRIKE  16
#define TA_DIM     32

/* keys: characters are their code (Latin-1 / Unicode); the rest below.
 * The modifiers are or'ed in: TM_CTRL | 's' is Ctrl+S. */
enum {
    K_UP = 0x110000, K_DOWN, K_LEFT, K_RIGHT, K_HOME, K_END, K_PGUP, K_PGDN,
    K_INS, K_DEL, K_BS, K_TAB, K_ENTER, K_ESC, K_HELP,
    K_F1                        /* K_F1 .. K_F1 + 11 */
};
#define K_F(n)   (K_F1 + (n) - 1)
#define TM_SHIFT 0x01000000
#define TM_ALT   0x02000000
#define TM_CTRL  0x04000000
#define TM_MASK  0x07000000
#define K_BASE(k) ((k) & ~TM_MASK)

/* events */
enum { TE_NONE, TE_KEY, TE_MOUSE, TE_PASTE, TE_RESIZE,
       TE_END };                /* the terminal has gone (a CTerm tab closed): quit */
/* mouse actions */
enum { TB_PRESS = 1, TB_RELEASE, TB_DRAG, TB_WHEELUP, TB_WHEELDOWN };

typedef struct TuiEvent {
    int type;
    int key;                    /* TE_KEY */
    int mact, col, row;         /* TE_MOUSE: TB_*, the cell */
    int mods;                   /* TE_MOUSE: TM_* held */
    ULONG secs, mics;           /* when (double clicks) */
    const UBYTE *text;          /* TE_PASTE: the text (UTF-8 or Latin-1 as typed), */
    long len;                   /* valid until the next event */
} TuiEv;

/* the line-drawing characters (TuiBox): CTerm draws them itself in any
 * font; a plain console gets ASCII */
enum { BX_H, BX_V, BX_TL, BX_TR, BX_BL, BX_BR, BX_ARROW, BX_DOT };

/* one program's terminal, from TuiOpen(). Read these, never write them;
 * the library's own state follows them and is private. */
struct Tui {
    int Cols, Rows;             /* the terminal's size now */
    int CTerm;                  /* CTerm's console: colours, the mouse, UTF-8 */
    int UTF8;                   /* the terminal takes UTF-8 (CTerm, switched on) */
    int CellW, CellH;           /* a cell in pixels (CTerm), else 0 */
};

#endif
