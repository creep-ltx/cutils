/* top - the tasks that use the processor, refreshed every second.
 *
 *   top [-b] [-d SECONDS] [-n COUNT]
 *
 * The classic Unix top, inside the terminal (a CTerm tab or any console
 * window). At the top:
 *   top - 21:45:20 up 1:04, 8 shells        the time, the time since the
 *                                           start, the Shells (as uptime)
 *   Tasks: 42 total, 1 running, 2 ready, 39 waiting
 *   %Cpu: 12.0 busy, 88.0 idle              over the last interval (on a
 *                                           PiStorm with Emu68 also the
 *                                           68k instructions per second)
 *   Chip: ... total, used, free, largest    chip and fast memory, in K/M
 *   Fast: ... total, used, free, largest
 * then every task: CLI (the Shell's number), ADDRESS, PRI, STATE (run,
 * ready, wait), CPU% (its share of the processor over the interval),
 * TIME+ (its processor time since top started), STACK (its size), FREE
 * (the stack still free), COMMAND (what the Shell runs now, else [the
 * task's name], as ps shows it).
 *
 *   -b batch: plain text, no keys (also when the output is not a window:
 *      a file or a pipe)   -d SECONDS between refreshes (1; tenths too:
 *      -d 0.5)   -n COUNT refreshes, then end (batch: 1)
 *
 * Keys: P sort by CPU% (the default), M by stack size, N by name, T by
 * state; c the task's name instead of the command; e every task or only
 * the Shells; Up/Down move the bar, PgUp/PgDn/Home/End too; k break
 * (Ctrl-C) a task: a CLI number, a 0x address, or Return for the one
 * under the bar; space refresh now; d or s the delay; h help; q, Esc or
 * Ctrl-C quit.
 *
 * How CPU% is measured: the Amiga keeps no account of the time each task
 * uses, so top asks Exec to call it at every task switch (the tc_Launch /
 * tc_Switch hooks, lib/cu_cpu.c) and notes the EClock time: CPU% and TIME+ (the
 * time since top started) are exact. A task with hooks of its own, and
 * every task in a second top while the first holds the hooks, is sampled
 * instead: an interrupt server on the vertical blank (50 times a second)
 * notes the running task, and a task's share is its samples out of all
 * (its TIME+ is -). The sampler also paces the refreshes.
 *
 * Return code 0, 10 bad arguments, 20 Ctrl-C in batch mode, no memory,
 * no timer.device, or the terminal could not be opened. */
#include <stdlib.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <hardware/intbits.h>
#include <devices/timer.h>
#include <libraries/configvars.h>
#include <proto/timer.h>
#include <proto/expansion.h>
#include "cu.h"
#include "tui.h"

const char *cu_name = "top";
static const char version[] = "$VER: top 0.1b3 (16.10.2026)";

struct Device *TimerBase;

#define MAXT CU_MAXTASKS

/* ---- a snapshot ------------------------------------------------------ */

typedef struct {
    CuTask *t;
    const char *name;            /* as shown: the command or [the task] */
    const char *key;             /* for sorting by name: without [ or a path */
    int pct;                     /* tenths of a percent */
    int timed;                   /* time is exact (else CPU% is sampled) */
    unsigned long long time;     /* EClock ticks since top started */
} Row;

static CuTask tasks[MAXT];
static Row base[MAXT], rows[MAXT];   /* every task; those shown, sorted */
static int ntasks, nrows;
static CuShare share[MAXT];
static int busy = -1;            /* tenths of a percent; -1 not measured yet */
static ULONG lastclk;            /* cu_cpuclock() at the last refresh */
static ULONG mips = (ULONG)-1;
static int o_name, o_shells, sortkey = 'P';
static char names[MAXT][76];

static const char *shown(CuTask *t, char *buf)
{
    int i, n = 0;
    const char *s = t->name;
    if (!o_name && t->cmd[0]) s = t->cmd;
    else buf[n++] = '[';
    for (i = 0; s[i] && n < 70; i++) {
        unsigned char c = s[i];
        buf[n++] = c < 32 || (c >= 0x7f && c < 0xa0) ? '?' : c;
    }
    if (buf[0] == '[') buf[n++] = ']';
    buf[n] = 0;
    return buf;
}

