/* tui.h - the toolkit's old tui_* names, for Sumi, Maki, Markdown, man,
 * top and ctop: they compile unchanged against this header, over
 * tui.library (opened by tui_possible / tui_open, closed by tui_close),
 * or with TUI_STATIC over the toolkit built in (libtui.a, no library).
 * Link build/libtuiglue.a (or libtuiglue_static.a + libtui.a).
 *
 * New programs may as well call the library directly (proto/tui.h):
 * t = TuiOpen(0); TuiPut(t, ...); TuiClose(t). */
#ifndef TUI_H
#define TUI_H

#include <libraries/tui.h>
#ifdef TUI_STATIC
#include <clib/tui_protos.h>
#else
#include <proto/tui.h>
#endif

extern struct Tui *tui_h;           /* the open terminal (NULL: none) */
extern const char *tui_keymsg;      /* what tui_keyafter shows (NULL: " a key goes back ") */

/* the terminal's size and kind now (80 x 24 and plain before tui_open) */
#define tui_cols  (tui_h ? tui_h->Cols : 80)
#define tui_rows  (tui_h ? tui_h->Rows : 24)
#define tui_cterm (tui_h ? tui_h->CTerm : 0)
#define tui_utf8  (tui_h ? tui_h->UTF8 : 0)
#define tui_cellw (tui_h ? tui_h->CellW : 0)
#define tui_cellh (tui_h ? tui_h->CellH : 0)

void tui_console(BPTR in);      /* keys from this console, not Input() (a pipe) */
int tui_possible(void);         /* an interactive console (and tui.library there) */
int tui_open(void);             /* raw mode, own screen, size; FALSE: no terminal */
void tui_close(void);           /* everything back as it was */

#define tui_suspend()     TuiSuspend(tui_h)
#define tui_resume()      TuiResume(tui_h)
#define tui_keyafter()    TuiKeyAfter(tui_h, (CONST_STRPTR)tui_keymsg)
#define tui_put(c, r, s, l, f, b, a)   TuiPut(tui_h, c, r, (CONST_STRPTR)(s), l, f, b, a)
#define tui_putc(c, r, ch, f, b, a)    TuiPutC(tui_h, c, r, ch, f, b, a)
#define tui_pututf8(c, r, s, l, f, b, a) TuiPutUTF8(tui_h, c, r, (CONST_STRPTR)(s), l, f, b, a)  /* version 2 */
#define tui_fill(c, r, w, ch, f, b, a) TuiFill(tui_h, c, r, w, ch, f, b, a)
#define tui_clear()       TuiClear(tui_h)
#define tui_cursor(c, r, on) TuiCursor(tui_h, c, r, on)
#define tui_flush()       TuiFlush(tui_h)
#define tui_redraw()      TuiRedraw(tui_h)
#define tui_resized()     TuiResized(tui_h)
#define tui_title(s)      TuiTitle(tui_h, (CONST_STRPTR)(s))
#define tui_wait(e, us, x, g) TuiWait(tui_h, e, us, x, g)
#define tui_beep()        TuiBeep(tui_h)
#define tui_box(w)        TuiBox(tui_h, w)
#define tui_dirty(c, r, w, h) TuiDirty(tui_h, c, r, w, h)
#define tui_raw(c, r, s, n)   TuiRaw(tui_h, c, r, (CONST_STRPTR)(s), n)

#endif
