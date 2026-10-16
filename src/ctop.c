/* ctop - btop for the Amiga: the processor, memory, disks, network and
 * every task, with graphs, inside the terminal (a CTerm tab or pane, or
 * any console window).
 *
 *   ctop [-d SECONDS]
 *
 * Four boxes, as btop (docs/ctop.md has the design and what each number
 * comes from):
 *   1 cpu   how busy the processor is, as a history graph and a meter
 *           (exact: exec's task hooks, lib/cu_cpu.c), the CPU, 68k MIPS
 *           and the Pi's temperature and clock under Emu68, the uptime
 *   2 mem   chip and fast memory with meters and graphs; the volumes
 *   3 net   bytes in and out a second (the stack's own counters,
 *           Roadshow's API: lwip-amiga and Roadshow have it)
 *   4 proc  every task: CLI, name or command, priority, state, free
 *           stack, CPU% with a small graph, TIME+
 * Graphs are braille (CTerm draws it in any font); a plain console gets
 * ASCII. It fits itself to the window: all four boxes from about 70 x 24
 * (a Shell window on PAL hires), the cpu and proc boxes down to 40 x 12.
 *
 * Keys: every one can be changed in ENVARC:ctop.keys (written with the
 * built-in ones when it is missing; h shows what is bound).
 *
 * Return code 0, 10 bad arguments, 20 no terminal, no memory or no
 * timer.device. */
#include <stdlib.h>
#include <string.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <devices/timer.h>
#include <dos/dosextens.h>
#include <utility/tagitem.h>
#include <proto/timer.h>
#include "cu.h"
#include "tui.h"

const char *cu_name = "ctop";
static const char version[] = "$VER: ctop 0.1b1 (16.10.2026)";

struct Device *TimerBase;

#define MAXT CU_MAXTASKS
#define HIST 512                 /* values kept for each graph */
#define MINW 40                  /* the smallest window it draws in */
#define MINH 12

/* ---- colours: btop's default theme, as near as the 256 go ---------- */
#define C_TEXT   TC_X(252)
#define C_DIM    TC_X(244)
#define C_TITLE  TC_X(255)
#define C_HOT    TC_X(167)
#define C_CPUBOX TC_X(65)
#define C_MEMBOX TC_X(101)
#define C_NETBOX TC_X(61)
#define C_PROBOX TC_X(95)
#define C_SELBG  TC_X(88)
#define C_EMPTY  TC_X(238)
static const int g_cpu[] = { TC_X(114), TC_X(150), TC_X(186), TC_X(180), TC_X(173), TC_X(167) };
static const int g_down[] = { TC_X(60), TC_X(61), TC_X(62), TC_X(104), TC_X(141), TC_X(147) };
static const int g_up[] = { TC_X(89), TC_X(127), TC_X(133), TC_X(169), TC_X(176), TC_X(183) };

/* ---- the numbers ------------------------------------------------------- */

typedef struct { int v[HIST]; int n, head; } Hist;    /* tenths of a percent, or any */

static void hpush(Hist *h, int v)
{
    h->head = (h->head + 1) % HIST;
    h->v[h->head] = v;
    if (h->n < HIST) h->n++;
}

static int hget(const Hist *h, int back)    /* back 0: the newest; -1 none */
{
    if (back >= h->n) return -1;
    return h->v[(h->head - back + HIST) % HIST];
}

static Hist hcpu, hchip, hfast, hdown, hup;
static int busy = -1;
static ULONG mips = (ULONG)-1;
static CuPi pi;
static int haspi;

static CuTask tasks[MAXT];
static CuShare share[MAXT];
static int ntasks;

/* every task's last CPU% values for its small graph, by address */
#define NPH 512
typedef struct { APTR addr; UBYTE v[10]; } PHist;   /* percent, newest at v[0] */
static PHist ph[NPH], ph2[NPH];

static PHist *phfor(PHist *h, APTR a, int add)
{
    int i = ((ULONG)a >> 4) & (NPH - 1), n;
    for (n = 0; n < NPH; n++, i = (i + 1) & (NPH - 1)) {
        if (h[i].addr == a) return &h[i];
        if (!h[i].addr) {
            if (!add) return NULL;
            memset(&h[i], 0, sizeof h[i]);
            h[i].addr = a;
            return &h[i];
        }
    }
    return NULL;
}

/* memory */
static ULONG chiptot, chipfree, chipbig, fasttot, fastfree, fastbig;

static void readmem(void)
{
    chiptot = AvailMem(MEMF_CHIP | MEMF_TOTAL);
    chipfree = AvailMem(MEMF_CHIP);
    chipbig = AvailMem(MEMF_CHIP | MEMF_LARGEST);
    fasttot = AvailMem(MEMF_FAST | MEMF_TOTAL);
    fastfree = AvailMem(MEMF_FAST);
    fastbig = AvailMem(MEMF_FAST | MEMF_LARGEST);
    hpush(&hchip, chiptot ? (int)((unsigned long long)(chiptot - chipfree) * 1000 / chiptot) : 0);
    hpush(&hfast, fasttot ? (int)((unsigned long long)(fasttot - fastfree) * 1000 / fasttot) : 0);
}

/* the volumes: read every 10 seconds (Info() can be slow on a network
 * volume), requesters off */
#define MAXVOL 24
typedef struct { char name[32]; ULONG kb, usedkb; } Vol;
static Vol vols[MAXVOL];
static int nvols;

static ULONG kbof(ULONG blocks, ULONG bpb)
{
    if (!bpb) return 0;
    return bpb >= 1024 ? blocks * (bpb / 1024) : blocks / (1024 / bpb);
}

static void readvols(void)
{
    char *names[MAXVOL];
    int n = 0, i;
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR win = me->pr_WindowPtr;
    struct InfoData *id = AllocVec(sizeof *id, MEMF_PUBLIC | MEMF_CLEAR);
    struct DosList *dl;
    if (!id) return;
    dl = LockDosList(LDF_VOLUMES | LDF_READ);
    while ((dl = NextDosEntry(dl, LDF_VOLUMES)) && n < MAXVOL) {
        UBYTE *b = BADDR(dl->dol_Name);
        if (!dl->dol_Task || !b || b[0] > 29) continue;
        if ((names[n] = AllocVec(b[0] + 2, MEMF_ANY))) {
            memcpy(names[n], b + 1, b[0]);
            names[n][b[0]] = ':';
            names[n][b[0] + 1] = 0;
            n++;
        }
    }
    UnLockDosList(LDF_VOLUMES | LDF_READ);
    me->pr_WindowPtr = (APTR)-1;
    nvols = 0;
    for (i = 0; i < n; i++) {
        BPTR l = Lock((STRPTR)names[i], SHARED_LOCK);
        if (l) {
            if (Info(l, id) && id->id_NumBlocks) {
                Vol *v = &vols[nvols++];
                strcpy(v->name, names[i]);
                v->name[strlen(v->name) - 1] = 0;
                v->kb = kbof(id->id_NumBlocks, id->id_BytesPerBlock);
                v->usedkb = kbof(id->id_NumBlocksUsed, id->id_BytesPerBlock);
                if (v->usedkb > v->kb) v->usedkb = v->kb;
            }
            UnLock(l);
        }
        FreeVec(names[i]);
    }
    me->pr_WindowPtr = win;
    FreeVec(id);
    /* by name */
    for (i = 1; i < nvols; i++) {
        Vol t = vols[i];
        int j = i;
        while (j > 0 && stricmp(vols[j - 1].name, t.name) > 0) { vols[j] = vols[j - 1]; j--; }
        vols[j] = t;
    }
}

/* ---- the network: the stack's byte counters (Roadshow's API) ---------- */

struct Library *SocketBase;
static int netok;                /* the counters answer */
static unsigned long long netrx, nettx, rxrate, txrate, rxmax = 1, txmax = 1;
static unsigned long long netlast;   /* EClock at the last reading */
static char netip[20], netif[24];

#define SBTM_GETREF(code) (TAG_USER | 0x8000UL | ((ULONG)(code) << 1))
#define SBTC_GET_BYTES_RECEIVED 64
#define SBTC_GET_BYTES_SENT 65
#define IFQ_BASE (TAG_USER + 1900)
#define IFQ_Address (IFQ_BASE + 14)

static LONG sbtags(struct TagItem *t)
{
    register struct Library *a6 __asm("a6") = SocketBase;
    register struct TagItem *a0 __asm("a0") = t;
    register LONG d0 __asm("d0");
    register ULONG d1 __asm("d1");
    register ULONG a1 __asm("a1");
    __asm volatile("jsr -294(a6)" : "=r"(d0), "=r"(d1), "+r"(a0), "=r"(a1) : "r"(a6) : "cc", "memory");
    return d0;
}

static struct List *iflist(void)
{
    register struct Library *a6 __asm("a6") = SocketBase;
    register struct List *d0 __asm("d0");
    register ULONG d1 __asm("d1"), a0 __asm("a0"), a1 __asm("a1");
    __asm volatile("jsr -462(a6)" : "=r"(d0), "=r"(d1), "=r"(a0), "=r"(a1) : "r"(a6) : "cc", "memory");
    return d0;
}

