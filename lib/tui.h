/* tui - the toolkit for programs that live inside the terminal: a CTerm
 * tab or pane, or any Amiga console window. No window or screen of
 * their own: a grid of cells drawn with escape sequences on Output(),
 * keys, the mouse and pastes from Input() in raw mode.
 *
 * Draw into the grid (tui_put, tui_fill), then tui_flush() sends only
 * what changed. tui_wait() gives one event at a time.
 * (Knowledge/amiga/programs-inside-the-terminal.md has the why.) */
#ifndef TUI_H
#define TUI_H

#include <exec/types.h>
#include <dos/dos.h>

/* colours: the ANSI sixteen (CTerm's theme colours), TC_DEFAULT the
 * terminal's own (its background picture shows through) */
#define TC_DEFAULT 16
/* attributes */
#define TA_BOLD    1
#define TA_UNDER   2
#define TA_REV     4

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
enum { TE_NONE, TE_KEY, TE_MOUSE, TE_PASTE, TE_RESIZE };
/* mouse actions */
enum { TB_PRESS = 1, TB_RELEASE, TB_DRAG, TB_WHEELUP, TB_WHEELDOWN };

typedef struct {
    int type;
    int key;                    /* TE_KEY */
    int mact, col, row;         /* TE_MOUSE: TB_*, the cell */
    int mods;                   /* TE_MOUSE: TM_* held */
    ULONG secs, mics;           /* when (double clicks) */
    const UBYTE *text;          /* TE_PASTE: the text (UTF-8 or Latin-1 as typed), */
    long len;                   /* valid until the next event */
} TuiEv;

extern int tui_cols, tui_rows;  /* the terminal's size now */
extern int tui_cterm;           /* CTerm's console: 16 colours, the mouse, UTF-8 */
extern int tui_utf8;            /* the terminal takes UTF-8 (CTerm, switched on) */
extern int tui_cellw, tui_cellh;    /* a cell in pixels (CTerm), else 0 */

void tui_console(BPTR in);      /* keys from this console, not Input() (a pipe) */
int tui_possible(void);         /* an interactive console under Input()/Output() */
int tui_open(void);             /* raw mode, own screen, size; FALSE: no terminal */
void tui_close(void);           /* everything back as it was */
void tui_suspend(void);         /* the terminal to a command for a while ... */
void tui_resume(void);          /* ... and back (all drawn again on the next flush) */
void tui_keyafter(void);        /* (suspended) a key read before going back */

/* drawing into the grid (nothing is sent until tui_flush) */
void tui_put(int col, int row, const char *s, int len, int fg, int bg, int attr);
void tui_putc(int col, int row, ULONG c, int fg, int bg, int attr);  /* one cell, a code point */
void tui_fill(int col, int row, int w, ULONG c, int fg, int bg, int attr);
void tui_clear(void);
void tui_cursor(int col, int row, int on);  /* where the terminal's cursor stands */
void tui_flush(void);
void tui_redraw(void);          /* the next flush sends everything */
int tui_resized(void);          /* after TE_RESIZE: the grid follows; TRUE if it changed */
void tui_title(const char *t);  /* the window's (tab's) title */

/* events: the next one, waiting up to us microseconds (-1: for ever,
 * 0: not at all); extra: more signals to wake for (*got says which) */
int tui_wait(TuiEv *e, long us, ULONG extra, ULONG *got);
void tui_beep(void);

/* the line-drawing characters: CTerm draws them itself in any font; a
 * plain console gets ASCII */
ULONG tui_box(int which);
enum { BX_H, BX_V, BX_TL, BX_TR, BX_BL, BX_BR, BX_ARROW, BX_DOT };

#endif