static int bysort(const void *a, const void *b)
{
    const Row *x = a, *y = b;
    int d = 0;
    switch (sortkey) {
    case 'M': d = (y->t->stack > x->t->stack) - (y->t->stack < x->t->stack); break;
    case 'T': d = x->t->state - y->t->state; if (!d) d = y->pct - x->pct; break;
    case 'N': break;
    default: d = y->pct - x->pct; if (!d) d = x->t->state - y->t->state; break;
    }
    if (d) return d;
    return stricmp(x->key, y->key);
}

/* the tasks now, and each one's share since the last refresh */
static void snapshot(void)
{
    int k;
    ntasks = cu_tasks(tasks, MAXT);
    busy = cu_cpumeasure(tasks, ntasks, share);
    lastclk = cu_cpuclock();
    for (k = 0; k < ntasks; k++) {
        base[k].t = &tasks[k];
        base[k].name = names[k];
        base[k].pct = share[k].pct;
        base[k].timed = share[k].timed;
        base[k].time = share[k].time;
    }
    if (cu_emu68()) mips = cu_emu68mips();
}

/* the rows to show, sorted */
static void arrange(void)
{
    int k;
    for (k = 0; k < ntasks; k++) {
        shown(&tasks[k], names[k]);
        base[k].key = names[k][0] == '[' ? names[k] + 1 : (const char *)FilePart((STRPTR)names[k]);
    }
    nrows = 0;
    for (k = 0; k < ntasks; k++) if (!o_shells || base[k].t->cli) rows[nrows++] = base[k];
    qsort(rows, nrows, sizeof *rows, bysort);
}

/* ---- text into a line ------------------------------------------------- */

static char L[600];
static int Ln;

static void ls(const char *s) { while (*s && Ln < (int)sizeof L - 1) L[Ln++] = *s++; L[Ln] = 0; }
static void lc(int c) { if (Ln < (int)sizeof L - 1) L[Ln++] = c; L[Ln] = 0; }
static void lnum(long v, int w)
{
    char b[16];
    int n = 0, neg = v < 0;
    unsigned long u = neg ? -v : v;
    do b[n++] = '0' + u % 10; while ((u /= 10));
    if (neg) b[n++] = '-';
    while (n < w--) lc(' ');
    while (n) lc(b[--n]);
}
static void lpct(int t, int w) { lnum(t / 10, w - 2); lc('.'); lc('0' + t % 10); }
static void lpad(const char *s, int w) { int n = strlen(s); ls(s); while (n++ < w) lc(' '); }
static void lhex(ULONG v) { int i; for (i = 0; i < 8; i++) lc("0123456789abcdef"[(v >> (28 - 4 * i)) & 15]); }
static void ltwo(ULONG v) { lc('0' + v / 10 % 10); lc('0' + v % 10); }
/* bytes as K or M (G), one decimal under 10 */
static void lmem(ULONG v)
{
    const char *u = "KMG";
    int k = 0;
    ULONG whole = v >> 10, tenth = (v & 1023) * 10 / 1024;
    while (whole >= 1024 && k < 2) { tenth = (whole % 1024) * 10 / 1024; whole /= 1024; k++; }
    if (k && whole < 10) { lnum(whole, 0); lc('.'); lc('0' + tenth); }
    else lnum(whole, 0);
    lc(u[k]);
}

/* ---- the header -------------------------------------------------------- */

static void line_top(void)
{
    struct EClockVal ev;
    struct DateStamp now;
    ULONG freq, secs, days;
    unsigned long long t;
    int k, shells = 0;
    freq = ReadEClock(&ev);
    t = ((unsigned long long)ev.ev_hi << 32) | ev.ev_lo;
    secs = freq ? (ULONG)(t / freq) : 0;
    days = secs / 86400;
    DateStamp(&now);
    for (k = 0; k < ntasks; k++) if (tasks[k].cli) shells++;
    Ln = 0;
    ls("top - ");
    ltwo(now.ds_Minute / 60); lc(':'); ltwo(now.ds_Minute % 60); lc(':'); ltwo(now.ds_Tick / TICKS_PER_SECOND);
    ls(" up ");
    if (days) { lnum(days, 0); ls(days == 1 ? " day, " : " days, "); }
    lnum(secs / 3600 % 24, 0); lc(':'); ltwo(secs / 60 % 60);
    ls(", "); lnum(shells, 0); ls(shells == 1 ? " shell" : " shells");
}