static void iffree(struct List *l)
{
    register struct Library *a6 __asm("a6") = SocketBase;
    register struct List *a0 __asm("a0") = l;
    register ULONG d0 __asm("d0"), d1 __asm("d1"), a1 __asm("a1");
    __asm volatile("jsr -456(a6)" : "=r"(d0), "=r"(d1), "+r"(a0), "=r"(a1) : "r"(a6) : "cc", "memory");
}

static LONG ifquery(const char *name, struct TagItem *t)
{
    register struct Library *a6 __asm("a6") = SocketBase;
    register const char *a0 __asm("a0") = name;
    register struct TagItem *a1 __asm("a1") = t;
    register LONG d0 __asm("d0");
    register ULONG d1 __asm("d1");
    __asm volatile("jsr -468(a6)" : "=r"(d0), "=r"(d1), "+r"(a0), "+r"(a1) : "r"(a6) : "cc", "memory");
    return d0;
}

static unsigned long long eclock(ULONG *freq)
{
    struct EClockVal ev;
    ULONG f = ReadEClock(&ev);
    if (freq) *freq = f;
    return ((unsigned long long)ev.ev_hi << 32) | ev.ev_lo;
}

static int netcounters(unsigned long long *rx, unsigned long long *tx)
{
    ULONG q[4] = { 0, 0, 0, 0 };     /* SBQUAD_Ts: high, low */
    struct TagItem t[3];
    t[0].ti_Tag = SBTM_GETREF(SBTC_GET_BYTES_RECEIVED); t[0].ti_Data = (ULONG)&q[0];
    t[1].ti_Tag = SBTM_GETREF(SBTC_GET_BYTES_SENT); t[1].ti_Data = (ULONG)&q[2];
    t[2].ti_Tag = TAG_END;
    if (sbtags(t) != 0) return 0;
    *rx = ((unsigned long long)q[0] << 32) | q[1];
    *tx = ((unsigned long long)q[2] << 32) | q[3];
    return 1;
}

/* the first interface that is not the loopback, and its address */
static void netaddress(void)
{
    struct List *l;
    struct Node *n;
    netip[0] = netif[0] = 0;
    if (!(l = iflist())) return;
    for (n = l->lh_Head; n->ln_Succ; n = n->ln_Succ) {
        UBYTE sa[16];
        struct TagItem t[2];
        if (!n->ln_Name || !strnicmp(n->ln_Name, "lo", 2)) continue;
        memset(sa, 0, sizeof sa);
        t[0].ti_Tag = IFQ_Address; t[0].ti_Data = (ULONG)sa;
        t[1].ti_Tag = TAG_END;
        strncpy(netif, n->ln_Name, sizeof netif - 1);
        if (ifquery(n->ln_Name, t) == 0 && (sa[4] | sa[5] | sa[6] | sa[7])) {
            char *p = netip;
            int i;
            for (i = 4; i < 8; i++) {
                int v = sa[i];
                if (v >= 100) *p++ = '0' + v / 100;
                if (v >= 10) *p++ = '0' + v / 10 % 10;
                *p++ = '0' + v % 10;
                if (i < 7) *p++ = '.';
            }
            *p = 0;
            break;
        }
    }
    iffree(l);
}

static void netopen(void)
{
    if (!(SocketBase = OpenLibrary("bsdsocket.library", 4))) return;
    if (!netcounters(&netrx, &nettx)) return;
    netok = 1;
    netlast = eclock(NULL);
    if (SocketBase->lib_Version >= 4) netaddress();
}

static void netread(void)
{
    unsigned long long rx, tx, now, dt;
    ULONG f;
    if (!netok) return;
    if (!netcounters(&rx, &tx)) return;
    now = eclock(&f);
    dt = now - netlast;
    if (dt && rx >= netrx && tx >= nettx) {
        rxrate = (rx - netrx) * f / dt;
        txrate = (tx - nettx) * f / dt;
    }
    netrx = rx;
    nettx = tx;
    netlast = now;
    hpush(&hdown, rxrate > 0x7fffffff ? 0x7fffffff : (int)rxrate);
    hpush(&hup, txrate > 0x7fffffff ? 0x7fffffff : (int)txrate);
}

/* ---- text into a buffer ------------------------------------------------- */

static char *snum(char *p, unsigned long v)
{
    char b[12];
    int n = 0;
    do b[n++] = '0' + v % 10; while ((v /= 10));
    while (n) *p++ = b[--n];
    *p = 0;
    return p;
}

/* bytes as 123B 1.5K 12K 1.5M ...: at most 5 characters */
static char *sbytes(char *p, unsigned long long v)
{
    static const char u[] = "BKMGT";
    int k = 0;
    unsigned long long whole = v, frac = 0;
    while (whole >= 1000 && k < 4) { frac = (whole % 1024) * 10 / 1024; whole /= 1024; k++; }
    p = snum(p, (unsigned long)whole);
    if (k && whole < 10) { *p++ = '.'; *p++ = '0' + (int)frac; }
    *p++ = u[k];
    *p = 0;
    return p;
}

static char *spct(char *p, int tenths)    /* "12%" */
{
    p = snum(p, (tenths + 5) / 10);
    *p++ = '%';
    *p = 0;
    return p;
}

static char *sstr(char *p, const char *s) { while (*s) *p++ = *s++; *p = 0; return p; }
static char *stwo(char *p, int v) { *p++ = '0' + v / 10 % 10; *p++ = '0' + v % 10; *p = 0; return p; }

/* ---- drawing ------------------------------------------------------------ */

static int fancy;                /* CTerm with UTF-8: braille, blocks, round boxes */

static void text(int x, int y, const char *s, int fg, int attr)
{
    tui_put(x, y, s, strlen(s), fg, TC_DEFAULT, attr);
}

static void rtext(int x, int y, const char *s, int fg, int attr)    /* ending before x */
{
    int n = strlen(s);
    tui_put(x - n, y, s, n, fg, TC_DEFAULT, attr);
}

static void box(int x, int y, int w, int h, int col)
{
    int i;
    if (w < 2 || h < 2) return;
    tui_putc(x, y, tui_box(BX_TL), col, TC_DEFAULT, 0);
    tui_putc(x + w - 1, y, tui_box(BX_TR), col, TC_DEFAULT, 0);
    tui_putc(x, y + h - 1, tui_box(BX_BL), col, TC_DEFAULT, 0);
    tui_putc(x + w - 1, y + h - 1, tui_box(BX_BR), col, TC_DEFAULT, 0);
    tui_fill(x + 1, y, w - 2, tui_box(BX_H), col, TC_DEFAULT, 0);
    tui_fill(x + 1, y + h - 1, w - 2, tui_box(BX_H), col, TC_DEFAULT, 0);
    for (i = 1; i < h - 1; i++) {
        tui_putc(x, y + i, tui_box(BX_V), col, TC_DEFAULT, 0);
        tui_putc(x + w - 1, y + i, tui_box(BX_V), col, TC_DEFAULT, 0);
        tui_fill(x + 1, y + i, w - 2, ' ', C_TEXT, TC_DEFAULT, 0);
    }
}

/* a title in a border: ┐label┌ with the hot letter (index, -1 none) in
 * red; key: a digit shown raised before it (0 none). The width used. */
static int title(int x, int y, const char *s, int hot, int key, int col)
{
    int n = strlen(s), i, w = 0;
    tui_putc(x, y, fancy ? 0x2510 : '[', col, TC_DEFAULT, 0);
    w++;
    if (key) {
        static const ULONG sup[] = { 0, 0xB9, 0xB2, 0xB3 };   /* Latin-1 has no raised 4 */
        ULONG c = key < 4 ? sup[key] : '0' + key;
        tui_putc(x + w++, y, c, C_HOT, TC_DEFAULT, 0);
    }
    for (i = 0; i < n; i++)
        tui_putc(x + w++, y, (unsigned char)s[i], i == hot ? C_HOT : C_TITLE, TC_DEFAULT, TA_BOLD);
    tui_putc(x + w++, y, fancy ? 0x250C : ']', col, TC_DEFAULT, 0);
    return w;
}

static int grad(const int *g, int v, int max)    /* a colour for v of max */
{
    int i = max > 0 ? v * 6 / (max + 1) : 0;
    if (i < 0) i = 0;
    if (i > 5) i = 5;
    return g[i];
}

/* a meter: w cells, filled to tenths of 1000 */
static void meter(int x, int y, int w, int tenths, const int *g)
{
    int i, f = (tenths * w + 500) / 1000;
    if (tenths > 0 && !f) f = 1;
    for (i = 0; i < w; i++) {
        if (i < f) tui_putc(x + i, y, fancy ? 0x25A0 : '#', grad(g, i, w - 1), TC_DEFAULT, 0);
        else tui_putc(x + i, y, fancy ? 0x25A0 : '-', C_EMPTY, TC_DEFAULT, 0);
    }
}

