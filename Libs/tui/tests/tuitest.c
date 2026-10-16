/* tuitest - tui.library tried by hand: boxes, the colours, the
 * attributes, the size, and the last events. Built over the library
 * (tuitest) and with the toolkit inside (tuitest_static), from the same
 * source through the old tui_* names (include/tui.h).
 *
 *   tuitest        until q, Esc or the terminal goes
 *   tuitest SECS   draws, waits SECS seconds, closes, says what it saw */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "tui.h"

static char log[6][60];

static void addlog(const char *s)
{
    memmove(log[1], log[0], sizeof(log) - sizeof(log[0]));
    strncpy(log[0], s, sizeof(log[0]) - 1);
}

static void box(int x, int y, int w, int h, const char *title)
{
    int i;
    tui_putc(x, y, tui_box(BX_TL), 7, TC_DEFAULT, 0);
    tui_fill(x + 1, y, w - 2, tui_box(BX_H), 7, TC_DEFAULT, 0);
    tui_putc(x + w - 1, y, tui_box(BX_TR), 7, TC_DEFAULT, 0);
    for (i = 1; i < h - 1; i++) {
        tui_putc(x, y + i, tui_box(BX_V), 7, TC_DEFAULT, 0);
        tui_putc(x + w - 1, y + i, tui_box(BX_V), 7, TC_DEFAULT, 0);
    }
    tui_putc(x, y + h - 1, tui_box(BX_BL), 7, TC_DEFAULT, 0);
    tui_fill(x + 1, y + h - 1, w - 2, tui_box(BX_H), 7, TC_DEFAULT, 0);
    tui_putc(x + w - 1, y + h - 1, tui_box(BX_BR), 7, TC_DEFAULT, 0);
    tui_put(x + 2, y, title, strlen(title), 15, TC_DEFAULT, TA_BOLD);
}

static void draw(void)
{
    char b[80];
    int i, w = tui_cols, y;
    tui_clear();
    box(0, 0, w, tui_rows, " tuitest ");
    sprintf(b, "%d x %d  %s  %s  cell %dx%d", tui_cols, tui_rows,
            tui_cterm ? "CTerm" : "console", tui_utf8 ? "UTF-8" : "Latin-1", tui_cellw, tui_cellh);
    tui_put(2, 1, b, strlen(b), 7, TC_DEFAULT, 0);
    for (i = 0; i < 16; i++)
        tui_put(2 + i * 3, 3, "   ", 3, 7, i, 0);
    for (i = 0; i < 24 && 2 + i * 2 < w - 2; i++)
        tui_put(2 + i * 2, 4, "  ", 2, 7, TC_X(196 + i), 0);
    y = 6;
    tui_put(2, y, "bold", 4, 7, TC_DEFAULT, TA_BOLD);
    tui_put(8, y, "under", 5, 7, TC_DEFAULT, TA_UNDER);
    tui_put(15, y, "reverse", 7, 7, TC_DEFAULT, TA_REV);
    tui_put(24, y, "italic", 6, 7, TC_DEFAULT, TA_ITAL);
    tui_put(32, y, "dim", 3, 7, TC_DEFAULT, TA_DIM);
    tui_put(37, y, "strike", 6, 7, TC_DEFAULT, TA_STRIKE);
    { static const char l1[] = "Latin-1: \xe5\xe4\xf6 \xc5\xc4\xd6 \xe9"; tui_put(2, 7, l1, sizeof(l1) - 1, 6, TC_DEFAULT, 0); }
    tui_putc(22, 7, 0x2588, 2, TC_DEFAULT, 0);         /* a block: CTerm draws it */
    tui_putc(23, 7, 0x2591, 2, TC_DEFAULT, 0);
    for (i = 0; i < 6 && 9 + i < tui_rows - 2; i++)
        tui_put(2, 9 + i, log[i], strlen(log[i]), i ? 8 : 11, TC_DEFAULT, 0);
    tui_put(2, tui_rows - 2, "q or Esc: quit", 14, 8, TC_DEFAULT, 0);
}

int main(int argc, char **argv)
{
    TuiEv e;
    long secs = argc > 1 ? atol(argv[1]) : -1;
    char b[60];
    if (!tui_possible()) {
        printf("tuitest: not in an interactive console (or no tui.library)\n");
        return 10;
    }
    if (!tui_open()) {
        printf("tuitest: the terminal would not open\n");
        return 10;
    }
    tui_title("tuitest");
    addlog("press keys, click, paste, resize");
    draw();
    for (;;) {
        tui_flush();
        if (!tui_wait(&e, secs >= 0 ? secs * 1000000 : -1, 0, NULL)) {
            if (secs >= 0) break;
            continue;
        }
        if (e.type == TE_END) break;
        if (e.type == TE_KEY && (e.key == 'q' || e.key == K_ESC)) break;
        switch (e.type) {
        case TE_KEY: sprintf(b, "key 0x%08x", e.key); break;
        case TE_MOUSE: sprintf(b, "mouse %d at %d,%d mods %x", e.mact, e.col, e.row, e.mods); break;
        case TE_PASTE: sprintf(b, "paste of %ld bytes", e.len); break;
        case TE_RESIZE: tui_resized(); sprintf(b, "resized to %d x %d", tui_cols, tui_rows); break;
        default: sprintf(b, "event %d", e.type);
        }
        addlog(b);
        draw();
    }
    sprintf(b, "%d x %d %s", tui_cols, tui_rows, tui_cterm ? "cterm" : "console");
    tui_close();
    printf("tuitest: ok, %s\n", b);
    return 0;
}