static void line_tasks(void)
{
    int k, st[3] = { 0, 0, 0 };
    for (k = 0; k < ntasks; k++) st[tasks[k].state]++;
    Ln = 0;
    ls("Tasks: "); lnum(ntasks, 0); ls(" total, ");
    lnum(st[0], 0); ls(" running, ");
    lnum(st[1], 0); ls(" ready, ");
    lnum(st[2], 0); ls(" waiting");
}

static void line_cpu(void)
{
    Ln = 0;
    ls("%Cpu: ");
    if (busy < 0) ls("measuring");
    else { lpct(busy, 0); ls(" busy, "); lpct(1000 - busy, 0); ls(" idle"); }
    if (cu_emu68() && mips != (ULONG)-1) { ls(", "); lnum(mips, 0); ls(" MIPS"); }
}

static void line_mem(int fast)
{
    ULONG type = fast ? MEMF_FAST : MEMF_CHIP;
    ULONG tot = AvailMem(type | MEMF_TOTAL), av = AvailMem(type), big = AvailMem(type | MEMF_LARGEST);
    Ln = 0;
    ls(fast ? "Fast: " : "Chip: ");
    lmem(tot); ls(" total, ");
    lmem(tot - av); ls(" used, ");
    lmem(av); ls(" free, ");
    lmem(big); ls(" largest");
}

static void line_titles(void)
{
    Ln = 0;
    ls("CLI "); lpad("ADDRESS", 8); ls("  PRI "); lpad("STATE", 5);
    ls("  CPU%    TIME+  STACK   FREE COMMAND");
}

/* TIME+: minutes:seconds.hundredths, as Unix top (over 99 minutes no
 * hundredths); - when the task is sampled */
static void ltime(Row *r)
{
    ULONG h, m;
    if (!r->timed || !cu_cpufreq()) { ls("       -"); return; }
    h = (ULONG)(r->time * 100 / cu_cpufreq());
    m = h / 6000;
    if (m < 100) { lnum(m, 2); lc(':'); ltwo(h / 100 % 60); lc('.'); ltwo(h % 100); }
    else { lnum(m, 5); lc(':'); ltwo(h / 100 % 60); }
}

static void line_row(Row *r)
{
    CuTask *t = r->t;
    Ln = 0;
    if (t->cli) lnum(t->cli, 3); else ls("  -");
    lc(' ');
    lhex((ULONG)t->addr);
    lc(' ');
    lnum(t->pri, 4);
    lc(' ');
    lpad(t->state == 0 ? "run" : t->state == 1 ? "ready" : "wait", 5);
    lc(' ');
    lpct(r->pct, 5);
    lc(' ');
    ltime(r);
    lc(' ');
    lnum(t->stack, 6);
    lc(' ');
    if (t->free >= 0) lnum(t->free, 6); else ls("     ?");
    lc(' ');
    ls(r->name);
}

#define HEADER 5                 /* lines before the message line */

static void header(int i)
{
    switch (i) {
    case 0: line_top(); break;
    case 1: line_tasks(); break;
    case 2: line_cpu(); break;
    case 3: line_mem(0); break;
    default: line_mem(1); break;
    }
}

static int hasfast(void) { return AvailMem(MEMF_FAST | MEMF_TOTAL) != 0; }

/* ---- batch mode ------------------------------------------------------- */

/* until the ring has counted to target; 0: Ctrl-C */
static int sleepto(ULONG target)
{
    while ((LONG)(target - cu_cpuclock()) > 0) {
        if (cu_break()) return 0;
        Delay(2);
        cu_cpudrain();
    }
    return 1;
}

static int batch(int count, int delay10, int cols)
{
    int i, k;
    for (i = 0; i < count; i++) {
        if (!sleepto(lastclk + (ULONG)delay10 * cu_cpurate() / 10)) return 0;
        snapshot();
        arrange();
        if (i) cu_putc('\n');
        for (k = 0; k < HEADER; k++) {
            if (k == 4 && !hasfast()) continue;
            header(k);
            cu_put(L, Ln < cols ? Ln : cols); cu_putc('\n');
        }
        cu_putc('\n');
        line_titles(); cu_put(L, Ln < cols ? Ln : cols); cu_putc('\n');
        for (k = 0; k < nrows; k++) { line_row(&rows[k]); cu_put(L, Ln < cols ? Ln : cols); cu_putc('\n'); }
        cu_flush();
    }
    return 1;
}

/* ---- the interactive view ------------------------------------------- */