/* a graph: w x h cells, the newest value at the right; each braille cell
 * holds two values, four dots high. vals: the history, max: full height.
 * down: grows from the top (the upload half of the net graph). */
static void graph(int x, int y, int w, int h, const Hist *hs, int max, const int *g, int down)
{
    int c, r, dots = h * 4;
    static const UBYTE lbit[4] = { 0x40, 0x04, 0x02, 0x01 };   /* from the bottom: dots 7 3 2 1 */
    static const UBYTE rbit[4] = { 0x80, 0x20, 0x10, 0x08 };   /* dots 8 6 5 4 */
    if (w < 1 || h < 1) return;
    if (max < 1) max = 1;
    for (c = 0; c < w; c++) {
        int back = (w - 1 - c) * 2, vl = hget(hs, back + 1), vr = hget(hs, back), hl, hr;
        hl = vl < 0 ? -1 : (int)((long long)vl * dots / max);
        hr = vr < 0 ? -1 : (int)((long long)vr * dots / max);
        if (vl > 0 && hl < 1) hl = 1;
        if (vr > 0 && hr < 1) hr = 1;
        if (hl > dots) hl = dots;
        if (hr > dots) hr = dots;
        for (r = 0; r < h; r++) {
            int level = down ? r : h - 1 - r;   /* cells from the base */
            int fl = hl - level * 4, fr = hr - level * 4, k, ch;
            UBYTE bits = 0;
            if (fl > 4) fl = 4;
            if (fr > 4) fr = 4;
            if (!fancy) {
                int f = fl > fr ? fl : fr;
                ch = f <= 0 ? ' ' : f >= 4 ? '#' : f >= 3 ? ':' : '.';
                if (f <= 0 && level == 0 && (vl >= 0 || vr >= 0)) ch = '_';
                tui_putc(x + c, y + r, ch, grad(g, level, h - 1), TC_DEFAULT, 0);
                continue;
            }
            for (k = 0; k < 4; k++) {
                int d = down ? 3 - k : k;   /* down: fill from the top of the cell */
                if (k < fl) bits |= lbit[d];
                if (k < fr) bits |= rbit[d];
            }
            if (!bits && level == 0 && (vl >= 0 || vr >= 0)) bits = down ? 0x09 : 0xC0;   /* the floor: a dot each */
            if (!bits) tui_putc(x + c, y + r, ' ', C_TEXT, TC_DEFAULT, 0);
            else tui_putc(x + c, y + r, 0x2800 + bits, bits == 0xC0 || bits == 0x09 ? C_EMPTY : grad(g, level, h - 1), TC_DEFAULT, 0);
        }
    }
}

/* ---- the keys ------------------------------------------------------------ */

enum {
    A_NONE, A_QUIT, A_HELP, A_UP, A_DOWN, A_PGUP, A_PGDN, A_HOME, A_END, A_INFO,
    A_BREAK, A_SIGNAL, A_PRI, A_SORTL, A_SORTR, A_REVERSE, A_FILTER, A_SHELLS,
    A_NAMES, A_BOX1, A_BOX2, A_BOX3, A_BOX4, A_FASTER, A_SLOWER, A_PAUSE, A_REFRESH, NACT
};
static const char *const actname[NACT] = {
    "none", "quit", "help", "up", "down", "pageup", "pagedown", "home", "end", "info",
    "break", "signal", "priority", "sortleft", "sortright", "reverse", "filter", "shells",
    "names", "box1", "box2", "box3", "box4", "faster", "slower", "pause", "refresh"
};
static const char *const actdesc[NACT] = {
    "", "quit", "this help", "select up", "select down", "a page up", "a page down",
    "the first task", "the last task", "details of the task", "break it (Ctrl-C)",
    "send Ctrl-C, D, E or F", "change its priority", "sort by the column left",
    "sort by the column right", "reverse the order", "show only names with ...",
    "every task / only the Shells", "task name / command", "show / hide cpu",
    "show / hide mem", "show / hide net", "show / hide proc", "refresh faster",
    "refresh slower", "pause / go on", "refresh now"
};
static const char defkeys[] =
    "; ctop's keys: KEY ACTION, one a line. KEY is a character or a name:\n"
    "; <Up> <Down> <Left> <Right> <PgUp> <PgDn> <Home> <End> <Return> <Esc>\n"
    "; <Tab> <Space> <Del> <BS> <Help> <F1>..<F10>, <C-x> for Ctrl, <S-Up> for Shift.\n"
    "; ACTION none takes a key away. h in ctop lists what is bound.\n"
    "q quit\n<Esc> quit\n<C-c> quit\nh help\n? help\n<Help> help\n"
    "<Up> up\nk up\n<Down> down\nj down\n<PgUp> pageup\n<S-Up> pageup\n<PgDn> pagedown\n<S-Down> pagedown\n"
    "<Home> home\ng home\n<End> end\nG end\n<Return> info\n"
    "t break\nK break\ns signal\nN priority\nn priority\n<Left> sortleft\n<Right> sortright\n"
    "r reverse\nf filter\n/ filter\ne shells\nc names\n"
    "1 box1\n2 box2\n3 box3\n4 box4\n+ faster\n- slower\n<Space> pause\n<C-l> refresh\n";

#define MAXBIND 96
static struct { int key, act; } binds[MAXBIND];
static int nbinds;
static char keybad[120];

static const struct { const char *n; int k; } keynames[] = {
    { "Up", K_UP }, { "Down", K_DOWN }, { "Left", K_LEFT }, { "Right", K_RIGHT },
    { "PgUp", K_PGUP }, { "PgDn", K_PGDN }, { "Home", K_HOME }, { "End", K_END },
    { "Return", K_ENTER }, { "Enter", K_ENTER }, { "Esc", K_ESC }, { "Tab", K_TAB },
    { "Space", ' ' }, { "Del", K_DEL }, { "BS", K_BS }, { "Help", K_HELP }, { NULL, 0 }
};

static int parsekey(const char *s, int len)
{
    int mods = 0, i;
    if (len == 1) return (unsigned char)s[0];
    if (len < 3 || s[0] != '<' || s[len - 1] != '>') return -1;
    s++;
    len -= 2;
    while (len > 2 && s[1] == '-' && (s[0] == 'C' || s[0] == 'S' || s[0] == 'A')) {
        mods |= s[0] == 'C' ? TM_CTRL : s[0] == 'S' ? TM_SHIFT : TM_ALT;
        s += 2;
        len -= 2;
    }
    if (len == 1) return mods | (unsigned char)s[0];
    if ((s[0] == 'F' || s[0] == 'f') && len <= 3 && s[1] >= '1' && s[1] <= '9') {
        int n = s[1] - '0';
        if (len == 3) n = n * 10 + s[2] - '0';
        if (n >= 1 && n <= 12) return mods | K_F(n);
        return -1;
    }
    for (i = 0; keynames[i].n; i++)
        if ((int)strlen(keynames[i].n) == len && !strnicmp(keynames[i].n, s, len)) return mods | keynames[i].k;
    return -1;
}

static void bindline(const char *s, int len)
{
    int i = 0, ks, kl, as, al, k, a, j;
    while (i < len && (s[i] == ' ' || s[i] == '\t')) i++;
    if (i >= len || s[i] == ';' || s[i] == '#') return;
    ks = i;
    while (i < len && s[i] != ' ' && s[i] != '\t') i++;
    kl = i - ks;
    while (i < len && (s[i] == ' ' || s[i] == '\t')) i++;
    as = i;
    while (i < len && s[i] != ' ' && s[i] != '\t' && s[i] != '\r') i++;
    al = i - as;
    k = parsekey(s + ks, kl);
    for (a = 0; a < NACT; a++) if ((int)strlen(actname[a]) == al && !strnicmp(actname[a], s + as, al)) break;
    if (k < 0 || a == NACT) {
        if (!keybad[0]) {
            int n = len < 60 ? len : 60;
            strcpy(keybad, "ctop.keys: not understood: ");
            strncat(keybad, s, n);
        }
        return;
    }
    for (j = 0; j < nbinds; j++) if (binds[j].key == k) { binds[j].act = a; return; }
    if (nbinds < MAXBIND) { binds[nbinds].key = k; binds[nbinds].act = a; nbinds++; }
}

static void bindtext(const char *t, long n)
{
    long i = 0, s;
    while (i < n) {
        s = i;
        while (i < n && t[i] != '\n') i++;
        bindline(t + s, (int)(i - s));
        i++;
    }
}

static void loadkeys(void)
{
    BPTR f;
    bindtext(defkeys, strlen(defkeys));
    if ((f = Open((STRPTR)"ENVARC:ctop.keys", MODE_OLDFILE))) {
        static char buf[4096];
        long n = Read(f, buf, sizeof buf - 1);
        Close(f);
        if (n > 0) { nbinds = 0; bindtext(buf, n); }
    } else if ((f = Open((STRPTR)"ENVARC:ctop.keys", MODE_NEWFILE))) {
        Write(f, (APTR)defkeys, strlen(defkeys));
        Close(f);
    }
}

