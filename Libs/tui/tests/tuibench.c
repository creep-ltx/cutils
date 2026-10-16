/* tuibench - how fast tui.library draws, to compare its builds (000, 020,
 * 060) on one machine. Run in a terminal; prints the times after:
 *
 *   draw    the grid filled N times (TuiPut, TuiFill, TuiPutUTF8 rows),
 *           nothing sent: the library's own work only
 *   scan    one cell changed and flushed, N times: the flush finding the
 *           change in a full grid (the usual case: a cursor, a clock)
 *   full    every cell changed in 24-bit colour and flushed, N / 10
 *           times: the library and the terminal together (the terminal's
 *           drawing is most of it)
 *
 *   tuibench [N [LABEL]]   (N: the rounds, default 20; with a LABEL the
 *                          times are also added to RAM:tuibench.log) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "tui.h"

static long ticks(void)
{
    struct DateStamp d;
    DateStamp(&d);
    return (d.ds_Minute * 60L * 50L) + d.ds_Tick;
}

int main(int argc, char **argv)
{
    static const char line[] = "The quick brown fox jumps over the lazy dog 0123456789 \xe5\xe4\xf6 "
                               "The quick brown fox jumps over the lazy dog 0123456789 \xe5\xe4\xf6 "
                               "The quick brown fox jumps over the lazy dog";
    static const char u8[] = "UTF-8 \xe2\x94\x80\xe2\x94\x82 \xc5\x81\xc3\xb3" "d\xc5\xba \xe2\x86\x92 \xe2\x9c\x93 "
                             "and some plain text to make the row long enough for any width";
    int n = argc > 1 ? atoi(argv[1]) : 20, i, r, k;
    long t0, tdraw, tscan, tfull;
    int cols, rows;
    if (n < 1) n = 1;
    if (!tui_possible() || !tui_open()) {
        printf("tuibench: needs a terminal and tui.library\n");
        return 10;
    }
    cols = tui_cols;
    rows = tui_rows;

    t0 = ticks();
    for (i = 0; i < n * 10; i++)
        for (r = 0; r < rows; r++) {
            if (r % 3 == 0) tui_put(0, r, line + (i + r) % 20, cols, 7, TC_DEFAULT, 0);
            else if (r % 3 == 1) tui_fill(0, r, cols, '-', 3, TC_X(200), TA_BOLD);
            else tui_pututf8(0, r, u8, -1, TC_RGB(255, 200, 80), TC_DEFAULT, 0);
        }
    tdraw = ticks() - t0;

    tui_clear();
    tui_flush();
    t0 = ticks();
    for (i = 0; i < n * 20; i++) {
        tui_putc(i % cols, i % rows, 'A' + i % 26, 7, TC_DEFAULT, 0);
        tui_flush();
    }
    tscan = ticks() - t0;

    t0 = ticks();
    for (i = 0; i < (n + 9) / 10; i++) {
        for (r = 0; r < rows; r++)
            for (k = 0; k < cols; k++)
                tui_putc(k, r, 'a' + (k + r + i) % 26,
                         TC_RGB((k * 255) / cols, (r * 255) / rows, (i * 40) & 255), TC_DEFAULT, 0);
        tui_flush();
    }
    tfull = ticks() - t0;

    tui_close();
    printf("tuibench %d x %d, %d rounds\n", cols, rows, n);
    printf("draw  %5ld ms  (%d full grids, nothing sent)\n", tdraw * 20, n * 10);
    printf("scan  %5ld ms  (%d one-cell flushes)\n", tscan * 20, n * 20);
    printf("full  %5ld ms  (%d full-screen 24-bit flushes)\n", tfull * 20, (n + 9) / 10);
    if (argc > 2) {
        FILE *f = fopen("RAM:tuibench.log", "a");
        if (f) {
            fprintf(f, "%-8s %dx%d n=%d  draw %5ld  scan %5ld  full %5ld ms\n",
                    argv[2], cols, rows, n, tdraw * 20, tscan * 20, tfull * 20);
            fclose(f);
        }
    }
    return 0;
}