static int sel, first;           /* the bar's row, the first row shown */
static APTR seladdr;
static char msg[160];

/* L on a row, the rest of the row blank */
static void put(int row, int attr)
{
    int n = Ln < tui_cols ? Ln : tui_cols;
    tui_put(0, row, L, n, TC_DEFAULT, TC_DEFAULT, attr);
    tui_fill(n, row, tui_cols - n, ' ', TC_DEFAULT, TC_DEFAULT, attr);
}

static int listrows(void) { int n = tui_rows - (HEADER + 2); return n < 1 ? 1 : n; }

static void keepsel(void)
{
    int k, page = listrows();
    if (seladdr) for (k = 0; k < nrows; k++) if (rows[k].t->addr == seladdr) { sel = k; break; }
    if (sel >= nrows) sel = nrows - 1;
    if (sel < 0) sel = 0;
    if (sel < first) first = sel;
    if (sel >= first + page) first = sel - page + 1;
    if (first > nrows - page) first = nrows - page;
    if (first < 0) first = 0;
    seladdr = nrows ? rows[sel].t->addr : NULL;
}

static void draw(const char *prompt)
{
    int k, r;
    for (k = 0; k < HEADER && k < tui_rows; k++) {
        int lab = 0;
        header(k);
        if (k == 4 && !hasfast()) Ln = 0;
        put(k, 0);
        while (lab < Ln && L[lab] != ' ' && lab < 7) lab++;   /* the label bold */
        if (k == 0) lab = 3;
        tui_put(0, k, L, lab < tui_cols ? lab : tui_cols, TC_DEFAULT, TC_DEFAULT, TA_BOLD);
    }
    if (HEADER < tui_rows) {
        Ln = 0;
        ls(prompt ? prompt : msg);
        put(HEADER, prompt ? TA_BOLD : 0);
        if (prompt) tui_cursor(Ln < tui_cols ? Ln : tui_cols - 1, HEADER, 1);
        else tui_cursor(0, tui_rows - 1, 0);
    }
    if (HEADER + 1 < tui_rows) { line_titles(); put(HEADER + 1, TA_REV); }
    for (r = 0; HEADER + 2 + r < tui_rows; r++) {
        int at = 0, fg = TC_DEFAULT, bg = TC_DEFAULT, n;
        Ln = 0;
        if (first + r < nrows) {
            Row *w = &rows[first + r];
            line_row(w);
            if (w->t->state < 2) at = TA_BOLD;         /* running or ready */
            if (first + r == sel) {
                if (tui_cterm) { fg = 15; bg = 4; }     /* the bar: white on blue */
                else at |= TA_REV;
            }
        }
        n = Ln < tui_cols ? Ln : tui_cols;
        tui_put(0, HEADER + 2 + r, L, n, fg, bg, at);
        tui_fill(n, HEADER + 2 + r, tui_cols - n, ' ', fg, bg, at);
    }
    tui_flush();
}

static const char *const helptext[] = {
    "top 0.1b3 - the tasks that use the processor",
    "",
    "  P          sort by CPU% (the default)",
    "  M          sort by stack size",
    "  N          sort by name",
    "  T          sort by state",
    "  c          task name / command",
    "  e          every task / only the Shells",
    "  Up Down    move the bar (Shift: a page; PgUp PgDn Home End)",
    "  k          break (Ctrl-C) a task",
    "  space      refresh now",
    "  d s        the delay between refreshes",
    "  h          this help",
    "  q Esc      quit (Ctrl-C too)",
    "",
    "CPU% and TIME+ are exact: exec tells top each time a task",
    "gets and loses the processor. TIME+ counts from top's start.",
    "",
    "Any key goes back.",
    NULL
};

static int help(void)
{
    TuiEv e;
    int i;
    for (;;) {
        tui_clear();
        for (i = 0; helptext[i] && i < tui_rows; i++)
            tui_put(1, i, helptext[i], strlen(helptext[i]), TC_DEFAULT, TC_DEFAULT, i == 0 ? TA_BOLD : 0);
        tui_cursor(0, tui_rows - 1, 0);
        tui_flush();
        if (!tui_wait(&e, -1, 0, NULL)) continue;
        if (e.type == TE_RESIZE) { tui_resized(); continue; }
        if (e.type == TE_KEY) return e.key != (TM_CTRL | 'c');
    }
}