static int action(int key)
{
    int i;
    for (i = 0; i < nbinds; i++) if (binds[i].key == key) return binds[i].act;
    if (key == (TM_CTRL | 'c')) return A_QUIT;       /* a way out always */
    return A_NONE;
}

static void keyname(int k, char *d)
{
    int i, mods = k & TM_MASK;
    k = K_BASE(k);
    if (!mods && k > 32 && k < 127) { d[0] = k; d[1] = 0; return; }
    *d++ = '<';
    if (mods & TM_CTRL) { *d++ = 'C'; *d++ = '-'; }
    if (mods & TM_SHIFT) { *d++ = 'S'; *d++ = '-'; }
    if (mods & TM_ALT) { *d++ = 'A'; *d++ = '-'; }
    for (i = 0; keynames[i].n; i++) if (keynames[i].k == k) { d = sstr(d, keynames[i].n); break; }
    if (!keynames[i].n) {
        if (k >= K_F1 && k < K_F1 + 12) { *d++ = 'F'; d = snum(d, k - K_F1 + 1); }
        else *d++ = k;
    }
    *d++ = '>';
    *d = 0;
}

static void firstkey(int act, char *d)    /* the first key for act, "" none */
{
    int i;
    d[0] = 0;
    for (i = 0; i < nbinds; i++) if (binds[i].act == act) { keyname(binds[i].key, d); return; }
}

/* ---- the view ------------------------------------------------------------ */

static int show[4] = { 1, 1, 1, 1 };
static int delay10 = 10, paused;
static int sel, top;             /* the selected row, the first shown */
static APTR seladdr;
static int o_shells, o_names, sortcol = 4, reverse;   /* sortcol: see cols[] */
static char filter[32];
static char msg[100];
static int info;                 /* the details shown */

static const char *const sortnames[] = { "cli", "name", "pri", "state", "cpu", "time", "stack" };
#define NSORT 7

typedef struct {
    CuTask *t;
    CuShare *s;
    char name[72];               /* as shown */
} Row;

/* what sorts by name: without the [ or the path. Worked out from the row
 * itself on every comparison - a pointer kept in the row would point
 * into another row once qsort has moved them, and a comparison that
 * changes its mind lets a quicksort run off the end of the array */
static const char *keyof(const Row *r)
{
    return r->name[0] == '[' ? r->name + 1 : (const char *)FilePart((STRPTR)r->name);
}
static Row rows[MAXT];
static int nrows;

static void taskname(CuTask *t, char *buf)
{
    int i, n = 0;
    const char *s = t->name;
    if (!o_names && t->cmd[0]) s = t->cmd;
    else buf[n++] = '[';
    for (i = 0; s[i] && n < 68; i++) {
        unsigned char c = s[i];
        buf[n++] = c < 32 || (c >= 0x7f && c < 0xa0) ? '?' : c;
    }
    if (buf[0] == '[') buf[n++] = ']';
    buf[n] = 0;
}

static int bysort(const void *a, const void *b)
{
    const Row *x = a, *y = b;
    int d = 0;
    switch (sortcol) {
    case 0: d = (x->t->cli ? x->t->cli : 9999) - (y->t->cli ? y->t->cli : 9999); break;
    case 1: break;
    case 2: d = y->t->pri - x->t->pri; break;
    case 3: d = x->t->state - y->t->state; break;
    case 4: d = y->s->pct - x->s->pct; if (!d) d = x->t->state - y->t->state; break;
    case 5: d = (y->s->time > x->s->time) - (y->s->time < x->s->time); break;
    case 6: d = (y->t->stack > x->t->stack) - (y->t->stack < x->t->stack); break;
    }
    if (reverse) d = -d;
    if (d) return d;
    return stricmp(keyof(x), keyof(y));
}

static int contains(const char *s, const char *f)
{
    int n = strlen(f);
    if (!n) return 1;
    for (; *s; s++) if (!strnicmp(s, f, n)) return 1;
    return 0;
}

static void arrange(void)
{
    int k;
    nrows = 0;
    for (k = 0; k < ntasks; k++) {
        Row *r = &rows[nrows];
        if (o_shells && !tasks[k].cli) continue;
        r->t = &tasks[k];
        r->s = &share[k];
        taskname(&tasks[k], r->name);
        if (!contains(r->name, filter)) continue;
        nrows++;
    }
    qsort(rows, nrows, sizeof *rows, bysort);
}

/* everything measured since the last time */
static void measure(void)
{
    int k;
    ntasks = cu_tasks(tasks, MAXT);
    busy = cu_cpumeasure(tasks, ntasks, share);
    if (busy >= 0) hpush(&hcpu, busy);
    memset(ph2, 0, sizeof ph2);
    for (k = 0; k < ntasks; k++) {
        PHist *o = phfor(ph, tasks[k].addr, 0), *n = phfor(ph2, tasks[k].addr, 1);
        if (!n) continue;
        if (o) memcpy(n->v + 1, o->v, sizeof n->v - 1);
        n->v[0] = (UBYTE)((share[k].pct + 5) / 10);
    }
    memcpy(ph, ph2, sizeof ph);
    readmem();
    netread();
    if (cu_emu68()) mips = cu_emu68mips();
}

static int procrows(int h) { return h - 3 - (info ? 6 : 0); }   /* list rows in the proc box */

static int pbox_x, pbox_y, pbox_w, pbox_h;    /* where the proc box is (the mouse) */

static void keepsel(void)
{
    int k, page = procrows(pbox_h);
    if (page < 1) page = 1;
    if (seladdr) for (k = 0; k < nrows; k++) if (rows[k].t->addr == seladdr) { sel = k; break; }
    if (sel >= nrows) sel = nrows - 1;
    if (sel < 0) sel = 0;
    if (sel < top) top = sel;
    if (sel >= top + page) top = sel - page + 1;
    if (top > nrows - page) top = nrows - page;
    if (top < 0) top = 0;
    seladdr = nrows ? rows[sel].t->addr : NULL;
}

static const char *cpuname(void)
{
    UWORD f = SysBase->AttnFlags;
    if (f & 0x80) return "68060";
    if (f & AFF_68040) return "68040";
    if (f & AFF_68030) return "68030";
    if (f & AFF_68020) return "68020";
    if (f & AFF_68010) return "68010";
    return "68000";
}

static int hasfpu(void) { return (SysBase->AttnFlags & (AFF_68881 | AFF_68882 | AFF_FPU40)) != 0; }

static void drawcpu(int x, int y, int w, int h)
{
    char b[80], *p;
    int rw, gx, gw, i, ly;
    struct DateStamp ds;
    ULONG f, secs;
    unsigned long long t;
    box(x, y, w, h, C_CPUBOX);
    title(x + 1, y, "cpu", -1, 1, C_CPUBOX);
    DateStamp(&ds);
    p = stwo(b, ds.ds_Minute / 60); *p++ = ':'; p = stwo(p, ds.ds_Minute % 60); *p++ = ':';
    stwo(p, ds.ds_Tick / TICKS_PER_SECOND);
    i = strlen(b) + 2;
    title(x + (w - i) / 2, y, b, -1, 0, C_CPUBOX);
    p = sstr(b, "- ");
    p = snum(p, delay10 * 100);
    p = sstr(p, "ms +");
    if (paused) sstr(p, " paused");
    i = strlen(b) + 2;
    {
        int tx = x + w - 1 - i;
        tui_putc(tx, y, fancy ? 0x2510 : '[', C_CPUBOX, TC_DEFAULT, 0);
        tui_put(tx + 1, y, b, 1, C_HOT, TC_DEFAULT, TA_BOLD);
        tui_put(tx + 2, y, b + 1, i - 4, C_TITLE, TC_DEFAULT, 0);
        tui_put(tx + i - 2, y, b + i - 3, 1, C_HOT, TC_DEFAULT, TA_BOLD);
        tui_putc(tx + i - 1, y, fancy ? 0x250C : ']', C_CPUBOX, TC_DEFAULT, 0);
    }
    /* the panel at the right */
    rw = w >= 100 ? 40 : w >= 70 ? 34 : w / 2;
    if (rw > w - 12) rw = w - 12;
    gx = x + 1;
    gw = w - rw - 3;
    graph(gx, y + 1, gw, h - 2, &hcpu, 1000, g_cpu, 0);
    t = eclock(&f);
    secs = f ? (ULONG)(t / f) : 0;
    p = sstr(b, "up ");
    if (secs >= 86400) { p = snum(p, secs / 86400); p = sstr(p, "d "); }
    p = stwo(p, secs / 3600 % 24); *p++ = ':'; stwo(p, secs / 60 % 60);
    text(gx + 1, y + h - 2, b, C_DIM, 0);
    {
        /* boxed when there is room (the name in its border), else plain
         * lines: py the first row, end the row after the last */
        int px = x + w - rw - 1, pw = rw, py = y + 1, boxed = h >= 6, end = boxed ? y + h - 2 : y + h - 1;
        if (boxed) box(px, py, pw, h - 2, C_CPUBOX);
        else { px--; pw += 2; tui_fill(px, py, pw, ' ', C_TEXT, TC_DEFAULT, 0); }
        /* the name line */
        p = b;
        if (cu_emu68() && haspi && pi.emu68[0]) {
            const char *e = pi.emu68;
            int n = 0;
            while (e[n] && e[n] != '(' && n < 24) n++;
            while (n && e[n - 1] == ' ') n--;
            memcpy(p, e, n); p += n; *p = 0;
            p = sstr(p, " ");
        }
        p = sstr(p, cpuname());
        if (hasfpu()) p = sstr(p, "+FPU");
        if (haspi && pi.armhz) {
            ULONG mhz = pi.armhz / 1000000;
            int room = pw - 2 - (int)strlen(b) - 1;
            char c[16], *q = c;
            if (mhz >= 1000) { q = snum(q, mhz / 1000); *q++ = '.'; q = snum(q, mhz % 1000 / 100); q = sstr(q, " GHz"); }
            else { q = snum(q, mhz); q = sstr(q, " MHz"); }
            if ((int)strlen(c) <= room) rtext(px + pw - 1, py, c, C_TEXT, TA_BOLD);
        }
        b[pw - 2] = 0;
        if (boxed) { title(px + 1, py, b, -1, 0, C_CPUBOX); ly = py + 1; }
        else if (h >= 5) { text(px + 1, py, b, C_TITLE, TA_BOLD); ly = py + 1; }
        else ly = py;            /* one row only: the meter */
        if (ly < end) {
            int mw = pw - 2 - 9;
            text(px + 1, ly, "CPU", C_TEXT, TA_BOLD);
            meter(px + 5, ly, mw > 3 ? mw : 3, busy < 0 ? 0 : busy, g_cpu);
            b[0] = 0;
            if (busy >= 0) spct(b, busy);
            rtext(px + pw - 1, ly, b, grad(g_cpu, busy < 0 ? 0 : busy, 1000), TA_BOLD);
            ly++;
        }
        if (ly < end) {
            p = b;
            *p = 0;
            if (mips != (ULONG)-1) { p = snum(p, mips); p = sstr(p, " MIPS  "); }
            /* memory in a word each: the mem box may not fit */
            if (chiptot) { p = sstr(p, "chip "); p = spct(p, (int)((unsigned long long)(chiptot - chipfree) * 1000 / chiptot)); }
            if (fasttot) { p = sstr(p, " fast "); p = spct(p, (int)((unsigned long long)(fasttot - fastfree) * 1000 / fasttot)); }
            if ((int)strlen(b) > pw - 2) b[pw - 2] = 0;
            text(px + 1, ly, b, C_DIM, 0);
            ly++;
        }
        if (haspi && ly < end && pi.temp) {
            p = sstr(b, "Pi ");
            p = snum(p, pi.temp / 1000);
            p = sstr(p, "\xB0" "C");
            if (pi.tempmax) { p = sstr(p, " (max "); p = snum(p, pi.tempmax / 1000); p = sstr(p, ")"); }
            text(px + 1, ly, b, pi.temp >= 80000 ? C_HOT : C_TEXT, 0);
            if (pi.hasthrottle && (pi.throttled & 0xF))
                rtext(px + pw - 1, ly, pi.throttled & 1 ? "low power!" : "throttled", C_HOT, TA_BOLD);
            ly++;
        }
        if (ly < end && haspi && pi.model[0]) {
            char m[40];
            const char *s = pi.model;
            int n;
            if (!strncmp(s, "Raspberry Pi ", 13)) { strcpy(m, "Pi "); strncat(m, s + 13, sizeof m - 4); }
            else strncpy(m, s, sizeof m - 1);
            m[sizeof m - 1] = 0;
            n = strlen(m);
            if (n > pw - 2) m[pw - 2] = 0;
            text(px + 1, ly, m, C_DIM, 0);
        }
    }
}

static void drawmem(int x, int y, int w, int h)
{
    char b[48], *p;
    int iw = w - 2, ly = y + 1, end = y + h - 1, kind, i, dw;
    box(x, y, w, h, C_MEMBOX);
    title(x + 1, y, "mem", -1, 2, C_MEMBOX);
    for (kind = 0; kind < 2 && ly < end; kind++) {
        ULONG tot = kind ? fasttot : chiptot, fr = kind ? fastfree : chipfree, big = kind ? fastbig : chipbig;
        int used;
        if (!tot) continue;
        used = (int)((unsigned long long)(tot - fr) * 1000 / tot);
        text(x + 1, ly, kind ? "Fast:" : "Chip:", C_TEXT, TA_BOLD);
        p = sbytes(b, tot);
        rtext(x + 1 + iw, ly, b, C_TEXT, TA_BOLD);
        if (iw >= 30) {
            p = sstr(b, "largest ");
            sbytes(p, big);
            rtext(x + 1 + iw - 7, ly, b, C_DIM, 0);
        }
        if (++ly >= end) break;
        text(x + 1, ly, "Used:", C_DIM, 0);
        dw = iw - 6 - 11;
        if (dw >= 4) meter(x + 7, ly, dw, used, g_cpu);
        p = spct(b, used); *p++ = ' '; *p = 0;
        sbytes(p, tot - fr);
        rtext(x + 1 + iw, ly, b, C_TEXT, 0);
        if (++ly >= end) break;
        /* a graph of used, when there is room for disks too */
        if (end - ly >= 6 + nvols) {
            graph(x + 1, ly, iw, 2, kind ? &hfast : &hchip, 1000, g_cpu, 0);
            ly += 2;
        }
    }
    /* the volumes */
    if (ly + 2 < end && nvols) {
        tui_fill(x + 1, ly, iw, tui_box(BX_H), C_MEMBOX, TC_DEFAULT, 0);
        title(x + 2, ly, "disks", 0, 0, C_MEMBOX);
        ly++;
        for (i = 0; i < nvols && ly < end; i++) {
            Vol *v = &vols[i];
            int used = v->kb ? (int)((unsigned long long)v->usedkb * 1000 / v->kb) : 0, nw = iw / 3;
            char nm[32];
            if (nw < 9) nw = 9;
            if (nw > 20) nw = 20;
            strncpy(nm, v->name, nw);
            nm[nw] = 0;
            text(x + 1, ly, nm, C_TEXT, TA_BOLD);
            p = sbytes(b, (unsigned long long)v->kb * 1024);
            rtext(x + 1 + iw, ly, b, C_TEXT, 0);
            dw = iw - nw - 1 - 6 - 5;
            if (dw >= 3) meter(x + 1 + nw + 1, ly, dw, used, g_cpu);
            spct(b, used);
            rtext(x + 1 + iw - 6, ly, b, C_DIM, 0);
            ly++;
        }
    }
}

static unsigned long long nicemax(unsigned long long v)    /* a scale above v */
{
    unsigned long long s = 1024;
    while (s < v) s *= 2;
    return s;
}

static void drawnet(int x, int y, int w, int h)
{
    char b[48], *p;
    int iw = w - 2, gw, ih = h - 2, half, px;
    box(x, y, w, h, C_NETBOX);
    title(x + 1, y, "net", -1, 3, C_NETBOX);
    if (netok && (netip[0] || netif[0])) {
        p = sstr(b, netip[0] ? netip : netif);
        title(x + 7, y, b, -1, 0, C_NETBOX);
    }
    if (!netok) {
        const char *a = SocketBase ? "the network stack has no" : "no bsdsocket.library:", *b2 = SocketBase ? "byte counters (Roadshow's API)" : "no network";
        text(x + (w - (int)strlen(a)) / 2, y + ih / 2, a, C_DIM, 0);
        text(x + (w - (int)strlen(b2)) / 2, y + ih / 2 + 1, b2, C_DIM, 0);
        return;
    }
    px = iw >= 40 ? 22 : 17;
    gw = iw - px - 1;
    half = ih / 2;
    {
        int i, n = gw * 2;
        unsigned long long m = 0;
        for (i = 0; i < n; i++) {
            int a = hget(&hdown, i), c = hget(&hup, i);
            if (a > (long long)m) m = a;
            if (c > (long long)m) m = c;
        }
        rxmax = txmax = nicemax(m);
    }
    if (gw >= 4) {
        graph(x + 1, y + 1, gw, half, &hdown, (int)(rxmax > 0x7fffffff ? 0x7fffffff : rxmax), g_down, 0);
        graph(x + 1, y + 1 + half, gw, ih - half, &hup, (int)(txmax > 0x7fffffff ? 0x7fffffff : txmax), g_up, 1);
        p = sbytes(b, rxmax);
        text(x + 1, y + 1, b, C_DIM, 0);
    }
    {
        int ox = x + 1 + gw + 1, ly = y + 1;
        tui_putc(ox, ly, 'v', g_down[5], TC_DEFAULT, TA_BOLD);
        p = sbytes(b, rxrate); sstr(p, "/s");
        text(ox + 2, ly, b, C_TEXT, TA_BOLD);
        if (ly + 1 < y + 1 + half) {
            p = sstr(b, "Total ");
            sbytes(p, netrx);
            text(ox + 2, ly + 1, b, C_DIM, 0);
        }
        ly = y + 1 + half;
        tui_putc(ox, ly, '^', g_up[5], TC_DEFAULT, TA_BOLD);
        p = sbytes(b, txrate); sstr(p, "/s");
        text(ox + 2, ly, b, C_TEXT, TA_BOLD);
        if (ly + 1 < y + h - 1) {
            p = sstr(b, "Total ");
            sbytes(p, nettx);
            text(ox + 2, ly + 1, b, C_DIM, 0);
        }
    }
}