/* a line typed on the message row; 0: Esc */
static int ask(const char *prompt, char *buf, int size)
{
    TuiEv e;
    int n = 0;
    char st[200];
    buf[0] = 0;
    for (;;) {
        strcpy(st, prompt);
        strcat(st, buf);
        draw(st);
        if (!tui_wait(&e, -1, 0, NULL)) continue;
        if (e.type == TE_RESIZE) { tui_resized(); keepsel(); continue; }
        if (e.type != TE_KEY) continue;
        if (e.key == K_ENTER) return 1;
        if (e.key == K_ESC || e.key == (TM_CTRL | 'c')) return 0;
        if (e.key == K_BS || e.key == K_DEL) { if (n) buf[--n] = 0; continue; }
        if (e.key >= 32 && e.key < 127 && n < size - 1) { buf[n++] = e.key; buf[n] = 0; }
    }
}

static void killask(void)
{
    char b[24], p[140], nm[80];
    CuTask *t = NULL;
    int k;
    if (!nrows) return;
    strcpy(p, "Break which task? CLI number, 0x address, Return: ");
    strcat(p, shown(rows[sel].t, nm));
    strcat(p, " > ");
    if (!ask(p, b, sizeof b)) { msg[0] = 0; return; }
    if (!b[0]) t = rows[sel].t;
    else if (b[0] == '0' && (b[1] == 'x' || b[1] == 'X')) {
        ULONG a = 0;
        for (k = 2; b[k]; k++) {
            int c = b[k], v = c >= '0' && c <= '9' ? c - '0' : (c | 32) >= 'a' && (c | 32) <= 'f' ? (c | 32) - 'a' + 10 : -1;
            if (v < 0) break;
            a = a << 4 | v;
        }
        for (k = 0; k < ntasks; k++) if ((ULONG)tasks[k].addr == a) t = &tasks[k];
    } else {
        int ok;
        long n = cu_num(b, &ok);
        if (ok) for (k = 0; k < ntasks; k++) if (tasks[k].cli == n) t = &tasks[k];
    }
    if (!t) { strcpy(msg, "no such task"); return; }
    if (cu_signal(t->addr, SIGBREAKF_CTRL_C)) { strcpy(msg, "Ctrl-C sent to "); strcat(msg, shown(t, nm)); }
    else strcpy(msg, "that task is gone");
}

/* tenths of seconds from "1", "0.5", "2.5"; 0: not a delay */
static int tenths(const char *s)
{
    int v = 0, d = 0, frac = 0;
    for (; *s; s++) {
        if (*s >= '0' && *s <= '9') {
            if (frac) { if (frac++ == 1) d = *s - '0'; }
            else if ((v = v * 10 + *s - '0') > 3600) return 0;
        } else if ((*s == '.' || *s == ',') && !frac) frac = 1;
        else return 0;
    }
    v = v * 10 + d;
    return v;
}

static void delayask(int *delay10)
{
    char b[16], p[60];
    int d;
    Ln = 0; ls("Delay in seconds (now "); lpct(*delay10 * 10, 0); Ln -= 2; L[Ln] = 0; ls(") > ");
    strcpy(p, L);
    if (!ask(p, b, sizeof b) || !b[0]) { msg[0] = 0; return; }
    if ((d = tenths(b)) < 1) { strcpy(msg, "not a delay: "); strcat(msg, b); return; }
    *delay10 = d;
    msg[0] = 0;
}