/* the proc box's columns, by the room there is */
static int c_pri, c_free, c_time, c_addr, c_stack;

static void lnum(char *b, long v, int w)    /* right aligned in w */
{
    char t[16];
    int n;
    if (v < 0) { t[0] = '-'; snum(t + 1, -v); }
    else snum(t, v);
    n = strlen(t);
    memset(b, ' ', w);
    if (n > w) n = w;
    memcpy(b + w - n, t, n);
    b[w] = 0;
}

static void drawproc(int x, int y, int w, int h, const char *prompt)
{
    char b[300], *p;
    int iw = w - 2, ly, nw, list, r, cx;
    pbox_x = x; pbox_y = y; pbox_w = w; pbox_h = h;
    box(x, y, w, h, C_PROBOX);
    cx = x + 1;
    cx += title(cx, y, "proc", -1, 4, C_PROBOX);
    if (filter[0] || iw >= 50) {
        p = sstr(b, "filter");
        if (filter[0]) { p = sstr(p, " "); p = sstr(p, filter); }
        cx += title(cx, y, b, 0, 0, C_PROBOX);
    }
    if (iw >= 44) cx += title(cx, y, o_shells ? "shells" : "every", 0, 0, C_PROBOX);
    if (iw >= 56) cx += title(cx, y, "reverse", 0, 0, C_PROBOX);
    {
        int n = strlen(sortnames[sortcol]) + 4;
        int tx = x + w - 1 - n;
        if (tx > cx) {
            tui_putc(tx, y, fancy ? 0x2510 : '[', C_PROBOX, TC_DEFAULT, 0);
            tui_putc(tx + 1, y, fancy ? 0x2190 : '<', C_HOT, TC_DEFAULT, TA_BOLD);
            text(tx + 2, y, sortnames[sortcol], C_TITLE, TA_BOLD);
            tui_putc(tx + n - 2, y, fancy ? 0x2192 : '>', C_HOT, TC_DEFAULT, TA_BOLD);
            tui_putc(tx + n - 1, y, fancy ? 0x250C : ']', C_PROBOX, TC_DEFAULT, 0);
        }
    }
    c_pri = iw >= 38;
    c_free = iw >= 50;
    c_time = iw >= 62;
    c_addr = iw >= 80;
    c_stack = iw >= 90;
    nw = iw - 4 - (c_addr ? 9 : 0) - (c_pri ? 9 : 0) - (c_stack ? 7 : 0) - (c_free ? 7 : 0) - 6 - 6 - (c_time ? 9 : 0);
    if (nw < 8) nw = 8;
    if (nw > 160) nw = 160;      /* (b holds a row of the widest screen) */
    ly = y + 1;
    if (info && nrows && sel < nrows) {
        CuTask *t = rows[sel].t;
        CuShare *s = rows[sel].s;
        int bx = x + 1;
        tui_fill(bx, ly + 5, iw, tui_box(BX_H), C_PROBOX, TC_DEFAULT, 0);
        p = sstr(b, rows[sel].name);
        if ((int)strlen(b) > iw - 2) b[iw - 2] = 0;
        text(bx + 1, ly, b, C_TITLE, TA_BOLD);
        p = sstr(b, t->process ? "process " : "task ");
        { int i; for (i = 0; i < 8; i++) *p++ = "0123456789abcdef"[((ULONG)t->addr >> (28 - 4 * i)) & 15]; *p = 0; }
        if (t->cli) { p = sstr(p, "  CLI "); p = snum(p, t->cli); }
        text(bx + 1, ly + 1, b, C_TEXT, 0);
        p = sstr(b, "[");
        p = sstr(p, t->name);
        p = sstr(p, "]");
        if ((int)strlen(b) > iw - 2) b[iw - 2] = 0;
        text(bx + 1, ly + 2, b, C_DIM, 0);
        p = sstr(b, "stack ");
        p = snum(p, t->stack);
        if (t->free >= 0) { p = sstr(p, ", free "); p = snum(p, t->free); }
        p = sstr(p, "  pri ");
        if (t->pri < 0) { *p++ = '-'; p = snum(p, -t->pri); } else p = snum(p, t->pri);
        p = sstr(p, t->state == 0 ? "  running" : t->state == 1 ? "  ready" : "  waiting");
        text(bx + 1, ly + 3, b, C_TEXT, 0);
        p = sstr(b, "CPU ");
        p = spct(p, s->pct);
        if (s->timed && cu_cpufreq()) {
            ULONG hs = (ULONG)(s->time * 100 / cu_cpufreq());
            p = sstr(p, "  time ");
            p = snum(p, hs / 6000); *p++ = ':'; p = stwo(p, hs / 100 % 60); *p++ = '.'; p = stwo(p, hs % 100);
        }
        p = sstr(p, "  signals ");
        { int i; for (i = 0; i < 8; i++) *p++ = "0123456789abcdef"[(t->sigwait >> (28 - 4 * i)) & 15]; *p = 0; }
        text(bx + 1, ly + 4, b, C_TEXT, 0);
        ly += 6;
    }
    /* the titles */
    p = b;
    p = sstr(p, "CLI ");
    if (c_addr) p = sstr(p, "Address  ");
    {
        const char *hd = o_names ? "Task:" : "Program:";
        int n = strlen(hd);
        memcpy(p, hd, n); memset(p + n, ' ', nw - n); p += nw; *p = 0;
    }
    if (c_pri) p = sstr(p, " Pri St  ");
    if (c_stack) p = sstr(p, " Stack");
    if (c_free) p = sstr(p, "  Free");
    p = sstr(p, "       Cpu%");
    if (c_time) p = sstr(p, "    TIME+");
    b[iw] = 0;
    text(x + 1, ly, b, C_TEXT, TA_BOLD);
    ly++;
    list = y + h - 1 - ly;
    for (r = 0; r < list; r++) {
        int i = top + r, fg, bg = TC_DEFAULT, at = 0, gx;
        Row *w2;
        CuTask *t;
        if (i >= nrows) break;
        w2 = &rows[i];
        t = w2->t;
        p = b;
        if (t->cli) lnum(p, t->cli, 3); else strcpy(p, "  -");
        p += 3; *p++ = ' ';
        if (c_addr) { int k; for (k = 0; k < 8; k++) *p++ = "0123456789abcdef"[((ULONG)t->addr >> (28 - 4 * k)) & 15]; *p++ = ' '; }
        {
            int n = strlen(w2->name);
            if (n > nw - 1) n = nw - 1;
            memcpy(p, w2->name, n); memset(p + n, ' ', nw - n); p += nw;
        }
        if (c_pri) { lnum(p, t->pri, 4); p += 4; p = sstr(p, t->state == 0 ? " run " : t->state == 1 ? " rdy " : " wai "); }
        if (c_stack) { lnum(p, t->stack, 6); p += 6; }
        if (c_free) { if (t->free >= 0) lnum(p, t->free, 6); else strcpy(p, "     ?"); p += 6; }
        *p = 0;
        if (i == sel) { bg = C_SELBG; fg = TC_X(255); at = TA_BOLD; }
        else fg = t->state < 2 ? TC_X(255) : C_TEXT;
        tui_put(x + 1, ly + r, b, strlen(b), fg, bg, at);
        gx = x + 1 + strlen(b) + 1;
        /* the small graph: five cells, ten values */
        {
            PHist *hh = phfor(ph, t->addr, 0);
            int k;
            tui_putc(gx - 1, ly + r, ' ', fg, bg, at);
            for (k = 0; k < 5; k++) {
                int vl = hh ? hh->v[9 - 2 * k] : 0, vr = hh ? hh->v[8 - 2 * k] : 0;
                ULONG ch;
                if (fancy) {
                    UBYTE bits = 0;
                    int fl = vl ? (vl * 4 + 99) / 100 : 0, fr = vr ? (vr * 4 + 99) / 100 : 0, q;
                    static const UBYTE lb[4] = { 0x40, 0x04, 0x02, 0x01 }, rb[4] = { 0x80, 0x20, 0x10, 0x08 };
                    for (q = 0; q < 4; q++) { if (q < fl) bits |= lb[q]; if (q < fr) bits |= rb[q]; }
                    ch = bits ? 0x2800 + bits : 0x28C0;
                    tui_putc(gx + k, ly + r, ch, bits ? grad(g_cpu, vl > vr ? vl : vr, 100) : C_EMPTY, bg, 0);
                } else {
                    int v = vl > vr ? vl : vr;
                    ch = v >= 50 ? '#' : v >= 10 ? ':' : v ? '.' : ' ';
                    tui_putc(gx + k, ly + r, ch, grad(g_cpu, v, 100), bg, 0);
                }
            }
            gx += 5;
        }
        {
            int pct = w2->s->pct;
            p = b;
            lnum(p, pct / 10, 4); p += 4; *p++ = '.'; *p++ = '0' + pct % 10;
            if (c_time) {
                if (w2->s->timed && cu_cpufreq()) {
                    ULONG hs = (ULONG)(w2->s->time * 100 / cu_cpufreq()), m = hs / 6000;
                    char tb[12], *q = tb;
                    if (m < 100) { q = snum(q, m); *q++ = ':'; q = stwo(q, hs / 100 % 60); *q++ = '.'; q = stwo(q, hs % 100); }
                    else { q = snum(q, m); *q++ = ':'; q = stwo(q, hs / 100 % 60); }
                    *q = 0;
                    memset(p, ' ', 9);
                    memcpy(p + 9 - strlen(tb), tb, strlen(tb));
                    p += 9;
                } else { memcpy(p, "        -", 9); p += 9; }
            }
            *p = 0;
            tui_put(gx, ly + r, b, 6, i == sel ? fg : grad(g_cpu, pct, 1000), bg, i == sel ? at : pct >= 100 ? TA_BOLD : 0);
            if (c_time) tui_put(gx + 6, ly + r, b + 6, strlen(b + 6), fg, bg, at);
            gx += strlen(b);
            if (gx < x + 1 + iw) tui_fill(gx, ly + r, x + 1 + iw - gx, ' ', fg, bg, at);
        }
    }
    /* the bottom border: the hints, or the prompt */
    if (prompt) {
        int n = strlen(prompt);
        if (n > iw - 2) n = iw - 2;
        tui_put(x + 2, y + h - 1, prompt, n, C_TITLE, TC_DEFAULT, TA_BOLD);
        tui_cursor(x + 2 + n, y + h - 1, 1);
    } else {
        char k1[12];
        int bx = x + 2;
        if (msg[0]) {
            int n = strlen(msg);
            if (n > iw - 12) n = iw - 12;
            tui_put(bx, y + h - 1, msg, n, C_HOT, TC_DEFAULT, TA_BOLD);
        } else {
            firstkey(A_INFO, k1);
            if (iw >= 30) bx += title(bx, y + h - 1, "info", -1, 0, C_PROBOX);
            if (iw >= 40) bx += title(bx, y + h - 1, "break", 0, 0, C_PROBOX);
            if (iw >= 50) bx += title(bx, y + h - 1, "signal", 0, 0, C_PROBOX);
            if (iw >= 60) bx += title(bx, y + h - 1, "Nice", 0, 0, C_PROBOX);
            if (iw >= 70) bx += title(bx, y + h - 1, "help", 0, 0, C_PROBOX);
        }
        p = snum(b, nrows ? sel + 1 : 0); *p++ = '/'; snum(p, nrows);
        rtext(x + w - 2, y + h - 1, b, C_TITLE, TA_BOLD);
        tui_cursor(0, tui_rows - 1, 0);
    }
}