static int interactive(int count, int delay10)
{
    TuiEv e;
    ULONG next;
    int refreshes = 0;
    tui_title("top");
    snapshot();
    arrange();
    keepsel();
    draw(NULL);
    next = lastclk + (ULONG)delay10 * cu_cpurate() / 10;
    for (;;) {
        LONG left = (LONG)(next - cu_cpuclock()), hz = cu_cpurate();
        if (left <= 0) {
            snapshot();
            arrange();
            keepsel();
            draw(NULL);
            if (count && ++refreshes >= count) return 1;
            next = lastclk + (ULONG)delay10 * cu_cpurate() / 10;
            continue;
        }
        if (left > hz / 2) left = hz / 2;    /* the hooks' ring read twice a second */
        cu_cpudrain();
        if (!tui_wait(&e, left * (1000000 / hz), 0, NULL)) continue;
        if (e.type == TE_RESIZE) { tui_resized(); keepsel(); draw(NULL); continue; }
        if (e.type == TE_MOUSE) {
            if (e.mact == TB_WHEELUP) sel -= 3;
            else if (e.mact == TB_WHEELDOWN) sel += 3;
            else if (e.mact == TB_PRESS && e.row >= HEADER + 2 && first + e.row - HEADER - 2 < nrows) sel = first + e.row - HEADER - 2;
            else continue;
            seladdr = NULL;
            keepsel();
            draw(NULL);
            continue;
        }
        if (e.type != TE_KEY) continue;
        {
            int k = e.key, page = listrows(), moved = 0, again = 0;
            msg[0] = 0;
            if (k == 'q' || k == 'Q' || k == K_ESC || k == (TM_CTRL | 'c')) return 1;
            else if (k == 'P' || k == 'p') sortkey = 'P';
            else if (k == 'M' || k == 'm') sortkey = 'M';
            else if (k == 'N' || k == 'n') sortkey = 'N';
            else if (k == 'T' || k == 't') sortkey = 'T';
            else if (k == 'c' || k == 'C') o_name = !o_name;
            else if (k == 'e' || k == 'E') { o_shells = !o_shells; strcpy(msg, o_shells ? "only the Shells" : "every task"); }
            else if (k == K_UP) { sel--; moved = 1; }
            else if (k == K_DOWN) { sel++; moved = 1; }
            else if (k == K_PGUP || k == (TM_SHIFT | K_UP)) { sel -= page; moved = 1; }
            else if (k == K_PGDN || k == (TM_SHIFT | K_DOWN)) { sel += page; moved = 1; }
            else if (k == K_HOME) { sel = 0; moved = 1; }
            else if (k == K_END) { sel = nrows - 1; moved = 1; }
            else if (k == 'k' || k == 'K') killask();
            else if (k == ' ') again = 1;
            else if (k == 'd' || k == 's') { delayask(&delay10); again = 1; }
            else if (k == 'h' || k == 'H' || k == '?' || k == K_HELP) { if (!help()) return 1; }
            else continue;
            if (moved) seladdr = NULL;
            if (again) {
                snapshot();
                next = lastclk + (ULONG)delay10 * cu_cpurate() / 10;
            }
            /* a new order or filter: the same samples, arranged again */
            arrange();
            keepsel();
            draw(NULL);
        }
    }
}

int main(void)
{
    struct timerequest *tr = NULL;
    char *v;
    int c, o_batch = 0, count = 0, delay10 = 10, cols = 1000;
    (void)version;
    if (cu_args("top [-b] [-d SECONDS] [-n COUNT]") < 0) return cu_rc;
    while ((c = cu_opt("bd:n:", &v)) >= 0) switch (c) {
        case 'b': o_batch = 1; break;
        case 'd': if ((delay10 = tenths(v)) < 1) { cu_err(10, v, "not a delay in seconds"); return cu_rc; } break;
        case 'n': { int ok; long n = cu_num(v, &ok); if (!ok || n < 1) { cu_err(10, v, "not a count"); return cu_rc; } count = n; } break;
        default: return cu_rc;
    }
    if (cu_argc) { cu_err(10, cu_argv[0], "unexpected argument"); return cu_rc; }
    if (!o_batch && !tui_possible()) o_batch = 1;
    if (o_batch) {
        char b[16];
        if (!count) count = 1;
        if (IsInteractive(Output())) {
            cols = 80;
            if (GetVar("COLUMNS", b, sizeof b, 0) > 0 && atoi(b) > 20) cols = atoi(b);
        }
    }
    if (!(tr = AllocVec(sizeof *tr, MEMF_PUBLIC | MEMF_CLEAR))) { cu_err(20, NULL, "no memory"); return cu_rc; }
    if (OpenDevice(TIMERNAME, UNIT_ECLOCK, (struct IORequest *)tr, 0)) { FreeVec(tr); cu_err(20, NULL, "no timer.device"); return cu_rc; }
    TimerBase = tr->tr_node.io_Device;
    if (!cu_cpuopen()) cu_err(20, NULL, "no memory");
    else {
        if (o_batch) batch(count, delay10, cols);
        else if (!tui_open()) cu_err(20, NULL, "can not open the terminal");
        else {
            interactive(count, delay10);
            tui_close();
        }
        cu_cpuclose();
    }
    CloseDevice((struct IORequest *)tr);
    FreeVec(tr);
    cu_flush();
    return cu_rc;
}