static void drawall(const char *prompt)
{
    int W = tui_cols, H = tui_rows, cpuh = 0, lw = 0, rest, memh = 0, neth = 0, side;
    if (W < MINW || H < MINH) {
        char b[60], *p;
        tui_clear();
        p = sstr(b, "Window too small: ");
        p = snum(p, W); *p++ = 'x'; p = snum(p, H);
        text((W - (int)strlen(b)) / 2 > 0 ? (W - (int)strlen(b)) / 2 : 0, H / 2 - 1, b, C_TITLE, TA_BOLD);
        text((W - 18) / 2 > 0 ? (W - 18) / 2 : 0, H / 2, "ctop needs 40 x 12", C_DIM, 0);
        tui_cursor(0, H - 1, 0);
        tui_flush();
        return;
    }
    /* what fits: the mem/net column only from 70 wide (or with no proc
     * box), the net box only when there are rows for it, the cpu box
     * smaller in a short window (a stock Shell window on PAL hires is
     * about 76 x 27) */
    side = (show[1] || show[2]) && (W >= 70 || !show[3]);
    if (show[0]) {
        cpuh = H * 3 / 10;
        if (cpuh < (H < 20 ? 4 : 6)) cpuh = H < 20 ? 4 : 6;
        if (show[3] || side) { if (cpuh > H - 6) cpuh = H - 6; }
        else cpuh = H;
        if (cpuh < 4) cpuh = 4;
    }
    if (show[0]) drawcpu(0, 0, W, cpuh);
    rest = H - cpuh;
    if (rest <= 0) { tui_flush(); return; }
    if (side) {
        lw = show[3] ? W * 45 / 100 : W;
        if (show[3] && lw < 30) lw = 30;
        if (show[3] && W - lw < 36) lw = W - 36;
        if (show[2] && (rest >= 14 || !show[1])) {
            neth = show[1] ? rest * 3 / 10 : rest;
            if (neth < 6) neth = 6;
        }
        memh = show[1] ? rest - neth : 0;
        if (show[1] && memh < 4) { memh = 0; neth = show[2] ? rest : 0; }
        if (memh) drawmem(0, cpuh, lw, memh);
        if (neth) drawnet(0, cpuh + rest - neth, lw, neth);
        if (!memh && !neth) lw = 0;
    }
    if (show[3]) drawproc(lw, cpuh, W - lw, rest, prompt);
    else {
        if (!lw) tui_fill(0, cpuh, W, ' ', C_TEXT, TC_DEFAULT, 0);
        tui_cursor(0, H - 1, 0);
    }
    tui_flush();
}

/* ---- help, questions --------------------------------------------------- */

static int helpview(void)
{
    TuiEv e;
    int i, row, col, cw;
    char k[16], b[90];
    for (;;) {
        tui_clear();
        text(1, 0, "ctop 0.1b1 - btop for the Amiga. Keys (ENVARC:ctop.keys):", C_TITLE, TA_BOLD);
        cw = tui_cols >= 100 ? tui_cols / 2 : tui_cols;
        row = 2; col = 1;
        for (i = 1; i < NACT; i++) {
            int j, n = 0;
            b[0] = 0;
            for (j = 0; j < nbinds; j++) if (binds[j].act == i && n < 3) {
                keyname(binds[j].key, k);
                if (n++) strcat(b, " ");
                strcat(b, k);
            }
            if (!n) strcpy(b, "-");
            tui_put(col, row, b, strlen(b) < 18 ? strlen(b) : 18, C_HOT, TC_DEFAULT, TA_BOLD);
            text(col + 19, row, actdesc[i], C_TEXT, 0);
            if (++row >= tui_rows - 2) { if (col > 1 || cw == tui_cols) break; row = 2; col = cw + 1; }
        }
        if (keybad[0]) text(1, tui_rows - 2, keybad, C_HOT, 0);
        text(1, tui_rows - 1, cu_cpuexact() ? "CPU% is exact (exec's task hooks). Any key goes back." :
             "CPU% is sampled (another top has the hooks). Any key goes back.", C_DIM, 0);
        tui_cursor(0, tui_rows - 1, 0);
        tui_flush();
        if (!tui_wait(&e, -1, 0, NULL)) continue;
        if (e.type == TE_RESIZE) { tui_resized(); continue; }
        if (e.type == TE_KEY) return e.key != (TM_CTRL | 'c');
    }
}

/* a line typed in the proc box's bottom border; 0: Esc */
static int ask(const char *q, char *buf, int size)
{
    TuiEv e;
    int n = strlen(buf);
    char st[120];
    for (;;) {
        strcpy(st, q);
        strcat(st, buf);
        if (show[3]) drawall(st);
        else { show[3] = 1; drawall(st); show[3] = 0; }
        if (!tui_wait(&e, 1000000, 0, NULL)) { cu_cpudrain(); continue; }
        if (e.type == TE_RESIZE) { tui_resized(); continue; }
        if (e.type != TE_KEY) continue;
        if (e.key == K_ENTER) return 1;
        if (e.key == K_ESC || e.key == (TM_CTRL | 'c')) return 0;
        if (e.key == K_BS || e.key == K_DEL) { if (n) buf[--n] = 0; continue; }
        if (e.key >= 32 && e.key < 256 && e.key != 127 && n < size - 1) { buf[n++] = e.key; buf[n] = 0; }
    }
}

static CuTask *selected(void) { return nrows && sel < nrows ? rows[sel].t : NULL; }

static void dobreak(ULONG sig, const char *what)
{
    CuTask *t = selected();
    if (!t) return;
    if (cu_signal(t->addr, sig)) { strcpy(msg, what); strcat(msg, " sent to "); strncat(msg, rows[sel].name, 60); }
    else strcpy(msg, "that task is gone");
}

static void dosignal(void)
{
    char b[4] = "";
    CuTask *t = selected();
    if (!t) return;
    if (!ask("Send which break? C D E F > ", b, sizeof b) || !b[0]) { msg[0] = 0; return; }
    switch (b[0] | 32) {
    case 'c': dobreak(SIGBREAKF_CTRL_C, "Ctrl-C"); break;
    case 'd': dobreak(SIGBREAKF_CTRL_D, "Ctrl-D"); break;
    case 'e': dobreak(SIGBREAKF_CTRL_E, "Ctrl-E"); break;
    case 'f': dobreak(SIGBREAKF_CTRL_F, "Ctrl-F"); break;
    default: strcpy(msg, "not C, D, E or F");
    }
}

static void dopri(void)
{
    char b[8] = "", q[60], *p;
    CuTask *t = selected();
    int ok;
    long v;
    if (!t) return;
    p = sstr(q, "Priority (now ");
    if (t->pri < 0) { *p++ = '-'; p = snum(p, -t->pri); } else p = snum(p, t->pri);
    sstr(p, ", -128 to 127) > ");
    if (!ask(q, b, sizeof b) || !b[0]) { msg[0] = 0; return; }
    v = b[0] == '-' ? -cu_num(b + 1, &ok) : cu_num(b, &ok);
    if (!ok || v < -128 || v > 127) { strcpy(msg, "not a priority: "); strcat(msg, b); return; }
    {
        struct Node *n;
        int alive = 0;
        /* (SetTaskPri on a task that ended in between would write into
         * freed memory: the check and the call are under Forbid) */
        Forbid();
        if ((APTR)SysBase->ThisTask == t->addr) alive = 1;
        for (n = SysBase->TaskReady.lh_Head; !alive && n->ln_Succ; n = n->ln_Succ) if ((APTR)n == t->addr) alive = 1;
        for (n = SysBase->TaskWait.lh_Head; !alive && n->ln_Succ; n = n->ln_Succ) if ((APTR)n == t->addr) alive = 1;
        if (alive) SetTaskPri((struct Task *)t->addr, v);
        Permit();
        if (!alive) { strcpy(msg, "that task is gone"); return; }
    }
    strcpy(msg, "priority set");
}

/* tenths of seconds from "1", "0.5"; 0: not a delay */
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
    return v * 10 + d;
}

/* ---- the loop ------------------------------------------------------------ */

static int run(void)
{
    TuiEv e;
    ULONG next, lastvol = 0, lastpi = 0;
    tui_title("ctop");
    loadkeys();
    netopen();
    haspi = cu_pi(&pi);
    readvols();
    lastvol = cu_cpuclock();
    measure();
    arrange();
    keepsel();
    drawall(NULL);
    next = cu_cpuclock() + (ULONG)delay10 * cu_cpurate() / 10;
    for (;;) {
        LONG hz = cu_cpurate(), left = (LONG)(next - cu_cpuclock());
        if (left <= 0) {
            ULONG now = cu_cpuclock();
            if (!paused) {
                measure();
                if (now - lastvol >= (ULONG)hz * 10) { readvols(); lastvol = now; }
                if (haspi && now - lastpi >= (ULONG)hz * 2) { cu_pi(&pi); lastpi = now; }
                arrange();
                keepsel();
            } else cu_cpumeasure(tasks, ntasks, share);   /* (keeps the hooks' sums small) */
            drawall(NULL);
            next = now + (ULONG)delay10 * hz / 10;
            continue;
        }
        if (left > hz / 2) left = hz / 2;    /* the hooks' ring read twice a second */
        cu_cpudrain();
        if (!tui_wait(&e, left * (1000000 / hz), 0, NULL)) continue;
        if (e.type == TE_RESIZE) { tui_resized(); keepsel(); drawall(NULL); continue; }
        if (e.type == TE_MOUSE) {
            int lr = pbox_y + 2 + (info ? 6 : 0);
            if (e.mact == TB_WHEELUP) sel -= 3;
            else if (e.mact == TB_WHEELDOWN) sel += 3;
            else if (e.mact == TB_PRESS && show[3] && e.col > pbox_x && e.col < pbox_x + pbox_w - 1 &&
                     e.row >= lr && e.row < pbox_y + pbox_h - 1 && top + e.row - lr < nrows) sel = top + e.row - lr;
            else continue;
            seladdr = NULL;
            keepsel();
            drawall(NULL);
            continue;
        }
        if (e.type != TE_KEY) continue;
        {
            int page = procrows(pbox_h), moved = 0, again = 0, a = action(e.key);
            msg[0] = 0;
            if (page < 1) page = 1;
            switch (a) {
            case A_QUIT: return 1;
            case A_HELP: if (!helpview()) return 1; break;
            case A_UP: sel--; moved = 1; break;
            case A_DOWN: sel++; moved = 1; break;
            case A_PGUP: sel -= page; moved = 1; break;
            case A_PGDN: sel += page; moved = 1; break;
            case A_HOME: sel = 0; moved = 1; break;
            case A_END: sel = nrows - 1; moved = 1; break;
            case A_INFO: info = !info; break;
            case A_BREAK: dobreak(SIGBREAKF_CTRL_C, "Ctrl-C"); break;
            case A_SIGNAL: dosignal(); break;
            case A_PRI: dopri(); again = 1; break;
            case A_SORTL: sortcol = (sortcol + NSORT - 1) % NSORT; break;
            case A_SORTR: sortcol = (sortcol + 1) % NSORT; break;
            case A_REVERSE: reverse = !reverse; break;
            case A_FILTER: if (!ask("Filter > ", filter, sizeof filter)) filter[0] = 0; msg[0] = 0; break;
            case A_SHELLS: o_shells = !o_shells; break;
            case A_NAMES: o_names = !o_names; break;
            case A_BOX1: case A_BOX2: case A_BOX3: case A_BOX4: show[a - A_BOX1] = !show[a - A_BOX1]; break;
            case A_FASTER: delay10 = delay10 > 10 ? delay10 - 5 : delay10 > 2 ? delay10 - 1 : 2; break;
            case A_SLOWER: delay10 = delay10 < 10 ? delay10 + 1 : delay10 < 600 ? delay10 + 5 : 600; break;
            case A_PAUSE: paused = !paused; break;
            case A_REFRESH: tui_redraw(); again = 1; break;
            default: continue;
            }
            if (moved) seladdr = NULL;
            if (again) {
                measure();
                next = cu_cpuclock() + (ULONG)delay10 * cu_cpurate() / 10;
            }
            arrange();
            keepsel();
            drawall(NULL);
        }
    }
}

static int cmain(void)
{
    struct timerequest *tr;
    char *v;
    int c;
    (void)version;
    if (cu_args("ctop [-d SECONDS]") < 0) return cu_rc;
    while ((c = cu_opt("d:", &v)) >= 0) switch (c) {
        case 'd': if ((delay10 = tenths(v)) < 1) { cu_err(10, v, "not a delay in seconds"); return cu_rc; } break;
        default: return cu_rc;
    }
    if (cu_argc) { cu_err(10, cu_argv[0], "unexpected argument"); return cu_rc; }
    if (!tui_possible()) { cu_err(20, NULL, "needs a terminal window (top -b gives text)"); return cu_rc; }
    if (!(tr = AllocVec(sizeof *tr, MEMF_PUBLIC | MEMF_CLEAR))) { cu_err(20, NULL, "no memory"); return cu_rc; }
    if (OpenDevice(TIMERNAME, UNIT_ECLOCK, (struct IORequest *)tr, 0)) { FreeVec(tr); cu_err(20, NULL, "no timer.device"); return cu_rc; }
    TimerBase = tr->tr_node.io_Device;
    if (!cu_cpuopen()) cu_err(20, NULL, "no memory");
    else {
        if (!tui_open()) cu_err(20, NULL, "can not open the terminal");
        else {
            fancy = tui_cterm && tui_utf8;
            run();
            tui_close();
        }
        cu_cpuclose();
    }
    if (SocketBase) CloseLibrary(SocketBase);
    CloseDevice((struct IORequest *)tr);
    FreeVec(tr);
    cu_flush();
    return cu_rc;
}

/* the Shell's usual 4 KB is too little for the drawing (and its 64-bit
 * sums): a stack of our own, as wget does */
#define STACKSZ 16384
static struct StackSwapStruct sss;
static int sret;

static void __attribute__((noinline)) runswapped(void) { sret = cmain(); }

int main(void)
{
    struct Task *me = FindTask(NULL);
    char *big;
    if ((char *)me->tc_SPUpper - (char *)me->tc_SPLower >= STACKSZ || !(big = AllocVec(STACKSZ, MEMF_ANY)))
        return cmain();
    sss.stk_Lower = big;
    sss.stk_Upper = (ULONG)(big + STACKSZ);
    sss.stk_Pointer = (APTR)(big + STACKSZ);
    StackSwap(&sss);
    runswapped();
    StackSwap(&sss);
    FreeVec(big);
    return sret;
}
